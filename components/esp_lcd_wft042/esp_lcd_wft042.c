/* esp_lcd_wft042.c */
#include "esp_lcd_wft042.h"
#include <stdlib.h>
#include <string.h>
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

// 指令集
#define CMD_DRIVER_OUTPUT           0x01
#define CMD_BOOSTER_SOFT_START      0x0C
#define CMD_SW_RESET                0x12
#define CMD_MASTER_ACTIVATION       0x20
#define CMD_UPDATE_CTRL_2           0x22
#define CMD_WRITE_RAM               0x24
#define CMD_VCOM_SENSE              0x28
#define CMD_VCOM_DURATION           0x29
#define CMD_WRITE_VCOM              0x2C
#define CMD_LUT_VCOM                0x20 // GxEPD 使用独立寄存器写 LUT
#define CMD_LUT_WW                  0x21
#define CMD_LUT_BW                  0x22
#define CMD_LUT_WB                  0x23
#define CMD_LUT_BB                  0x24
#define CMD_SET_RAM_X_START_END     0x44
#define CMD_SET_RAM_Y_START_END     0x45
#define CMD_SET_RAM_X_COUNTER       0x4E
#define CMD_SET_RAM_Y_COUNTER       0x4F

// ============================================================================
// LUT 数据 (源自 GxEPD2_420.cpp)
// ============================================================================

// --- 1. 局刷波形 (Partial) ---
// T1=25, T2=1, T3=2, T4=25
#define T1 25
#define T2  1
#define T3  2
#define T4 25

static const uint8_t lut_20_vcom0_partial[] = {
  0x00, T1, T2, T3, T4, 1, 0x00,  1,  0,  0,  0, 1
};
static const uint8_t lut_21_ww_partial[] = {
  0x18, T1, T2, T3, T4, 1, 0x00,  1,  0,  0,  0, 1
};
static const uint8_t lut_22_bw_partial[] = {
  0x5A, T1, T2, T3, T4, 1, 0x00,  1,  0,  0,  0, 1
};
static const uint8_t lut_23_wb_partial[] = {
  0xA5, T1, T2, T3, T4, 1, 0x00,  1,  0,  0,  0, 1
};
static const uint8_t lut_24_bb_partial[] = {
  0x24, T1, T2, T3, T4, 1, 0x00,  1,  0,  0,  0, 1
};

