#include "ui.h"
#include "lvgl.h"

static lv_obj_t *label_count;
static uint32_t count = 0;

static void btn_event_cb(lv_event_t *e)
{
    count++;
    lv_label_set_text_fmt(label_count, "Count: %lu", (unsigned long)count);
}

void ui_init(void)
{
    /* ---- 背景色 ---- */
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x003a57), LV_PART_MAIN);

    /* ---- 标题 ---- */
    lv_obj_t *title = lv_label_create(lv_scr_act());
    lv_label_set_text(title, "LVGL on STM32H750");
    lv_obj_set_style_text_color(title, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);

    /* ---- 计数标签 ---- */
    label_count = lv_label_create(lv_scr_act());
    lv_label_set_text(label_count, "Count: 0");
    lv_obj_set_style_text_color(label_count, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_count, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(label_count, LV_ALIGN_CENTER, 0, -40);

    /* ---- 按钮 ---- */
    lv_obj_t *btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn, 160, 60);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 40);
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Click Me!");
    lv_obj_center(btn_label);

    /* ---- Slider ---- */
    lv_obj_t *slider = lv_slider_create(lv_scr_act());
    lv_obj_set_width(slider, 200);
    lv_obj_align(slider, LV_ALIGN_CENTER, 0, 130);
}
