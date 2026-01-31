/* esp_lcd_wft042.h */
#pragma once

#include <stdint.h>
#include "esp_lcd_panel_vendor.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

// 定义刷新模式
typedef enum {
    WFT042_MODE_FULL = 0,   // 全刷 (黑白, 闪烁, 清残影)
    WFT042_MODE_PARTIAL,    // 局刷 (黑白, 快速, 无闪烁)
    WFT042_MODE_GRAY_4L     // 灰度 (4级灰度, 较慢)
} wft042_refresh_mode_t;

// 初始化命令结构
typedef struct {
    int cmd;
    const void *data;
    size_t data_bytes;
    unsigned int delay_ms;
} wft042_init_cmd_t;

// Vendor 配置
typedef struct {
    const wft042_init_cmd_t *init_cmds;
    uint16_t init_cmds_size;
    int busy_gpio_num;      
    int busy_level;         
} wft042_vendor_config_t;

esp_err_t esp_lcd_new_panel_wft042(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel);

/**
 * @brief 切换屏幕刷新模式 (全刷 / 局刷 / 灰度)
 * @param panel 面板句柄
 * @param mode 模式枚举
 */
esp_err_t esp_lcd_panel_wft042_set_mode(esp_lcd_panel_handle_t panel, wft042_refresh_mode_t mode);

// 宏定义保持不变
#define WFT042_PANEL_BUS_SPI_CONFIG(sclk, mosi, max_trans_sz)   \
    {                                                           \
        .mosi_io_num = mosi,                                    \
        .miso_io_num = -1,                                      \
        .sclk_io_num = sclk,                                    \
        .quadwp_io_num = -1,                                    \
        .quadhd_io_num = -1,                                    \
        .max_transfer_sz = max_trans_sz,                        \
    }

#define WFT042_PANEL_IO_SPI_CONFIG(cs, dc, cb, cb_ctx)          \
    {                                                           \
        .cs_gpio_num = cs,                                      \
        .dc_gpio_num = dc,                                      \
        .spi_mode = 0,                                          \
        .pclk_hz = 5 * 1000 * 1000,                             \
        .trans_queue_depth = 10,                                \
        .on_color_trans_done = cb,                              \
        .user_ctx = cb_ctx,                                     \
        .lcd_cmd_bits = 8,                                      \
        .lcd_param_bits = 8,                                    \
    }

#ifdef __cplusplus
}
#endif