/*
* Copyright 2026 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#include "custom.h"
#include "../config.h"
#include <string.h>


void setup_scr_screen(lv_ui *ui)
{
    char path_buffer[64] = {0}; // 确保缓冲区够大

    //Write codes screen
    ui->screen = lv_obj_create(NULL);
    lv_obj_set_size(ui->screen, 400, 300);
    lv_obj_set_scrollbar_mode(ui->screen, LV_SCROLLBAR_MODE_OFF);

    //Write style for screen, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_label_1
    ui->screen_label_day = lv_label_create(ui->screen);
    lv_obj_set_pos(ui->screen_label_day, 136, 85);
    lv_obj_set_size(ui->screen_label_day, 128, 92);
    lv_label_set_text(ui->screen_label_day, "11");
    lv_label_set_long_mode(ui->screen_label_day, LV_LABEL_LONG_WRAP);

    //Write style for screen_label_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_label_day, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_label_day, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_label_day, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_label_day, lv_font_zaozigongfangxinranti_92, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_label_day, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_label_day, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_label_day, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_label_day, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_label_day, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_label_day, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_label_day, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_label_day, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_label_day, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_label_day, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_label_2
    ui->screen_label_mon = lv_label_create(ui->screen);
    lv_obj_set_pos(ui->screen_label_mon, 168, 180);
    lv_obj_align(ui->screen_label_mon, LV_ALIGN_CENTER, 0, 40);
    lv_obj_set_size(ui->screen_label_mon, 88, 24);
    lv_label_set_text(ui->screen_label_mon, "January");
    lv_label_set_long_mode(ui->screen_label_mon, LV_LABEL_LONG_WRAP);

    //Write style for screen_label_2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_label_mon, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_label_mon, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_label_mon, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_label_mon, lv_font_MFYueHei_22, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_label_mon, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_label_mon, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_label_mon, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_label_mon, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_label_mon, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_label_mon, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_label_mon, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_label_mon, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_label_mon, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_label_mon, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_label_3
    ui->screen_label_date = lv_label_create(ui->screen);
    lv_obj_set_pos(ui->screen_label_date, 9, 6);
    lv_obj_set_size(ui->screen_label_date, 100, 18);
    lv_label_set_text(ui->screen_label_date, "2026-01-11");
    lv_label_set_long_mode(ui->screen_label_date, LV_LABEL_LONG_WRAP);

    //Write style for screen_label_date, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_label_date, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_label_date, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_label_date, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_label_date, lv_font_MFYueHei_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_label_date, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_label_date, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_label_date, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_label_date, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_label_date, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_label_date, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_label_date, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_label_date, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_label_date, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_label_date, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_label_4
    ui->screen_label_wday = lv_label_create(ui->screen);
    lv_obj_set_pos(ui->screen_label_wday, 9, 27);
    lv_obj_set_size(ui->screen_label_wday, 100, 18);
    lv_label_set_text(ui->screen_label_wday, "Monday");
    lv_label_set_long_mode(ui->screen_label_wday, LV_LABEL_LONG_WRAP);

    //Write style for screen_label_wday, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_label_wday, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_label_wday, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_label_wday, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_label_wday, lv_font_MFYueHei_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_label_wday, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_label_wday, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_label_wday, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_label_wday, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_label_wday, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_label_wday, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_label_wday, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_label_wday, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_label_wday, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_label_wday, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_label_5
    ui->screen_label_location = lv_label_create(ui->screen);
    lv_obj_set_pos(ui->screen_label_location, 83, 27);
    lv_obj_set_size(ui->screen_label_location, 36, 18);
    lv_label_set_text(ui->screen_label_location, "南宁");
    lv_label_set_long_mode(ui->screen_label_location, LV_LABEL_LONG_WRAP);

    //Write style for screen_label_location, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_label_location, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_label_location, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_label_location, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_label_location, lv_font_MFYueHei_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_label_location, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_label_location, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_label_location, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_label_location, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_label_location, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_label_location, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_label_location, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_label_location, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_label_location, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_label_location, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_label_toxicsoul_label
    ui->screen_label_toxicsoul_label = lv_label_create(ui->screen);
    lv_obj_set_pos(ui->screen_label_toxicsoul_label, 44, 210);
    lv_obj_set_size(ui->screen_label_toxicsoul_label, 312, 40);
    lv_label_set_text(ui->screen_label_toxicsoul_label, "我发现我挺能哄女孩睡觉的，只要我一发信息，女孩就说我要睡觉了。");
    lv_label_set_long_mode(ui->screen_label_toxicsoul_label, LV_LABEL_LONG_WRAP);

    //Write style for screen_label_toxicsoul_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_label_toxicsoul_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_label_toxicsoul_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_label_toxicsoul_label, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_label_toxicsoul_label, lv_font_MFYueHei_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_label_toxicsoul_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_label_toxicsoul_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_label_toxicsoul_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_label_toxicsoul_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_label_toxicsoul_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_label_toxicsoul_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_label_toxicsoul_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_label_toxicsoul_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_label_toxicsoul_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_label_toxicsoul_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_img_now_weather_icon
    ui->screen_img_now_weather_icon = lv_image_create(ui->screen);
    lv_obj_set_pos(ui->screen_img_now_weather_icon, 40, 270);
    lv_obj_set_size(ui->screen_img_now_weather_icon, 16, 16);
    lv_obj_add_flag(ui->screen_img_now_weather_icon, LV_OBJ_FLAG_CLICKABLE);
    memset(path_buffer, 0, sizeof(path_buffer));
    snprintf(path_buffer, sizeof(path_buffer), "%s%d.bin", ICONS_PATH, 100);
    lv_image_set_src(ui->screen_img_now_weather_icon, path_buffer);
    lv_image_set_pivot(ui->screen_img_now_weather_icon, 50,50);
    lv_image_set_rotation(ui->screen_img_now_weather_icon, 0);

    //Write style for screen_img_now_weather_icon, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->screen_img_now_weather_icon, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->screen_img_now_weather_icon, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_label_now_weather_label
    ui->screen_label_now_weather_label = lv_label_create(ui->screen);
    lv_obj_set_pos(ui->screen_label_now_weather_label, 65, 270);
    lv_obj_set_size(ui->screen_label_now_weather_label, 320, 18);
    lv_label_set_text(ui->screen_label_now_weather_label, "晴 / 空气质量等级 优 / 体感温度 25°C");
    lv_label_set_long_mode(ui->screen_label_now_weather_label, LV_LABEL_LONG_WRAP);

    //Write style for screen_label_now_weather_label, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_label_now_weather_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_label_now_weather_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_label_now_weather_label, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_label_now_weather_label, lv_font_MFYueHei_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_label_now_weather_label, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_label_now_weather_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_label_now_weather_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_label_now_weather_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_label_now_weather_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_label_now_weather_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_label_now_weather_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_label_now_weather_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_label_now_weather_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_label_now_weather_label, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_img_2
    ui->screen_img_weather_day1 = lv_image_create(ui->screen);
    lv_obj_set_pos(ui->screen_img_weather_day1, 129, 10);
    lv_obj_set_size(ui->screen_img_weather_day1, 16, 16);
    lv_obj_add_flag(ui->screen_img_weather_day1, LV_OBJ_FLAG_CLICKABLE);
    memset(path_buffer, 0, sizeof(path_buffer));
    snprintf(path_buffer, sizeof(path_buffer), "%s%d.bin", ICONS_PATH, 100);
    lv_image_set_src(ui->screen_img_weather_day1, path_buffer);
    lv_image_set_pivot(ui->screen_img_weather_day1, 50,50);
    lv_image_set_rotation(ui->screen_img_weather_day1, 0);

    //Write style for screen_img_weather_day1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->screen_img_weather_day1, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->screen_img_weather_day1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_img_weather_day2
    ui->screen_img_weather_day2 = lv_image_create(ui->screen);
    lv_obj_set_pos(ui->screen_img_weather_day2, 159, 10);
    lv_obj_set_size(ui->screen_img_weather_day2, 16, 16);
    lv_obj_add_flag(ui->screen_img_weather_day2, LV_OBJ_FLAG_CLICKABLE);
    memset(path_buffer, 0, sizeof(path_buffer));
    snprintf(path_buffer, sizeof(path_buffer), "%s%d.bin", ICONS_PATH, 151);
    lv_image_set_src(ui->screen_img_weather_day2, path_buffer);
    lv_image_set_pivot(ui->screen_img_weather_day2, 50,50);
    lv_image_set_rotation(ui->screen_img_weather_day2, 0);

    //Write style for screen_img_weather_day2, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->screen_img_weather_day2, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->screen_img_weather_day2, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_img_weather_day3
    ui->screen_img_weather_day3 = lv_image_create(ui->screen);
    lv_obj_set_pos(ui->screen_img_weather_day3, 189, 10);
    lv_obj_set_size(ui->screen_img_weather_day3, 16, 16);
    lv_obj_add_flag(ui->screen_img_weather_day3, LV_OBJ_FLAG_CLICKABLE);
    memset(path_buffer, 0, sizeof(path_buffer));
    snprintf(path_buffer, sizeof(path_buffer), "%s%d.bin", ICONS_PATH, 309);
    lv_image_set_src(ui->screen_img_weather_day3, path_buffer);
    lv_image_set_pivot(ui->screen_img_weather_day3, 50,50);
    lv_image_set_rotation(ui->screen_img_weather_day3, 0);

    //Write style for screen_img_weather_day3, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->screen_img_weather_day3, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->screen_img_weather_day3, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_img_weather_day4
    ui->screen_img_weather_day4 = lv_image_create(ui->screen);
    lv_obj_set_pos(ui->screen_img_weather_day4, 219, 10);
    lv_obj_set_size(ui->screen_img_weather_day4, 16, 16);
    lv_obj_add_flag(ui->screen_img_weather_day4, LV_OBJ_FLAG_CLICKABLE);
    memset(path_buffer, 0, sizeof(path_buffer));
    snprintf(path_buffer, sizeof(path_buffer), "%s%d.bin", ICONS_PATH, 400);
    lv_image_set_src(ui->screen_img_weather_day4, path_buffer);
    lv_image_set_pivot(ui->screen_img_weather_day4, 50,50);
    lv_image_set_rotation(ui->screen_img_weather_day4, 0);

    //Write style for screen_img_weather_day4, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->screen_img_weather_day4, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->screen_img_weather_day4, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_img_weather_day5
    ui->screen_img_weather_day5 = lv_image_create(ui->screen);
    lv_obj_set_pos(ui->screen_img_weather_day5, 249, 10);
    lv_obj_set_size(ui->screen_img_weather_day5, 16, 16);
    lv_obj_add_flag(ui->screen_img_weather_day5, LV_OBJ_FLAG_CLICKABLE);
    memset(path_buffer, 0, sizeof(path_buffer));
    snprintf(path_buffer, sizeof(path_buffer), "%s%d.bin", ICONS_PATH, 499);
    lv_image_set_src(ui->screen_img_weather_day5, path_buffer);
    lv_image_set_pivot(ui->screen_img_weather_day5, 50,50);
    lv_image_set_rotation(ui->screen_img_weather_day5, 0);

    //Write style for screen_img_weather_day5, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->screen_img_weather_day5, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->screen_img_weather_day5, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_img_weather_day6
    ui->screen_img_weather_day6 = lv_image_create(ui->screen);
    lv_obj_set_pos(ui->screen_img_weather_day6, 279, 10);
    lv_obj_set_size(ui->screen_img_weather_day6, 16, 16);
    lv_obj_add_flag(ui->screen_img_weather_day6, LV_OBJ_FLAG_CLICKABLE);
    memset(path_buffer, 0, sizeof(path_buffer));
    snprintf(path_buffer, sizeof(path_buffer), "%s%d.bin", ICONS_PATH, 999);
    lv_image_set_src(ui->screen_img_weather_day6, path_buffer);
    lv_image_set_pivot(ui->screen_img_weather_day6, 50,50);
    lv_image_set_rotation(ui->screen_img_weather_day6, 0);

    //Write style for screen_img_weather_day6, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->screen_img_weather_day6, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->screen_img_weather_day6, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_img_weather_day7
    ui->screen_img_weather_day7 = lv_image_create(ui->screen);
    lv_obj_set_pos(ui->screen_img_weather_day7, 309, 10);
    lv_obj_set_size(ui->screen_img_weather_day7, 16, 16);
    lv_obj_add_flag(ui->screen_img_weather_day7, LV_OBJ_FLAG_CLICKABLE);
    memset(path_buffer, 0, sizeof(path_buffer));
    snprintf(path_buffer, sizeof(path_buffer), "%s%d.bin", ICONS_PATH, 350);
    lv_image_set_src(ui->screen_img_weather_day7, path_buffer);
    lv_image_set_pivot(ui->screen_img_weather_day7, 50,50);
    lv_image_set_rotation(ui->screen_img_weather_day7, 0);

    //Write style for screen_img_weather_day7, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->screen_img_weather_day7, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->screen_img_weather_day7, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //The custom code of screen.

    lv_obj_align_to(ui->screen_label_now_weather_label, ui->screen_img_now_weather_icon, LV_ALIGN_LEFT_MID, 0, 0);

    ui->now_weather_cont = lv_obj_create(ui->screen);
    lv_obj_set_style_pad_all(ui->now_weather_cont, 0, 0);
    lv_obj_set_style_border_width(ui->now_weather_cont, 0, 0);
    lv_obj_set_size(ui->now_weather_cont, 380, 18);
    lv_obj_align(ui->now_weather_cont, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_layout(ui->now_weather_cont , LV_LAYOUT_FLEX);
    lv_obj_set_style_pad_column(ui->now_weather_cont, 0, 0);
    lv_obj_set_flex_flow(ui->now_weather_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ui->now_weather_cont, 
                        LV_FLEX_ALIGN_CENTER,   // 主轴（水平）居中
                        LV_FLEX_ALIGN_CENTER,   // 交叉轴（垂直）居中
                        LV_FLEX_ALIGN_CENTER);  // 轨道居中

    ui->weather_cont = lv_obj_create(ui->screen);
    lv_obj_set_style_pad_all(ui->weather_cont, 0, 0);
    lv_obj_set_style_border_width(ui->weather_cont, 0, 0);
    lv_obj_set_size(ui->weather_cont, 240, 16);
    lv_obj_set_pos(ui->weather_cont, 115, 9);
    lv_obj_set_layout(ui->weather_cont , LV_LAYOUT_FLEX);
    lv_obj_set_style_pad_column(ui->weather_cont, 15, 0);
    lv_obj_set_flex_flow(ui->weather_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ui->weather_cont, 
                        LV_FLEX_ALIGN_CENTER,   // 主轴（水平）居中
                        LV_FLEX_ALIGN_CENTER,   // 交叉轴（垂直）居中
                        LV_FLEX_ALIGN_CENTER);  // 轨道居中

    lv_obj_set_parent(ui->screen_img_weather_day1, ui->weather_cont);
    lv_obj_set_parent(ui->screen_img_weather_day2, ui->weather_cont);
    lv_obj_set_parent(ui->screen_img_weather_day3, ui->weather_cont);
    lv_obj_set_parent(ui->screen_img_weather_day4, ui->weather_cont);
    lv_obj_set_parent(ui->screen_img_weather_day5, ui->weather_cont);
    lv_obj_set_parent(ui->screen_img_weather_day6, ui->weather_cont);
    lv_obj_set_parent(ui->screen_img_weather_day7, ui->weather_cont);

    lv_obj_set_parent(ui->screen_img_now_weather_icon, ui->now_weather_cont);
    lv_obj_set_parent(ui->screen_label_now_weather_label, ui->now_weather_cont);

    ui->battery_label = lv_label_create(ui->screen);
    lv_obj_set_size(ui->battery_label, 30, 16);
    lv_obj_align_to(ui->battery_label, ui->weather_cont, LV_ALIGN_OUT_RIGHT_MID, -13, -2);
    lv_label_set_text(ui->battery_label, "");
    lv_obj_set_style_text_align(ui->battery_label, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);

    ui->battery_value = lv_label_create(ui->screen);
    lv_obj_set_size(ui->battery_value, 36, 12);
    lv_obj_align_to(ui->battery_value, ui->battery_label, LV_ALIGN_OUT_RIGHT_MID, -3, 2);
    lv_obj_set_style_text_font(ui->battery_value, lv_font_MFYueHei_12, LV_STATE_DEFAULT);
    lv_label_set_text_fmt(ui->battery_value, "%d%%", 83);

    ui->network_state = lv_label_create(ui->screen);
    lv_obj_set_size(ui->network_state, 36, 16);
    lv_obj_align_to(ui->network_state, ui->battery_label, LV_ALIGN_OUT_RIGHT_MID, -3, 2);
    lv_label_set_text(ui->network_state, "");
    lv_obj_set_style_text_align(ui->network_state, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);

    //Update current screen layout.
    lv_obj_update_layout(ui->screen);

}
