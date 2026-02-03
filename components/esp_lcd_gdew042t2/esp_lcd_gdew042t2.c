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
#define IL0398_CMD_POWER_OFF_SEQ      0x03
#define IL0398_CMD_POWER_ON           0x04
#define IL0398_CMD_BOOSTER_SOFT_START 0x06
#define IL0398_CMD_DEEP_SLEEP         0x07
#define IL0398_CMD_DTM1               0x10 // Data Start Transmission 1 (Old Data)
#define IL0398_CMD_DISPLAY_REFRESH    0x12
#define IL0398_CMD_DTM2               0x13 // Data Start Transmission 2 (New Data)
#define IL0398_CMD_LUT_VCOM           0x20
#define IL0398_CMD_LUT_WW             0x21
#define IL0398_CMD_LUT_BW             0x22
#define IL0398_CMD_LUT_WB             0x23
#define IL0398_CMD_LUT_BB             0x24
#define IL0398_CMD_PLL_CONTROL        0x30
#define IL0398_CMD_TCON_SETTING       0x60
#define IL0398_CMD_RESOLUTION         0x61
#define IL0398_CMD_VCOM_DC            0x82
#define IL0398_CMD_PARTIAL_WINDOW     0x90
#define IL0398_CMD_PARTIAL_IN         0x91
#define IL0398_CMD_PARTIAL_OUT        0x92

// --- Added: Partial Refresh LUT (Waveform) ---
// Extracted from GxEPD2_420.cpp
#define T1 25
#define T2  1
#define T3  2
#define T4 25

// Waveform LUTs extracted from GxEPD2_420.cpp for GDEW042T2
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

// --- Added: 4-level Grayscale LUT (Source: GxEPD2_420.cpp) ---
static const uint8_t lut_20_vcom0_4G[] = {
    0x00, 0x0A, 0x00, 0x00, 0x00, 0x01,
    0x60, 0x14, 0x14, 0x00, 0x00, 0x01,
    0x00, 0x14, 0x00, 0x00, 0x00, 0x01,
    0x00, 0x13, 0x0A, 0x01, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
static const uint8_t lut_21_ww_4G[] = {
    0x40, 0x0A, 0x00, 0x00, 0x00, 0x01,
    0x90, 0x14, 0x14, 0x00, 0x00, 0x01,
    0x10, 0x14, 0x0A, 0x00, 0x00, 0x01,
    0xA0, 0x13, 0x01, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
static const uint8_t lut_22_bw_4G[] = {
    0x40, 0x0A, 0x00, 0x00, 0x00, 0x01,
    0x90, 0x14, 0x14, 0x00, 0x00, 0x01,
    0x00, 0x14, 0x0A, 0x00, 0x00, 0x01,
    0x99, 0x0C, 0x01, 0x03, 0x04, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
static const uint8_t lut_23_wb_4G[] = {
    0x40, 0x0A, 0x00, 0x00, 0x00, 0x01,
    0x90, 0x14, 0x14, 0x00, 0x00, 0x01,
    0x00, 0x14, 0x0A, 0x00, 0x00, 0x01,
    0x99, 0x0B, 0x04, 0x04, 0x01, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
static const uint8_t lut_24_bb_4G[] = {
    0x80, 0x0A, 0x00, 0x00, 0x00, 0x01,
    0x90, 0x14, 0x14, 0x00, 0x00, 0x01,
    0x20, 0x14, 0x0A, 0x00, 0x00, 0x01,
    0x50, 0x13, 0x01, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t lut_20_vcom0_partial[] = {
    0x00, T1, T2, T3, T4, 1, // 00 00 00 00
    0x00,  1,  0,  0,  0, 1, // gnd phase
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Pad to 44 bytes
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

// Wait for the BUSY pin to release
static void _wait_busy(gdew042t2_panel_t *epd)
{
    if (epd->busy_gpio_num >= 0) {
        // Timeout mechanism could be added here
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
        // Fallback delay if no busy pin is configured (not recommended for EPD)
        vTaskDelay(pdMS_TO_TICKS(500)); 
    }
}

static esp_err_t panel_gdew042t2_reset(esp_lcd_panel_t *panel)
{
    gdew042t2_panel_t *epd = __containerof(panel, gdew042t2_panel_t, base);
    
    // Hardware Reset
    if (epd->reset_gpio_num >= 0) {
        gpio_set_level(epd->reset_gpio_num, epd->reset_level);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(epd->reset_gpio_num, !epd->reset_level);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(epd->reset_gpio_num, epd->reset_level); // Keep active? No, usually RST is active low
        // Correct reset sequence: Pull Low -> Delay -> Pull High
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

    // 1. Power Setting
    // VDS_EN, VDG_EN, VCOM_HV, VGHL_LV, VDH, VDL parameters from GxEPD2
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, IL0398_CMD_POWER_SETTING, (uint8_t[]){0x03, 0x00, 0x2b, 0x2b}, 4), TAG, "Power Set failed");

    // 2. Booster Soft Start
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, IL0398_CMD_BOOSTER_SOFT_START, (uint8_t[]){0x17, 0x17, 0x17}, 3), TAG, "Booster failed");

    // 3. Panel Setting
    // 0x3F: BW mode, LUT from register
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, IL0398_CMD_PANEL_SETTING, (uint8_t[]){0x3f}, 1), TAG, "Panel Set failed");

    // 4. PLL Control
    // 0x3A: 100Hz
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, IL0398_CMD_PLL_CONTROL, (uint8_t[]){0x3a}, 1), TAG, "PLL failed");

    // 5. Resolution Setting
    // 400x300
    uint8_t res_param[] = {
        (ESP_LCD_GDEW042T2_WIDTH >> 8) & 0xFF,
        ESP_LCD_GDEW042T2_WIDTH & 0xFF,
        (ESP_LCD_GDEW042T2_HEIGHT >> 8) & 0xFF,
        ESP_LCD_GDEW042T2_HEIGHT & 0xFF
    };
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, IL0398_CMD_RESOLUTION, res_param, 4), TAG, "Res Set failed");

    // 6. VCOM DC Setting
    // -1.0V
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, IL0398_CMD_VCOM_DC, (uint8_t[]){0x12}, 1), TAG, "Vcom DC failed");

    // 7. VCOM and Data Interval
    // Border floating
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0x50, (uint8_t[]){0xd7}, 1), TAG, "Vcom Interval failed");

    // 8. Load LUTs
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, IL0398_CMD_LUT_VCOM, lut_20_vcom0_full, sizeof(lut_20_vcom0_full)), TAG, "LUT20 failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, IL0398_CMD_LUT_WW, lut_21_ww_full, sizeof(lut_21_ww_full)), TAG, "LUT21 failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, IL0398_CMD_LUT_BW, lut_22_bw_full, sizeof(lut_22_bw_full)), TAG, "LUT22 failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, IL0398_CMD_LUT_WB, lut_23_wb_full, sizeof(lut_23_wb_full)), TAG, "LUT23 failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, IL0398_CMD_LUT_BB, lut_24_bb_full, sizeof(lut_24_bb_full)), TAG, "LUT24 failed");

    // 9. Power ON
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, IL0398_CMD_POWER_ON, NULL, 0), TAG, "Power ON failed");
    _wait_busy(epd);

    ESP_LOGI(TAG, "GDEW042T2 Init Done");
    return ESP_OK;
}

