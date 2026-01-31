/*
 * SPDX-FileCopyrightText: 2024 Your Name / Espressif Systems
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdint.h>
#include "esp_lcd_panel_vendor.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief WFT042 (UC8176) panel initialization commands.
 */
typedef struct {
    int cmd;                /*<! The specific E-Ink command */
    const void *data;       /*<! Buffer that holds the command specific data */
    size_t data_bytes;      /*<! Size of `data` in memory, in bytes */
    unsigned int delay_ms;  /*<! Delay in milliseconds after this command */
} wft042_init_cmd_t;

/**
 * @brief WFT042 panel vendor configuration.
 *
 * @note  This structure needs to be passed to the `vendor_config` field in `esp_lcd_panel_dev_config_t`.
 */
typedef struct {
    const wft042_init_cmd_t *init_cmds;    /*!< Pointer to initialization commands array. */
    uint16_t init_cmds_size;               /*<! Number of commands in above array */
    
    // --- E-Ink 特有配置 ---
    int busy_gpio_num;      /*<! GPIO number for the BUSY signal (Mandatory for E-Ink) */
    int busy_level;         /*<! Logic level when the screen is BUSY (0 or 1). Usually 0 (Low) for WFT042 */
} wft042_vendor_config_t;

/**
 * @brief Create LCD panel for model WFT042CZ15 (E-Ink)
 *
 * @param[in]  io LCD panel IO handle
 * @param[in]  panel_dev_config General panel device configuration
 * @param[out] ret_panel Returned LCD panel handle
 * @return
 * - ESP_OK: Success
 * - Otherwise: Fail
 */
esp_err_t esp_lcd_new_panel_wft042(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel);

/**
 * @brief WFT042 panel bus configuration structure (Standard SPI)
 *
 */
// 修改后的宏定义：严格遵循 ESP-IDF spi_bus_config_t 的结构体定义顺序
#define WFT042_PANEL_BUS_SPI_CONFIG(sclk, mosi, max_trans_sz)   \
    {                                                           \
        .mosi_io_num = mosi,       /* 1. MOSI */                \
        .miso_io_num = -1,         /* 2. MISO */                \
        .sclk_io_num = sclk,       /* 3. SCLK */                \
        .quadwp_io_num = -1,       /* 4. WP */                  \
        .quadhd_io_num = -1,       /* 5. HD */                  \
        .max_transfer_sz = max_trans_sz, /* ... max_sz */       \
    }

/**
 * @brief WFT042 panel IO configuration structure
 *
 * @note E-Ink displays generally require lower SPI frequency (e.g., 4MHz - 10MHz).
 * Do NOT use 40MHz like TFTs.
 */
#define WFT042_PANEL_IO_SPI_CONFIG(cs, dc, cb, cb_ctx)          \
    {                                                           \
        .cs_gpio_num = cs,                                      \
        .dc_gpio_num = dc,                                      \
        .spi_mode = 0,                                          \
        .pclk_hz = 5 * 1000 * 1000,   /* Default: 5MHz */       \
        .trans_queue_depth = 10,                                \
        .on_color_trans_done = cb,                              \
        .user_ctx = cb_ctx,                                     \
        .lcd_cmd_bits = 8,                                      \
        .lcd_param_bits = 8,                                    \
    }

#ifdef __cplusplus
}
#endif