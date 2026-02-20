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
#include <string.h>
#include "gui_guider.h"
#include "events_init.h"
#include "custom.h"
#include "../config.h"


void setup_scr_screen_1(lv_ui *ui)
{
    char path_buffer[64] = {0}; // 确保缓冲区够大

    //Write codes screen_1
    ui->screen_1 = lv_obj_create(NULL);
    lv_obj_set_size(ui->screen_1, 400, 300);
    lv_obj_set_scrollbar_mode(ui->screen_1, LV_SCROLLBAR_MODE_OFF);

    //Write style for screen_1, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_bg_opa(ui->screen_1, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui->screen_1, lv_color_hex(0xffffff), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui->screen_1, LV_GRAD_DIR_NONE, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_1_emo_img
    ui->screen_1_emo_img = lv_image_create(ui->screen_1);
    lv_obj_set_pos(ui->screen_1_emo_img, 0, 50);
    lv_obj_set_size(ui->screen_1_emo_img, 400, 130);
    memset(path_buffer, 0, sizeof(path_buffer));
    snprintf(path_buffer, sizeof(path_buffer), "%s%s.bin", ICONS_PATH, "neutral");
    lv_image_set_src(ui->screen_1_emo_img, path_buffer);
    lv_image_set_pivot(ui->screen_1_emo_img, 50,50);
    lv_image_set_rotation(ui->screen_1_emo_img, 0);

    //Write style for screen_1_emo_img, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_image_recolor_opa(ui->screen_1_emo_img, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(ui->screen_1_emo_img, 255, LV_PART_MAIN|LV_STATE_DEFAULT);

    //Write codes screen_1_chat_msg
    ui->screen_1_chat_msg = lv_label_create(ui->screen_1);
    lv_obj_set_pos(ui->screen_1_chat_msg, 25, 215);
    lv_obj_set_size(ui->screen_1_chat_msg, 350, 60);
    lv_label_set_text(ui->screen_1_chat_msg, "");
    lv_label_set_long_mode(ui->screen_1_chat_msg, LV_LABEL_LONG_WRAP);

    //Write style for screen_1_chat_msg, Part: LV_PART_MAIN, State: LV_STATE_DEFAULT.
    lv_obj_set_style_border_width(ui->screen_1_chat_msg, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui->screen_1_chat_msg, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui->screen_1_chat_msg, lv_color_hex(0x000000), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui->screen_1_chat_msg, lv_font_MFYueHei_18, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui->screen_1_chat_msg, 255, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui->screen_1_chat_msg, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_line_space(ui->screen_1_chat_msg, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui->screen_1_chat_msg, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui->screen_1_chat_msg, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui->screen_1_chat_msg, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui->screen_1_chat_msg, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui->screen_1_chat_msg, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui->screen_1_chat_msg, 0, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(ui->screen_1_chat_msg, 0, LV_PART_MAIN|LV_STATE_DEFAULT);

    //The custom code of screen_1.


    //Update current screen layout.
    lv_obj_update_layout(ui->screen_1);

}
