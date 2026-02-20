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
	lv_obj_t *screen_1;
	bool screen_1_del;
	lv_obj_t *screen_1_emo_img;
	lv_obj_t *screen_1_chat_msg;

	lv_obj_t *screen;
	bool screen_del;
	lv_obj_t *weather_cont;
	lv_obj_t *now_weather_cont;
	lv_obj_t *screen_label_day;
	lv_obj_t *screen_label_mon;
	lv_obj_t *screen_label_date;
	lv_obj_t *screen_label_wday;
	lv_obj_t *screen_label_location;
	lv_obj_t *screen_label_toxicsoul_label;
	lv_obj_t *screen_img_now_weather_icon;
	lv_obj_t *screen_label_now_weather_label;
	lv_obj_t *screen_img_weather_day1;
	lv_obj_t *screen_img_weather_day2;
	lv_obj_t *screen_img_weather_day3;
	lv_obj_t *screen_img_weather_day4;
	lv_obj_t *screen_img_weather_day5;
	lv_obj_t *screen_img_weather_day6;
	lv_obj_t *screen_img_weather_day7;

	lv_obj_t* battery_label;
    lv_obj_t* battery_value;
	lv_obj_t* network_state;

}lv_ui;

typedef void (*ui_setup_scr_t)(lv_ui * ui);

void setup_ui(lv_ui *ui);

void video_play(lv_ui *ui);

void init_keyboard(lv_ui *ui);

extern lv_ui* guider_ui;


void setup_scr_screen(lv_ui *ui);
void setup_scr_screen_1(lv_ui *ui);

#ifdef __cplusplus
}
#endif
#endif
