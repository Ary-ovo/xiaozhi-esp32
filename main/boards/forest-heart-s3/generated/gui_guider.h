/*
* Copyright 2026 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#ifndef GUI_GUIDER_H
#define GUI_GUIDER_H
#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"


typedef struct
{
  
	lv_obj_t *screen;
	bool screen_del;
	lv_obj_t *screen_label_1;
	lv_obj_t *screen_label_2;
	lv_obj_t *screen_label_3;
	lv_obj_t *screen_label_4;
	lv_obj_t *screen_label_5;
	lv_obj_t *screen_label_6;
	lv_obj_t *screen_img_1;
	lv_obj_t *screen_label_7;
	lv_obj_t *screen_img_2;
	lv_obj_t *screen_img_3;
	lv_obj_t *screen_img_4;
	lv_obj_t *screen_img_5;
	lv_obj_t *screen_img_6;
	lv_obj_t *screen_img_7;
	lv_obj_t *screen_img_8;
	lv_obj_t *screen_img_9;
}lv_ui;

typedef void (*ui_setup_scr_t)(lv_ui * ui);

void setup_ui(lv_ui *ui);

void video_play(lv_ui *ui);

void init_keyboard(lv_ui *ui);

extern lv_ui guider_ui;


void setup_scr_screen(lv_ui *ui);
// LV_IMAGE_DECLARE(_100_RGB565A8_16x16);
// LV_IMAGE_DECLARE(_151_RGB565A8_16x16);
// LV_IMAGE_DECLARE(_309_RGB565A8_16x16);
// LV_IMAGE_DECLARE(_400_RGB565A8_16x16);
// LV_IMAGE_DECLARE(_499_RGB565A8_16x16);
// LV_IMAGE_DECLARE(_999_RGB565A8_16x16);
// LV_IMAGE_DECLARE(_350_RGB565A8_16x16);
// LV_IMAGE_DECLARE(_qweather_RGB565A8_16x16);

// LV_FONT_DECLARE(lv_font_zaozigongfangxinranti_92)
// LV_FONT_DECLARE(lv_font_montserratMedium_16)
// LV_FONT_DECLARE(lv_font_MFYueHei_18)


#ifdef __cplusplus
}
#endif
#endif
