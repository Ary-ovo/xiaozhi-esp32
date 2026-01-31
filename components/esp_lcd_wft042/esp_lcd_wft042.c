/* esp_lcd_wft042.c */
#include "esp_lcd_wft042.h" // <--- 关键：这里已经包含了结构体定义

#include <stdlib.h>
#include <sys/cdefs.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"

static const char *TAG = "wft042";

// 墨水屏命令集
#define CMD_DRIVER_OUTPUT_CONTROL    0x01
#define CMD_BOOSTER_SOFT_START       0x0C
#define CMD_DEEP_SLEEP_MODE          0x10
#define CMD_DATA_ENTRY_MODE          0x11
#define CMD_SW_RESET                 0x12
#define CMD_MASTER_ACTIVATION        0x20
#define CMD_DISPLAY_UPDATE_CONTROL_2 0x22
#define CMD_WRITE_RAM                0x24
#define CMD_WRITE_VCOM_REGISTER      0x2C
#define CMD_SET_RAM_X_ADDRESS_START_END 0x44
#define CMD_SET_RAM_Y_ADDRESS_START_END 0x45
#define CMD_SET_RAM_X_ADDRESS_COUNTER   0x4E
#define CMD_SET_RAM_Y_ADDRESS_COUNTER   0x4F

// 私有数据结构 (这个可以保留在 .c 里，因为 .h 没用到它)
typedef struct {
    esp_lcd_panel_t base;
    esp_lcd_panel_io_handle_t io;
    int reset_gpio_num;
    int busy_gpio_num;
    int busy_level;
} wft042_panel_t;

// 辅助函数：等待忙
static void _wait_busy(wft042_panel_t *panel) {
    if (panel->busy_gpio_num >= 0) {
        int timeout = 2000; // 适当增加超时时间
        // 根据配置的 busy_level 判断
        while (gpio_get_level((gpio_num_t)panel->busy_gpio_num) == panel->busy_level) {
            vTaskDelay(pdMS_TO_TICKS(10));
            timeout--;
            if (timeout <= 0) {
                ESP_LOGW(TAG, "Wait busy timeout");
                break;
            }
        }
    }
}

// 默认初始化序列 (WFT042 / UC8176)
// ⚠️ 这里直接使用 wft042_init_cmd_t，不需要再 typedef struct 了
static const wft042_init_cmd_t vendor_init_cmds[] = {
    {CMD_SW_RESET, NULL, 0, 10}, 
    {0x00, (uint8_t []){0xDF, 0x0E}, 2, 0},
    {CMD_DRIVER_OUTPUT_CONTROL, (uint8_t []){0x2B, 0x01, 0x00}, 3, 0},
    {CMD_BOOSTER_SOFT_START, (uint8_t []){0xAE, 0xC7, 0xC3, 0xC0, 0x40}, 5, 0},
    {CMD_WRITE_VCOM_REGISTER, (uint8_t []){0x28}, 1, 0},
    {CMD_DATA_ENTRY_MODE, (uint8_t []){0x03}, 1, 0},
};

// 函数声明
static esp_err_t panel_wft042_del(esp_lcd_panel_t *panel);
static esp_err_t panel_wft042_reset(esp_lcd_panel_t *panel);
static esp_err_t panel_wft042_init(esp_lcd_panel_t *panel);
static esp_err_t panel_wft042_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start, int x_end, int y_end, const void *color_data);
static esp_err_t panel_wft042_invert_color(esp_lcd_panel_t *panel, bool invert_color_data);
static esp_err_t panel_wft042_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y);
static esp_err_t panel_wft042_swap_xy(esp_lcd_panel_t *panel, bool swap_axes);
static esp_err_t panel_wft042_disp_on_off(esp_lcd_panel_t *panel, bool off);

// 创建函数实现
esp_err_t esp_lcd_new_panel_wft042(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel)
{
    wft042_panel_t *wft042 = (wft042_panel_t *)calloc(1, sizeof(wft042_panel_t));
    if (!wft042) return ESP_ERR_NO_MEM;

    // 处理 Reset 引脚
    if (panel_dev_config->reset_gpio_num >= 0) {
        gpio_config_t io_conf = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << panel_dev_config->reset_gpio_num,
        };
        gpio_config(&io_conf);
    }

    // 处理 Vendor Config (BUSY 引脚)
    wft042_vendor_config_t *vendor_cfg = (wft042_vendor_config_t *)panel_dev_config->vendor_config;
    if (vendor_cfg) {
        wft042->busy_gpio_num = vendor_cfg->busy_gpio_num;
        wft042->busy_level = vendor_cfg->busy_level;
        if (wft042->busy_gpio_num >= 0) {
            gpio_config_t io_conf = {
                .mode = GPIO_MODE_INPUT,
                .pull_up_en = GPIO_PULLUP_ENABLE,
                .pin_bit_mask = 1ULL << wft042->busy_gpio_num,
            };
            gpio_config(&io_conf);
        }
    } else {
        wft042->busy_gpio_num = -1;
    }

    wft042->io = io;
    wft042->reset_gpio_num = panel_dev_config->reset_gpio_num;

    wft042->base.del = panel_wft042_del;
    wft042->base.reset = panel_wft042_reset;
    wft042->base.init = panel_wft042_init;
    wft042->base.draw_bitmap = panel_wft042_draw_bitmap;
    wft042->base.invert_color = panel_wft042_invert_color;
    wft042->base.mirror = panel_wft042_mirror;
    wft042->base.swap_xy = panel_wft042_swap_xy;
    wft042->base.disp_on_off = panel_wft042_disp_on_off;

    *ret_panel = &(wft042->base);
    return ESP_OK;
}

