/*
* Copyright 2024 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/


/*********************
 *      INCLUDES
 *********************/
#include <stdio.h>
#include "lvgl.h"
#include "custom.h"
#include "../config.h"
#include <string.h>
#include "esp_log.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

static char* TAG = "LVGL Custom";

lv_font_t* lv_font_zaozigongfangxinranti_92 = NULL;
lv_font_t* lv_font_MFYueHei_18 = NULL;

/**
 * Create a demo application
 */

void custom_init(lv_ui *ui)
{
    char path_buffer[64] = {0}; 
    
    if (lv_fs_is_ready('S'))
    {
        ESP_LOGI(TAG, "File System Ready");
    }else ESP_LOGE(TAG, "Fail! File System Not Found");

    memset(path_buffer, 0, sizeof(path_buffer));
    snprintf(path_buffer, sizeof(path_buffer), "%s%s", FONTS_PATH, "zaozigongfangxinranti_92.bin"); // 假设文件名是这个
    ESP_LOGI("Font", "Loading: %s", path_buffer); // [调试日志]
    lv_font_zaozigongfangxinranti_92 = lv_binfont_create(path_buffer);
    if (lv_font_zaozigongfangxinranti_92 == NULL) {
        ESP_LOGE("Font", "FAILED to load: %s", path_buffer);
        lv_font_zaozigongfangxinranti_92 = (lv_font_t *)LV_FONT_DEFAULT; // 建议兜底
    }

    memset(path_buffer, 0, sizeof(path_buffer));
    snprintf(path_buffer, sizeof(path_buffer), "%s%s", FONTS_PATH, "MFYueHei_18.bin"); // 假设文件名是这个
    ESP_LOGI("Font", "Loading: %s", path_buffer); // [调试日志]
    lv_font_MFYueHei_18 =    lv_binfont_create(path_buffer);
    if (lv_font_MFYueHei_18 == NULL) {
        ESP_LOGE("Font", "FAILED to load: %s", path_buffer);
        lv_font_MFYueHei_18 = (lv_font_t *)LV_FONT_DEFAULT; // 建议兜底
    }
}

