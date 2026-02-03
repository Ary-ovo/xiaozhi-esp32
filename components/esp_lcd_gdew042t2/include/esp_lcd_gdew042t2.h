/*
 * SPDX-FileCopyrightText: 2024 Your Name
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "esp_lcd_panel_vendor.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Display resolution definitions
#define ESP_LCD_GDEW042T2_WIDTH  400
#define ESP_LCD_GDEW042T2_HEIGHT 300

/**
 * @brief GDEW042T2 (IL0398) Vendor Configuration
 *
 * This structure is used to pass EPD-specific hardware pin configurations
 * via the `vendor_config` field in `esp_lcd_panel_dev_config_t`.
 */
typedef struct {
    int busy_gpio_num;       /*!< GPIO number for the BUSY pin */
    bool busy_active_level;  /*!< Active level for BUSY pin (typically 1 for High=Busy, 0 for Low=Busy) */
} esp_lcd_panel_gdew042t2_config_t;

/**
 * @brief EPD Refresh Mode Enumeration
 */
typedef enum {
    GDEW042T2_REFRESH_FULL = 0,    /*!< Full refresh: Slow, eliminates ghosting, causes screen flashing */
    GDEW042T2_REFRESH_PARTIAL = 1  /*!< Partial refresh: Fast, no flashing, may leave ghosting artifacts */
} gdew042t2_refresh_mode_t;

/**
 * @brief Create GDEW042T2 EPD panel handle
 *
 * @param[in]  io LCD panel IO handle
 * @param[in]  panel_dev_config General panel device configuration
 * @param[out] ret_panel Returned LCD panel handle
 * @return
 * - ESP_OK: Success
 * - ESP_ERR_INVALID_ARG: Invalid argument
 * - ESP_ERR_NO_MEM: Memory allocation failed
 */
esp_err_t esp_lcd_new_panel_gdew042t2(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel);

/**
 * @brief Initialize the display for 4-level grayscale mode
 *
 * This function loads the specific LUTs required for grayscale display.
 *
 * @param panel LCD panel handle
 * @return
 * - ESP_OK: Success
 * - ESP_FAIL: Transmission failed
 */
esp_err_t esp_lcd_gdew042t2_init_grayscale(esp_lcd_panel_handle_t panel);

/**
 * @brief Draw a 4-level grayscale image
 *
 * The IL0398 controller generates gray levels by mixing "Old Data" and "New Data".
 *
 * @param panel LCD panel handle
 * @param layer_0x10 Pointer to the buffer for Register 0x10 (Old Data / Bitplane 0)
 * @param layer_0x13 Pointer to the buffer for Register 0x13 (New Data / Bitplane 1)
 * @param len Size of the buffer in bytes
 * @return
 * - ESP_OK: Success
 * - ESP_FAIL: Transmission failed
 */
esp_err_t esp_lcd_gdew042t2_draw_grayscale(esp_lcd_panel_handle_t panel, const void *layer_0x10, const void *layer_0x13, size_t len);

/**
 * @brief Set the refresh mode (loads the corresponding LUT)
 *
 * @param panel LCD panel handle
 * @param mode Refresh mode (FULL or PARTIAL)
 * @return
 * - ESP_OK: Success
 * - ESP_FAIL: Transmission failed
 */
esp_err_t esp_lcd_gdew042t2_set_mode(esp_lcd_panel_handle_t panel, gdew042t2_refresh_mode_t mode);

/**
 * @brief Perform a partial window refresh
 *
 * Only updates the specified rectangular area. The panel must be set to
 * GDEW042T2_REFRESH_PARTIAL mode before calling this function.
 *
 * @note Due to the 1-bit memory layout, 'x' and 'w' must be multiples of 8.
 *
 * @param panel LCD panel handle
 * @param x Start X coordinate (Must be a multiple of 8)
 * @param y Start Y coordinate
 * @param w Width of the area (Must be a multiple of 8)
 * @param h Height of the area
 * @param color_data Buffer containing the image data for the area
 * @return
 * - ESP_OK: Success
 * - ESP_ERR_INVALID_ARG: x or w is not aligned to 8 bytes
 * - ESP_FAIL: Transmission failed
 */
esp_err_t esp_lcd_gdew042t2_draw_partial(esp_lcd_panel_handle_t panel, int x, int y, int w, int h, const void *color_data);

#ifdef __cplusplus
}
#endif