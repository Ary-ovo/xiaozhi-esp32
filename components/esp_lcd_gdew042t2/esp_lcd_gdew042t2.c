/*
 * SPDX-FileCopyrightText: 2024 Your Name
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>
#include <sys/cdefs.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "sdkconfig.h"
#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_commands.h"
#include "esp_log.h"

#include "esp_lcd_gdew042t2.h"

static const char *TAG = "gdew042t2";

// IL0398 / UC8176 Commands
#define IL0398_CMD_PANEL_SETTING      0x00
#define IL0398_CMD_POWER_SETTING      0x01
#define IL0398_CMD_POWER_OFF          0x02
#define IL0398_CMD_POWER_ON           0x04
#define IL0398_CMD_BOOSTER_SOFT_START 0x06
#define IL0398_CMD_DEEP_SLEEP         0x07
#define IL0398_CMD_DISPLAY_REFRESH    0x12
#define IL0398_CMD_DTM2               0x13 // Data Start Transmission 2 (New Data)
#define IL0398_CMD_LUT_VCOM           0x20
#define IL0398_CMD_LUT_WW             0x21
#define IL0398_CMD_LUT_BW             0x22
#define IL0398_CMD_LUT_WB             0x23
#define IL0398_CMD_LUT_BB             0x24
#define IL0398_CMD_PLL_CONTROL        0x30
#define IL0398_CMD_RESOLUTION         0x61
#define IL0398_CMD_VCOM_DC            0x82
#define IL0398_CMD_PARTIAL_WINDOW     0x90
#define IL0398_CMD_PARTIAL_IN         0x91
#define IL0398_CMD_PARTIAL_OUT        0x92

// --- Waveform Parameters (Extracted from GxEPD2_420.cpp) ---
// new waveform created by Jean-Marc Zingg for the actual panel
#define T1 25 // color change charge balance pre-phase
#define T2  1 // color change or sustain charge balance pre-phase
#define T3  4 // color change or sustain phase (GxEPD2 uses 4, previous was 2)
#define T4 25 // color change phase

// --- Full Refresh LUTs (Copy from GxEPD2_420.cpp) ---
static const uint8_t lut_20_vcom0_full[] = {
    0x00, 0x08, 0x00, 0x00, 0x00, 0x02,
    0x60, 0x28, 0x28, 0x00, 0x00, 0x01,
    0x00, 0x14, 0x00, 0x00, 0x00, 0x01,
    0x00, 0x12, 0x12, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t lut_21_ww_full[] = {
    0x40, 0x08, 0x00, 0x00, 0x00, 0x02,
    0x90, 0x28, 0x28, 0x00, 0x00, 0x01,
    0x40, 0x14, 0x00, 0x00, 0x00, 0x01,
    0xA0, 0x12, 0x12, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t lut_22_bw_full[] = {
    0x40, 0x08, 0x00, 0x00, 0x00, 0x02,
    0x90, 0x28, 0x28, 0x00, 0x00, 0x01,
    0x40, 0x14, 0x00, 0x00, 0x00, 0x01,
    0xA0, 0x12, 0x12, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t lut_23_wb_full[] = {
    0x80, 0x08, 0x00, 0x00, 0x00, 0x02,
    0x90, 0x28, 0x28, 0x00, 0x00, 0x01,
    0x80, 0x14, 0x00, 0x00, 0x00, 0x01,
    0x50, 0x12, 0x12, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t lut_24_bb_full[] = {
    0x80, 0x08, 0x00, 0x00, 0x00, 0x02,
    0x90, 0x28, 0x28, 0x00, 0x00, 0x01,
    0x80, 0x14, 0x00, 0x00, 0x00, 0x01,
    0x50, 0x12, 0x12, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// --- Partial Refresh LUTs (Copy from GxEPD2_420.cpp) ---
// Padded with 0x00 to match register length requirement (44 for VCOM, 42 for others)
static const uint8_t lut_20_vcom0_partial[] = {
    0x00, T1, T2, T3, T4, 1, // 00 00 00 00
    0x00,  1,  0,  0,  0, 1, // gnd phase
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Padding
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t lut_21_ww_partial[] = {
    0x18, T1, T2, T3, T4, 1, // 00 01 10 00
    0x00,  1,  0,  0,  0, 1, // gnd phase
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t lut_22_bw_partial[] = {
    0x5A, T1, T2, T3, T4, 1, // 01 01 10 10
    0x00,  1,  0,  0,  0, 1, // gnd phase
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t lut_23_wb_partial[] = {
    0xA5, T1, T2, T3, T4, 1, // 10 10 01 01
    0x00,  1,  0,  0,  0, 1, // gnd phase
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t lut_24_bb_partial[] = {
    0x24, T1, T2, T3, T4, 1, // 00 10 01 00
    0x00,  1,  0,  0,  0, 1, // gnd phase
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

typedef struct {
    esp_lcd_panel_t base;
    esp_lcd_panel_io_handle_t io;
    int reset_gpio_num;
    int busy_gpio_num;
    bool busy_active_level; 
    bool reset_level;
} gdew042t2_panel_t;

static esp_err_t panel_gdew042t2_del(esp_lcd_panel_t *panel);
static esp_err_t panel_gdew042t2_reset(esp_lcd_panel_t *panel);
static esp_err_t panel_gdew042t2_init(esp_lcd_panel_t *panel);
static esp_err_t panel_gdew042t2_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start, int x_end, int y_end, const void *color_data);
static esp_err_t panel_gdew042t2_invert_color(esp_lcd_panel_t *panel, bool invert_color_data);
static esp_err_t panel_gdew042t2_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y);
static esp_err_t panel_gdew042t2_disp_on_off(esp_lcd_panel_t *panel, bool off);

static void _wait_busy(gdew042t2_panel_t *epd)
{
    if (epd->busy_gpio_num >= 0) {
        int timeout_ms = 5000; 
        while (gpio_get_level(epd->busy_gpio_num) == epd->busy_active_level) {
            vTaskDelay(pdMS_TO_TICKS(10));
            timeout_ms -= 10;
            if(timeout_ms <= 0) {
                ESP_LOGW(TAG, "Wait busy timeout");
                break;
            }
        }
    } else {
        vTaskDelay(pdMS_TO_TICKS(500)); 
    }
}

static esp_err_t panel_gdew042t2_reset(esp_lcd_panel_t *panel)
{
    gdew042t2_panel_t *epd = __containerof(panel, gdew042t2_panel_t, base);
    
    if (epd->reset_gpio_num >= 0) {
        gpio_set_level(epd->reset_gpio_num, 0);
        vTaskDelay(pdMS_TO_TICKS(20));
        gpio_set_level(epd->reset_gpio_num, 1);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    
    return ESP_OK;
}

static esp_err_t panel_gdew042t2_init(esp_lcd_panel_t *panel)
{
    gdew042t2_panel_t *epd = __containerof(panel, gdew042t2_panel_t, base);
    esp_lcd_panel_io_handle_t io = epd->io;

    _wait_busy(epd);

    // Initialization Sequence (Matches GxEPD2_420.cpp _InitDisplay)
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, IL0398_CMD_POWER_SETTING, (uint8_t[]){0x03, 0x00, 0x2b, 0x2b}, 4), TAG, "Power Set failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, IL0398_CMD_BOOSTER_SOFT_START, (uint8_t[]){0x17, 0x17, 0x17}, 3), TAG, "Booster failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, IL0398_CMD_PANEL_SETTING, (uint8_t[]){0x3f}, 1), TAG, "Panel Set failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, IL0398_CMD_PLL_CONTROL, (uint8_t[]){0x3a}, 1), TAG, "PLL failed");

    uint8_t res_param[] = {
        (ESP_LCD_GDEW042T2_WIDTH >> 8) & 0xFF,
        ESP_LCD_GDEW042T2_WIDTH & 0xFF,
        (ESP_LCD_GDEW042T2_HEIGHT >> 8) & 0xFF,
        ESP_LCD_GDEW042T2_HEIGHT & 0xFF
    };
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, IL0398_CMD_RESOLUTION, res_param, 4), TAG, "Res Set failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, IL0398_CMD_VCOM_DC, (uint8_t[]){0x12}, 1), TAG, "Vcom DC failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0x50, (uint8_t[]){0xd7}, 1), TAG, "Vcom Interval failed");

    // Default to Full Refresh LUTs
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, IL0398_CMD_LUT_VCOM, lut_20_vcom0_full, sizeof(lut_20_vcom0_full)), TAG, "LUT20 failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, IL0398_CMD_LUT_WW, lut_21_ww_full, sizeof(lut_21_ww_full)), TAG, "LUT21 failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, IL0398_CMD_LUT_BW, lut_22_bw_full, sizeof(lut_22_bw_full)), TAG, "LUT22 failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, IL0398_CMD_LUT_WB, lut_23_wb_full, sizeof(lut_23_wb_full)), TAG, "LUT23 failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, IL0398_CMD_LUT_BB, lut_24_bb_full, sizeof(lut_24_bb_full)), TAG, "LUT24 failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, IL0398_CMD_POWER_ON, NULL, 0), TAG, "Power ON failed");
    _wait_busy(epd);

    ESP_LOGI(TAG, "GDEW042T2 Init Done (Full Refresh Mode)");
    return ESP_OK;
}

static esp_err_t panel_gdew042t2_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start, int x_end, int y_end, const void *color_data)
{
    gdew042t2_panel_t *epd = __containerof(panel, gdew042t2_panel_t, base);
    esp_lcd_panel_io_handle_t io = epd->io;

    size_t len = (x_end - x_start) * (y_end - y_start) / 8; // 1bpp
    if (len == 0) len = 1;

    // Send New Data (0x13)
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_color(io, IL0398_CMD_DTM2, color_data, len), TAG, "Tx Data failed");

    // Display Refresh (0x12)
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, IL0398_CMD_DISPLAY_REFRESH, NULL, 0), TAG, "Refresh cmd failed");

    _wait_busy(epd);

    return ESP_OK;
}

static esp_err_t panel_gdew042t2_invert_color(esp_lcd_panel_t *panel, bool invert_color_data)
{
    return ESP_ERR_NOT_SUPPORTED;
}

static esp_err_t panel_gdew042t2_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y)
{
    return ESP_ERR_NOT_SUPPORTED;
}

static esp_err_t panel_gdew042t2_disp_on_off(esp_lcd_panel_t *panel, bool off)
{
    gdew042t2_panel_t *epd = __containerof(panel, gdew042t2_panel_t, base);
    if (off) {
        esp_lcd_panel_io_tx_param(epd->io, IL0398_CMD_POWER_OFF, NULL, 0);
        _wait_busy(epd);
        esp_lcd_panel_io_tx_param(epd->io, IL0398_CMD_DEEP_SLEEP, (uint8_t[]){0xA5}, 1);
    } else {
        panel_gdew042t2_reset(panel);
        panel_gdew042t2_init(panel);
    }
    return ESP_OK;
}

static esp_err_t panel_gdew042t2_del(esp_lcd_panel_t *panel)
{
    gdew042t2_panel_t *epd = __containerof(panel, gdew042t2_panel_t, base);
    if (epd->reset_gpio_num >= 0) {
        gpio_reset_pin(epd->reset_gpio_num);
    }
    if (epd->busy_gpio_num >= 0) {
        gpio_reset_pin(epd->busy_gpio_num);
    }
    free(epd);
    return ESP_OK;
}

esp_err_t esp_lcd_new_panel_gdew042t2(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel)
{
    if (!io || !panel_dev_config || !ret_panel) {
        return ESP_ERR_INVALID_ARG;
    }

    gdew042t2_panel_t *epd = calloc(1, sizeof(gdew042t2_panel_t));
    if (!epd) {
        return ESP_ERR_NO_MEM;
    }

    epd->io = io;
    epd->reset_gpio_num = panel_dev_config->reset_gpio_num;
    epd->reset_level = panel_dev_config->flags.reset_active_high;
    epd->busy_gpio_num = -1;
    epd->busy_active_level = 1; 
    
    if (panel_dev_config->vendor_config) {
        esp_lcd_panel_gdew042t2_config_t *config = (esp_lcd_panel_gdew042t2_config_t *)panel_dev_config->vendor_config;
        epd->busy_gpio_num = config->busy_gpio_num;
        epd->busy_active_level = config->busy_active_level;
    }

    if (epd->reset_gpio_num >= 0) {
        gpio_config_t io_conf = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << epd->reset_gpio_num,
        };
        gpio_config(&io_conf);
    }

    if (epd->busy_gpio_num >= 0) {
        gpio_config_t io_conf = {
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE, 
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .pin_bit_mask = 1ULL << epd->busy_gpio_num,
        };
        gpio_config(&io_conf);
    }

    epd->base.del = panel_gdew042t2_del;
    epd->base.reset = panel_gdew042t2_reset;
    epd->base.init = panel_gdew042t2_init;
    epd->base.draw_bitmap = panel_gdew042t2_draw_bitmap;
    epd->base.invert_color = panel_gdew042t2_invert_color;
    epd->base.mirror = panel_gdew042t2_mirror;
    epd->base.disp_on_off = panel_gdew042t2_disp_on_off;

    *ret_panel = &(epd->base);
    ESP_LOGD(TAG, "new gdew042t2 panel @%p", epd);

    return ESP_OK;
}

// --- Set Refresh Mode (Switch LUTs) ---
esp_err_t esp_lcd_gdew042t2_set_mode(esp_lcd_panel_handle_t panel, gdew042t2_refresh_mode_t mode)
{
    gdew042t2_panel_t *epd = __containerof(panel, gdew042t2_panel_t, base);
    esp_lcd_panel_io_handle_t io = epd->io;
    
    _wait_busy(epd);

    if (mode == GDEW042T2_REFRESH_FULL) {
        ESP_LOGI(TAG, "Mode: Full Refresh");
        esp_lcd_panel_io_tx_param(io, IL0398_CMD_LUT_VCOM, lut_20_vcom0_full, sizeof(lut_20_vcom0_full));
        esp_lcd_panel_io_tx_param(io, IL0398_CMD_LUT_WW, lut_21_ww_full, sizeof(lut_21_ww_full));
        esp_lcd_panel_io_tx_param(io, IL0398_CMD_LUT_BW, lut_22_bw_full, sizeof(lut_22_bw_full));
        esp_lcd_panel_io_tx_param(io, IL0398_CMD_LUT_WB, lut_23_wb_full, sizeof(lut_23_wb_full));
        esp_lcd_panel_io_tx_param(io, IL0398_CMD_LUT_BB, lut_24_bb_full, sizeof(lut_24_bb_full));
    } else {
        ESP_LOGI(TAG, "Mode: Partial Refresh");
        // Partial refresh LUTs are padded to match register length: 44 bytes for VCOM, 42 for others
        esp_lcd_panel_io_tx_param(io, IL0398_CMD_LUT_VCOM, lut_20_vcom0_partial, 44);
        esp_lcd_panel_io_tx_param(io, IL0398_CMD_LUT_WW, lut_21_ww_partial, 42);
        esp_lcd_panel_io_tx_param(io, IL0398_CMD_LUT_BW, lut_22_bw_partial, 42);
        esp_lcd_panel_io_tx_param(io, IL0398_CMD_LUT_WB, lut_23_wb_partial, 42);
        esp_lcd_panel_io_tx_param(io, IL0398_CMD_LUT_BB, lut_24_bb_partial, 42);
    }
    
    // GxEPD2 calls PowerOn after switching LUTs (implied in _Init_Part/_Init_Full)
    esp_lcd_panel_io_tx_param(io, IL0398_CMD_POWER_ON, NULL, 0); 
    _wait_busy(epd);

    return ESP_OK;
}

// --- Partial Window Refresh ---
esp_err_t esp_lcd_gdew042t2_draw_partial(esp_lcd_panel_handle_t panel, int x, int y, int w, int h, const void *color_data)
{
    gdew042t2_panel_t *epd = __containerof(panel, gdew042t2_panel_t, base);
    esp_lcd_panel_io_handle_t io = epd->io;

    // 1. Check Alignment (x and w must be multiples of 8)
    if ((x % 8 != 0) || (w % 8 != 0)) {
        ESP_LOGE(TAG, "Partial update X and Width must be multiples of 8!");
        return ESP_ERR_INVALID_ARG;
    }

    // 2. Partial In (0x91)
    esp_lcd_panel_io_tx_param(io, IL0398_CMD_PARTIAL_IN, NULL, 0);

    // 3. Set Partial Window (0x90)
    // Parameter Format: XStart(2), XEnd(2), YStart(2), YEnd(2), 0x01
    // Logic from GxEPD2:
    uint16_t x_start = x;
    uint16_t x_end = x + w - 1;
    uint16_t y_start = y;
    uint16_t y_end = y + h - 1;

    // Alignment correction
    x_start &= 0xFFF8;
    x_end |= 0x0007;

    uint8_t window_data[9] = {
        x_start >> 8, x_start & 0xFF,
        x_end >> 8, x_end & 0xFF,
        y_start >> 8, y_start & 0xFF,
        y_end >> 8, y_end & 0xFF,
        0x01 // PT_SCAN
    };
    esp_lcd_panel_io_tx_param(io, IL0398_CMD_PARTIAL_WINDOW, window_data, 9);

    // 4. Send Image Data (0x13 New Data)
    size_t len = w * h / 8;
    esp_lcd_panel_io_tx_color(io, IL0398_CMD_DTM2, color_data, len);

    // 5. Refresh (0x12)
    esp_lcd_panel_io_tx_param(io, IL0398_CMD_DISPLAY_REFRESH, NULL, 0);
    _wait_busy(epd);

    // 6. Partial Out (0x92)
    esp_lcd_panel_io_tx_param(io, IL0398_CMD_PARTIAL_OUT, NULL, 0);

    return ESP_OK;
}