// --- 2. 灰度波形 (4-Gray) ---
static const uint8_t lut_20_vcom0_4G[] = {
  0x00, 0x0A, 0x00, 0x00, 0x00, 0x01, 0x60, 0x14, 0x14, 0x00, 0x00, 0x01,
  0x00, 0x14, 0x00, 0x00, 0x00, 0x01, 0x00, 0x13, 0x0A, 0x01, 0x00, 0x01,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
static const uint8_t lut_21_ww_4G[] = {
  0x40, 0x0A, 0x00, 0x00, 0x00, 0x01, 0x90, 0x14, 0x14, 0x00, 0x00, 0x01,
  0x10, 0x14, 0x0A, 0x00, 0x00, 0x01, 0xA0, 0x13, 0x01, 0x00, 0x00, 0x01,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static const uint8_t lut_22_bw_4G[] = {
  0x40, 0x0A, 0x00, 0x00, 0x00, 0x01, 0x90, 0x14, 0x14, 0x00, 0x00, 0x01,
  0x00, 0x14, 0x0A, 0x00, 0x00, 0x01, 0x99, 0x0C, 0x01, 0x03, 0x04, 0x01,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static const uint8_t lut_23_wb_4G[] = {
  0x40, 0x0A, 0x00, 0x00, 0x00, 0x01, 0x90, 0x14, 0x14, 0x00, 0x00, 0x01,
  0x00, 0x14, 0x0A, 0x00, 0x00, 0x01, 0x99, 0x0B, 0x04, 0x04, 0x01, 0x01,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static const uint8_t lut_24_bb_4G[] = {
  0x80, 0x0A, 0x00, 0x00, 0x00, 0x01, 0x90, 0x14, 0x14, 0x00, 0x00, 0x01,
  0x20, 0x14, 0x0A, 0x00, 0x00, 0x01, 0x50, 0x13, 0x01, 0x00, 0x00, 0x01,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

// ============================================================================
// 驱动结构体
// ============================================================================
typedef struct {
    esp_lcd_panel_t base;
    esp_lcd_panel_io_handle_t io;
    int reset_gpio_num;
    int busy_gpio_num;
    int busy_level;
    wft042_refresh_mode_t current_mode;
} wft042_panel_t;

// 辅助函数: 等待忙信号
static void _wait_busy(wft042_panel_t *panel) {
    if (panel->busy_gpio_num >= 0) {
        // 给一个比较长的超时，因为灰度刷新可能需要几秒
        int timeout = 5000; 
        while (gpio_get_level((gpio_num_t)panel->busy_gpio_num) == panel->busy_level) {
            vTaskDelay(pdMS_TO_TICKS(10));
            timeout -= 10;
            if (timeout <= 0) {
                ESP_LOGW(TAG, "Wait busy timeout");
                break;
            }
        }
    }
}

// 辅助函数: 写入一组 LUT
static void _inject_lut(esp_lcd_panel_io_handle_t io, 
                        const uint8_t* lut20, size_t len20,
                        const uint8_t* lut21, size_t len21,
                        const uint8_t* lut22, size_t len22,
                        const uint8_t* lut23, size_t len23,
                        const uint8_t* lut24, size_t len24) 
{
    esp_lcd_panel_io_tx_param(io, CMD_LUT_VCOM, lut20, len20);
    esp_lcd_panel_io_tx_param(io, CMD_LUT_WW, lut21, len21);
    esp_lcd_panel_io_tx_param(io, CMD_LUT_BW, lut22, len22);
    esp_lcd_panel_io_tx_param(io, CMD_LUT_WB, lut23, len23);
    esp_lcd_panel_io_tx_param(io, CMD_LUT_BB, lut24, len24);
}

// ============================================================================
// 接口函数实现
// ============================================================================

static esp_err_t panel_wft042_init(esp_lcd_panel_t *panel);

// 切换模式核心逻辑
esp_err_t esp_lcd_panel_wft042_set_mode(esp_lcd_panel_handle_t panel, wft042_refresh_mode_t mode)
{
    wft042_panel_t *wft042 = __containerof(panel, wft042_panel_t, base);
    esp_lcd_panel_io_handle_t io = wft042->io;

    if (wft042->current_mode == mode) return ESP_OK; // 模式未变

    _wait_busy(wft042);
    
    // 每次切换模式，GxEPD 建议重新初始化显示参数
    // 这里简化为重新发送基础配置 + 写入对应的 LUT
    
    // 1. 基础配置 (参考 _InitDisplay)
    // 0x01: Power Setting
    // 0x06: Booster
    // 0x00: Panel Setting
    // 0x30: PLL
    // 0x61: Res
    // 0x82: VCOM
    // 0x50: VCOM Interval
    // 为了稳健，我们直接调用 init 函数复位参数
    panel_wft042_init(panel); 

    // 2. 注入 LUT
    if (mode == WFT042_MODE_PARTIAL) {
        ESP_LOGI(TAG, "Switch to Partial Mode");
        _inject_lut(io,
            lut_20_vcom0_partial, sizeof(lut_20_vcom0_partial),
            lut_21_ww_partial, sizeof(lut_21_ww_partial),
            lut_22_bw_partial, sizeof(lut_22_bw_partial),
            lut_23_wb_partial, sizeof(lut_23_wb_partial),
            lut_24_bb_partial, sizeof(lut_24_bb_partial)
        );
    } else if (mode == WFT042_MODE_GRAY_4L) {
        ESP_LOGI(TAG, "Switch to 4-Gray Mode");
        _inject_lut(io,
            lut_20_vcom0_4G, sizeof(lut_20_vcom0_4G),
            lut_21_ww_4G, sizeof(lut_21_ww_4G),
            lut_22_bw_4G, sizeof(lut_22_bw_4G),
            lut_23_wb_4G, sizeof(lut_23_wb_4G),
            lut_24_bb_4G, sizeof(lut_24_bb_4G)
        );
    } else {
        ESP_LOGI(TAG, "Switch to Full Mode (OTP LUT)");
        // 全刷模式通常使用 OTP 里的默认 LUT，不需要手动注入
        // 只需要执行初始化和 Power On 即可
    }

    // 3. Power On (0x04)
    esp_lcd_panel_io_tx_param(io, 0x04, NULL, 0);
    _wait_busy(wft042);

    wft042->current_mode = mode;
    return ESP_OK;
}

// --------------------------------------------------------
// 标准 esp_lcd 接口实现
// --------------------------------------------------------

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

    // 标准初始化序列 (参考 GxEPD2 _InitDisplay)
    esp_lcd_panel_io_tx_param(io, CMD_SW_RESET, NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    _wait_busy(wft042);

    // 0x01: Power Setting
    esp_lcd_panel_io_tx_param(io, 0x01, (uint8_t[]){0x03, 0x00, 0x2B, 0x2B}, 4);
    // 0x06: Booster Soft Start
    esp_lcd_panel_io_tx_param(io, 0x06, (uint8_t[]){0x17, 0x17, 0x17}, 3);
    // 0x00: Panel Setting (300x400, BW Mode, LUT from Reg)
    // 注意: bit 5 = 1 (LUT from Register), bit 5 = 0 (LUT from OTP)
    // 我们默认从 Register 读，因为我们会注入 LUT
    esp_lcd_panel_io_tx_param(io, 0x00, (uint8_t[]){0x3F}, 1);
    // 0x30: PLL (100Hz)
    esp_lcd_panel_io_tx_param(io, 0x30, (uint8_t[]){0x3A}, 1);
    // 0x61: Resolution 400x300
    esp_lcd_panel_io_tx_param(io, 0x61, (uint8_t[]){0x01, 0x90, 0x01, 0x2C}, 4);
    // 0x82: VCOM DC
    esp_lcd_panel_io_tx_param(io, 0x82, (uint8_t[]){0x12}, 1);
    // 0x50: VCOM Interval
    esp_lcd_panel_io_tx_param(io, 0x50, (uint8_t[]){0xD7}, 1);

    return ESP_OK;
}

static esp_err_t panel_wft042_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start, int x_end, int y_end, const void *color_data) {
    wft042_panel_t *wft042 = __containerof(panel, wft042_panel_t, base);
    esp_lcd_panel_io_handle_t io = wft042->io;

    _wait_busy(wft042);

    // 设置坐标窗口
    uint8_t data_x[] = {(uint8_t)((x_start >> 3) & 0xFF), (uint8_t)(((x_end - 1) >> 3) & 0xFF)};
    esp_lcd_panel_io_tx_param(io, CMD_SET_RAM_X_START_END, data_x, 2);

    uint8_t data_y[] = {(uint8_t)(y_start & 0xFF), (uint8_t)((y_start >> 8) & 0xFF), (uint8_t)((y_end - 1) & 0xFF), (uint8_t)(((y_end - 1) >> 8) & 0xFF)};
    esp_lcd_panel_io_tx_param(io, CMD_SET_RAM_Y_START_END, data_y, 4);

    esp_lcd_panel_io_tx_param(io, CMD_SET_RAM_X_COUNTER, (uint8_t[]){(uint8_t)((x_start >> 3) & 0xFF)}, 1);
    esp_lcd_panel_io_tx_param(io, CMD_SET_RAM_Y_COUNTER, (uint8_t[]){(uint8_t)(y_start & 0xFF), (uint8_t)((y_start >> 8) & 0xFF)}, 2);

    // 发送数据
    size_t len = ((x_end - x_start) / 8) * (y_end - y_start);
    esp_lcd_panel_io_tx_color(io, CMD_WRITE_RAM, color_data, len);

    // 触发刷新
    // 0xCF = Load Temp, Load LUT, Display (用于局刷和灰度)
    // 0xF7 = 默认全刷 (如果使用 OTP)
    uint8_t update_param = (wft042->current_mode == WFT042_MODE_FULL) ? 0xF7 : 0xCF;
    
    esp_lcd_panel_io_tx_param(io, CMD_UPDATE_CTRL_2, &update_param, 1);
    esp_lcd_panel_io_tx_param(io, CMD_MASTER_ACTIVATION, NULL, 0);

    _wait_busy(wft042);
    return ESP_OK;
}

// 占位函数
static esp_err_t panel_wft042_invert_color(esp_lcd_panel_t *panel, bool invert_color_data) { return ESP_OK; }
static esp_err_t panel_wft042_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y) { return ESP_OK; }
static esp_err_t panel_wft042_swap_xy(esp_lcd_panel_t *panel, bool swap_axes) { return ESP_OK; }
static esp_err_t panel_wft042_disp_on_off(esp_lcd_panel_t *panel, bool on_off) { return ESP_OK; }

// 构造函数
esp_err_t esp_lcd_new_panel_wft042(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel)
{
    wft042_panel_t *wft042 = (wft042_panel_t *)calloc(1, sizeof(wft042_panel_t));
    if (!wft042) return ESP_ERR_NO_MEM;

    if (panel_dev_config->reset_gpio_num >= 0) {
        gpio_config_t io_conf = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << panel_dev_config->reset_gpio_num,
        };
        gpio_config(&io_conf);
    }

    wft042_vendor_config_t *vendor_cfg = (wft042_vendor_config_t *)panel_dev_config->vendor_config;
    if (vendor_cfg && vendor_cfg->busy_gpio_num >= 0) {
        wft042->busy_gpio_num = vendor_cfg->busy_gpio_num;
        wft042->busy_level = vendor_cfg->busy_level;
        gpio_config_t io_conf = {
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pin_bit_mask = 1ULL << wft042->busy_gpio_num,
        };
        gpio_config(&io_conf);
    } else {
        wft042->busy_gpio_num = -1;
    }

    wft042->io = io;
    wft042->reset_gpio_num = panel_dev_config->reset_gpio_num;
    wft042->current_mode = WFT042_MODE_FULL; // 默认全刷

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