static esp_err_t panel_gdew042t2_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start, int x_end, int y_end, const void *color_data)
{
    gdew042t2_panel_t *epd = __containerof(panel, gdew042t2_panel_t, base);
    esp_lcd_panel_io_handle_t io = epd->io;

    // EPDs typically require byte-aligned coordinates.
    // 400x300, 1 bit per pixel.
    
    // Note: The esp_lcd generic driver usually sends partial updates.
    // For EPD, a full refresh is often required for good quality unless partial mode is strictly set.
    // This implementation assumes 'color_data' contains the full buffer or the partial buffer
    // formatted correctly for the controller.
    
    // 1. Set RAM X/Y start/end not strictly needed if we assume full frame buffer writes,
    // but good practice if partials supported. IL0398 usually handles auto-increment.
    
    // 2. Write Data to RAM (Using 0x13 for NEW data)
    size_t len = (x_end - x_start) * (y_end - y_start) / 8; // 1bpp
    if (len == 0) len = 1; // Safety

    // Optional: Write 0x10 (Old Data) for differential update.
    // For simple refresh, we often just write 0x13.
    
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_color(io, IL0398_CMD_DTM2, color_data, len), TAG, "Tx Data failed");

    // 3. Display Refresh
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, IL0398_CMD_DISPLAY_REFRESH, NULL, 0), TAG, "Refresh cmd failed");

    // 4. Wait for update to complete
    _wait_busy(epd);

    return ESP_OK;
}

static esp_err_t panel_gdew042t2_invert_color(esp_lcd_panel_t *panel, bool invert_color_data)
{
    // EPD controllers generally don't have a hardware invert command like TFTs (0x20/0x21).
    // Inversion usually happens in software buffer.
    return ESP_ERR_NOT_SUPPORTED;
}