static esp_err_t panel_wft042_del(esp_lcd_panel_t *panel) {
    wft042_panel_t *wft042 = __containerof(panel, wft042_panel_t, base);
    if (wft042->reset_gpio_num >= 0) gpio_reset_pin((gpio_num_t)wft042->reset_gpio_num);
    free(wft042);
    return ESP_OK;
}

static esp_err_t panel_wft042_reset(esp_lcd_panel_t *panel) {
    wft042_panel_t *wft042 = __containerof(panel, wft042_panel_t, base);
    if (wft042->reset_gpio_num >= 0) {
        gpio_set_level((gpio_num_t)wft042->reset_gpio_num, 0);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level((gpio_num_t)wft042->reset_gpio_num, 1);
        vTaskDelay(pdMS_TO_TICKS(10));
        _wait_busy(wft042);
    } else {
        esp_lcd_panel_io_tx_param(wft042->io, CMD_SW_RESET, NULL, 0);
        vTaskDelay(pdMS_TO_TICKS(10));
        _wait_busy(wft042);
    }
    return ESP_OK;
}

static esp_err_t panel_wft042_init(esp_lcd_panel_t *panel) {
    wft042_panel_t *wft042 = __containerof(panel, wft042_panel_t, base);
    esp_lcd_panel_io_handle_t io = wft042->io;
    for (int i = 0; i < sizeof(vendor_init_cmds) / sizeof(wft042_init_cmd_t); i++) {
        const wft042_init_cmd_t *p = &vendor_init_cmds[i];
        esp_lcd_panel_io_tx_param(io, p->cmd, p->data, p->data_bytes);
        if (p->delay_ms > 0) vTaskDelay(pdMS_TO_TICKS(p->delay_ms));
    }
    _wait_busy(wft042);
    return ESP_OK;
}

static esp_err_t panel_wft042_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start, int x_end, int y_end, const void *color_data) {
    wft042_panel_t *wft042 = __containerof(panel, wft042_panel_t, base);
    esp_lcd_panel_io_handle_t io = wft042->io;

    _wait_busy(wft042);

    uint8_t data_x[] = {(uint8_t)((x_start >> 3) & 0xFF), (uint8_t)(((x_end - 1) >> 3) & 0xFF)};
    esp_lcd_panel_io_tx_param(io, CMD_SET_RAM_X_ADDRESS_START_END, data_x, 2);

    uint8_t data_y[] = {(uint8_t)(y_start & 0xFF), (uint8_t)((y_start >> 8) & 0xFF), (uint8_t)((y_end - 1) & 0xFF), (uint8_t)(((y_end - 1) >> 8) & 0xFF)};
    esp_lcd_panel_io_tx_param(io, CMD_SET_RAM_Y_ADDRESS_START_END, data_y, 4);

    esp_lcd_panel_io_tx_param(io, CMD_SET_RAM_X_ADDRESS_COUNTER, (uint8_t[]){(uint8_t)((x_start >> 3) & 0xFF)}, 1);
    esp_lcd_panel_io_tx_param(io, CMD_SET_RAM_Y_ADDRESS_COUNTER, (uint8_t[]){(uint8_t)(y_start & 0xFF), (uint8_t)((y_start >> 8) & 0xFF)}, 2);

    size_t len = ((x_end - x_start) / 8) * (y_end - y_start);
    esp_lcd_panel_io_tx_color(io, CMD_WRITE_RAM, color_data, len);

    esp_lcd_panel_io_tx_param(io, CMD_DISPLAY_UPDATE_CONTROL_2, (uint8_t[]){0xF7}, 1); 
    esp_lcd_panel_io_tx_param(io, CMD_MASTER_ACTIVATION, NULL, 0);

    return ESP_OK;
}

static esp_err_t panel_wft042_invert_color(esp_lcd_panel_t *panel, bool invert_color_data) { return ESP_OK; }
static esp_err_t panel_wft042_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y) { return ESP_OK; }
static esp_err_t panel_wft042_swap_xy(esp_lcd_panel_t *panel, bool swap_axes) { return ESP_OK; }
static esp_err_t panel_wft042_disp_on_off(esp_lcd_panel_t *panel, bool on_off) { return ESP_OK; }