static esp_err_t panel_gdew042t2_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y)
{
    // IL0398 supports scanning direction via 0x00 (Panel Setting) or 0x16.
    // However, GxEPD2 hardcodes scanning. Implementing this requires changing init sequence.
    return ESP_ERR_NOT_SUPPORTED;
}

static esp_err_t panel_gdew042t2_disp_on_off(esp_lcd_panel_t *panel, bool off)
{
    gdew042t2_panel_t *epd = __containerof(panel, gdew042t2_panel_t, base);
    if (off) {
        // Power Off
        esp_lcd_panel_io_tx_param(epd->io, IL0398_CMD_POWER_OFF, NULL, 0);
        _wait_busy(epd);
        // Deep Sleep
        esp_lcd_panel_io_tx_param(epd->io, IL0398_CMD_DEEP_SLEEP, (uint8_t[]){0xA5}, 1);
    } else {
        // Wake up logic usually requires Reset
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
    
    // Process Vendor Config for Busy Pin
    epd->busy_gpio_num = -1;
    epd->busy_active_level = 1; // Default High = Busy
    
    if (panel_dev_config->vendor_config) {
        esp_lcd_panel_gdew042t2_config_t *config = (esp_lcd_panel_gdew042t2_config_t *)panel_dev_config->vendor_config;
        epd->busy_gpio_num = config->busy_gpio_num;
        epd->busy_active_level = config->busy_active_level;
    }

    // GPIO Configuration
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
            .pull_up_en = GPIO_PULLUP_DISABLE, // Busy usually driven by EPD
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

// Implementation: Grayscale Initialization
esp_err_t esp_lcd_gdew042t2_init_grayscale(esp_lcd_panel_handle_t panel)
{
    gdew042t2_panel_t *epd = __containerof(panel, gdew042t2_panel_t, base);
    esp_lcd_panel_io_handle_t io = epd->io;

    _wait_busy(epd);
    
    esp_lcd_panel_io_tx_param(io, 0x01, (uint8_t[]){0x03, 0x00, 0x2b, 0x2b}, 4); // Power
    esp_lcd_panel_io_tx_param(io, 0x06, (uint8_t[]){0x17, 0x17, 0x17}, 3);       // Booster
    esp_lcd_panel_io_tx_param(io, 0x00, (uint8_t[]){0x3f}, 1);                   // Panel
    esp_lcd_panel_io_tx_param(io, 0x30, (uint8_t[]){0x3a}, 1);                   // PLL
    uint8_t res[] = {(400>>8)&0xFF, 400&0xFF, (300>>8)&0xFF, 300&0xFF};
    esp_lcd_panel_io_tx_param(io, 0x61, res, 4);                                 // Res
    esp_lcd_panel_io_tx_param(io, 0x82, (uint8_t[]){0x12}, 1);                   // Vcom DC
    esp_lcd_panel_io_tx_param(io, 0x50, (uint8_t[]){0xd7}, 1);                   // Interval

    // --- Key step: Load 4-Gray Level LUT ---
    esp_lcd_panel_io_tx_param(io, 0x20, lut_20_vcom0_4G, sizeof(lut_20_vcom0_4G));
    esp_lcd_panel_io_tx_param(io, 0x21, lut_21_ww_4G, sizeof(lut_21_ww_4G));
    esp_lcd_panel_io_tx_param(io, 0x22, lut_22_bw_4G, sizeof(lut_22_bw_4G));
    esp_lcd_panel_io_tx_param(io, 0x23, lut_23_wb_4G, sizeof(lut_23_wb_4G));
    esp_lcd_panel_io_tx_param(io, 0x24, lut_24_bb_4G, sizeof(lut_24_bb_4G));

    // Power ON
    esp_lcd_panel_io_tx_param(io, 0x04, NULL, 0);
    _wait_busy(epd);
    
    return ESP_OK;
}

// Implementation: Grayscale Drawing
esp_err_t esp_lcd_gdew042t2_draw_grayscale(esp_lcd_panel_handle_t panel, const void *layer_0x10, const void *layer_0x13, size_t len)
{
    gdew042t2_panel_t *epd = __containerof(panel, gdew042t2_panel_t, base);
    esp_lcd_panel_io_handle_t io = epd->io;

    // 1. Write Old Data (0x10)
    if (layer_0x10) {
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_color(io, 0x10, layer_0x10, len), TAG, "Tx 0x10 failed");
    }

    // 2. Write New Data (0x13)
    if (layer_0x13) {
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_color(io, 0x13, layer_0x13, len), TAG, "Tx 0x13 failed");
    }

    // 3. Refresh (0x12)
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(io, 0x12, NULL, 0), TAG, "Refresh failed");

    // 4. Wait for completion
    _wait_busy(epd);

    return ESP_OK;
}

// --- Implementation: Set Refresh Mode ---
esp_err_t esp_lcd_gdew042t2_set_mode(esp_lcd_panel_handle_t panel, gdew042t2_refresh_mode_t mode)
{
    gdew042t2_panel_t *epd = __containerof(panel, gdew042t2_panel_t, base);
    esp_lcd_panel_io_handle_t io = epd->io;
    
    _wait_busy(epd);

    if (mode == GDEW042T2_REFRESH_FULL) {
        // Load Full Refresh LUTs (Longer sequence, same as init)
        ESP_LOGI(TAG, "Loading Full Refresh LUTs");
        esp_lcd_panel_io_tx_param(io, 0x20, lut_20_vcom0_full, sizeof(lut_20_vcom0_full));
        esp_lcd_panel_io_tx_param(io, 0x21, lut_21_ww_full, sizeof(lut_21_ww_full));
        esp_lcd_panel_io_tx_param(io, 0x22, lut_22_bw_full, sizeof(lut_22_bw_full));
        esp_lcd_panel_io_tx_param(io, 0x23, lut_23_wb_full, sizeof(lut_23_wb_full));
        esp_lcd_panel_io_tx_param(io, 0x24, lut_24_bb_full, sizeof(lut_24_bb_full));
    } else {
        // Load Partial Refresh LUTs (Shorter sequence, only 42-44 bytes valid, zero-padded)
        ESP_LOGI(TAG, "Loading Partial Refresh LUTs");
        esp_lcd_panel_io_tx_param(io, 0x20, lut_20_vcom0_partial, 44);
        esp_lcd_panel_io_tx_param(io, 0x21, lut_21_ww_partial, 42);
        esp_lcd_panel_io_tx_param(io, 0x22, lut_22_bw_partial, 42);
        esp_lcd_panel_io_tx_param(io, 0x23, lut_23_wb_partial, 42);
        esp_lcd_panel_io_tx_param(io, 0x24, lut_24_bb_partial, 42);
    }
    
    // Power On is recommended after switching LUTs to ensure they take effect (required by some ICs)
    esp_lcd_panel_io_tx_param(io, 0x04, NULL, 0); 
    _wait_busy(epd);

    return ESP_OK;
}

// --- Implementation: Partial Refresh Drawing ---
esp_err_t esp_lcd_gdew042t2_draw_partial(esp_lcd_panel_handle_t panel, int x, int y, int w, int h, const void *color_data)
{
    gdew042t2_panel_t *epd = __containerof(panel, gdew042t2_panel_t, base);
    esp_lcd_panel_io_handle_t io = epd->io;

    // 1. Coordinate alignment check (x and w must be multiples of 8)
    if ((x % 8 != 0) || (w % 8 != 0)) {
        ESP_LOGE(TAG, "Partial update X and Width must be multiples of 8!");
        return ESP_ERR_INVALID_ARG;
    }

    // 2. Enable Partial Mode (0x91)
    esp_lcd_panel_io_tx_param(io, 0x91, NULL, 0);

    // 3. Set Partial Window (0x90)
    // Parameter Format: XStart(2), XEnd(2), YStart(2), YEnd(2), 1 (Scan Mode)
    // Is X sent by byte address (X / 8)? No, IL0398 manual typically uses pixel coordinates, but low 3 bits are ignored.
    // Logic from GxEPD2:
    uint16_t x_start = x;
    uint16_t x_end = x + w - 1;
    uint16_t y_start = y;
    uint16_t y_end = y + h - 1;

    // Correction: Ensure End coordinate is also a byte-aligned boundary.
    x_start &= 0xFFF8;
    x_end |= 0x0007;

    uint8_t window_data[9] = {
        x_start >> 8, x_start & 0xFF,
        x_end >> 8, x_end & 0xFF,
        y_start >> 8, y_start & 0xFF,
        y_end >> 8, y_end & 0xFF,
        0x01 // PT_SCAN
    };
    esp_lcd_panel_io_tx_param(io, 0x90, window_data, 9);

    // 4. Send image data (0x13 New Data)
    size_t len = w * h / 8;
    esp_lcd_panel_io_tx_color(io, 0x13, color_data, len);

    // 5. Refresh (0x12)
    esp_lcd_panel_io_tx_param(io, 0x12, NULL, 0);
    _wait_busy(epd);

    // 6. Disable Partial Mode (0x92)
    esp_lcd_panel_io_tx_param(io, 0x92, NULL, 0);

    return ESP_OK;
}