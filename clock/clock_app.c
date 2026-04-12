#include "clock_app.h"

#include "main.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define CLOCK_TAB_COUNT 3U

#define COLOR_BG_TOP      lv_color_hex(0xDDF3DA)
#define COLOR_BG_BOTTOM   lv_color_hex(0xBFEFD8)
#define COLOR_CARD        lv_color_hex(0xF9FCF8)
#define COLOR_CARD_SOFT   lv_color_hex(0xEEF7EF)
#define COLOR_TAB_IDLE    lv_color_hex(0xF0F8F0)
#define COLOR_TAB_ACTIVE  lv_color_hex(0x62C7B6)
#define COLOR_TAB_ACTIVE2 lv_color_hex(0x5AB58D)
#define COLOR_TEXT_MAIN   lv_color_hex(0x217847)
#define COLOR_TEXT_SOFT   lv_color_hex(0x6ABF78)
#define COLOR_ACCENT      lv_color_hex(0x5ABEB7)
#define COLOR_ACCENT_SOFT lv_color_hex(0xCFEED6)
#define COLOR_SHADOW      lv_color_hex(0x85CFA5)

LV_FONT_DECLARE(lv_font_custom_cn16);
#define FONT_CJK &lv_font_custom_cn16

#if LV_FONT_MONTSERRAT_48
#define FONT_TIME_LARGE &lv_font_montserrat_48
#else
#define FONT_TIME_LARGE &lv_font_montserrat_20
#endif

typedef enum {
    CLOCK_PAGE_CLOCK = 0,
    CLOCK_PAGE_TIMER,
    CLOCK_PAGE_STOPWATCH,
} clock_page_t;

typedef struct {
    uint8_t unit;
    int8_t delta;
} timer_adjust_t;

typedef struct {
    lv_obj_t *tab_btns[CLOCK_TAB_COUNT];
    lv_obj_t *pages[CLOCK_TAB_COUNT];
    lv_obj_t *clock_value_label;
    lv_obj_t *timer_value_label;
    lv_obj_t *timer_unit_labels[3];
    lv_obj_t *timer_plus_btns[3];
    lv_obj_t *timer_minus_btns[3];
    lv_obj_t *timer_start_btn;
    lv_obj_t *timer_pause_btn;
    lv_obj_t *timer_reset_btn;
    lv_obj_t *stopwatch_value_label;
    lv_obj_t *stopwatch_start_btn;
    lv_obj_t *stopwatch_pause_btn;
    lv_obj_t *stopwatch_reset_btn;
    uint8_t timer_set[3];
    uint32_t timer_remaining_ms;
    uint32_t timer_last_tick;
    bool timer_running;
    uint32_t stopwatch_elapsed_ms;
    uint32_t stopwatch_last_tick;
    bool stopwatch_running;
    lv_timer_t *tick_timer;
} clock_app_t;

static clock_app_t g_clock_app;
static clock_app_exit_cb_t g_exit_callback;
static void *g_exit_user_data;

static const char *k_tab_titles[CLOCK_TAB_COUNT] = {"\xe6\x97\xb6\xe9\x92\x9f", "\xe8\xae\xa1\xe6\x97\xb6", "\xe7\xa7\x92\xe8\xa1\xa8"};
static const char *k_unit_titles[3] = {"\xe6\x97\xb6", "\xe5\x88\x86", "\xe7\xa7\x92"};
static const uint8_t k_unit_limits[3] = {23U, 59U, 59U};
static const timer_adjust_t k_timer_adjusts[3][2] = {
    { {0U, 1}, {0U, -1} },
    { {1U, 1}, {1U, -1} },
    { {2U, 1}, {2U, -1} },
};

static uint32_t timer_total_ms(void)
{
    uint32_t total_seconds = (uint32_t)g_clock_app.timer_set[0] * 3600U;
    total_seconds += (uint32_t)g_clock_app.timer_set[1] * 60U;
    total_seconds += (uint32_t)g_clock_app.timer_set[2];
    return total_seconds * 1000U;
}

static void format_hms(uint32_t total_seconds, char *buffer, size_t size)
{
    uint32_t hours = total_seconds / 3600U;
    uint32_t minutes = (total_seconds % 3600U) / 60U;
    uint32_t seconds = total_seconds % 60U;
    (void)snprintf(buffer, size, "%02lu:%02lu:%02lu",
                   (unsigned long)hours,
                   (unsigned long)minutes,
                   (unsigned long)seconds);
}

static void set_button_enabled(lv_obj_t *button, bool enabled)
{
    if (enabled) {
        lv_obj_clear_state(button, LV_STATE_DISABLED);
    } else {
        lv_obj_add_state(button, LV_STATE_DISABLED);
    }
}

static lv_obj_t *create_text_button(lv_obj_t *parent, const char *text,
                                    lv_color_t color1, lv_color_t color2)
{
    lv_obj_t *button = lv_btn_create(parent);
    lv_obj_set_size(button, 76, 34);
    lv_obj_set_style_radius(button, 12, 0);
    lv_obj_set_style_border_width(button, 0, 0);
    lv_obj_set_style_shadow_width(button, 18, 0);
    lv_obj_set_style_shadow_color(button, COLOR_SHADOW, 0);
    lv_obj_set_style_shadow_opa(button, LV_OPA_20, 0);
    lv_obj_set_style_bg_color(button, color1, 0);
    lv_obj_set_style_bg_grad_color(button, color2, 0);
    lv_obj_set_style_bg_grad_dir(button, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_bg_color(button, lv_color_darken(color1, 12), LV_STATE_PRESSED);
    lv_obj_set_style_bg_grad_color(button, lv_color_darken(color2, 12), LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(button, LV_OPA_40, LV_STATE_DISABLED);
    lv_obj_set_style_shadow_width(button, 0, LV_STATE_DISABLED);

    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, FONT_CJK, 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_set_style_text_opa(label, LV_OPA_60, LV_STATE_DISABLED);
    lv_obj_center(label);

    return button;
}

static void update_timer_display(void)
{
    char buffer[16];
    uint32_t display_seconds = g_clock_app.timer_remaining_ms == 0U ? 0U :
                               (g_clock_app.timer_remaining_ms + 999U) / 1000U;

    format_hms(display_seconds, buffer, sizeof(buffer));
    lv_label_set_text(g_clock_app.timer_value_label, buffer);

    for (uint32_t i = 0; i < 3U; ++i) {
        lv_label_set_text_fmt(g_clock_app.timer_unit_labels[i], "%u",
                              (unsigned int)g_clock_app.timer_set[i]);
        set_button_enabled(g_clock_app.timer_plus_btns[i], !g_clock_app.timer_running);
        set_button_enabled(g_clock_app.timer_minus_btns[i], !g_clock_app.timer_running);
    }

    set_button_enabled(g_clock_app.timer_start_btn, g_clock_app.timer_remaining_ms > 0U && !g_clock_app.timer_running);
    set_button_enabled(g_clock_app.timer_pause_btn, g_clock_app.timer_running);
}

static void update_stopwatch_display(void)
{
    char buffer[16];
    uint32_t display_seconds = g_clock_app.stopwatch_elapsed_ms / 1000U;

    format_hms(display_seconds, buffer, sizeof(buffer));
    lv_label_set_text(g_clock_app.stopwatch_value_label, buffer);

    set_button_enabled(g_clock_app.stopwatch_start_btn, !g_clock_app.stopwatch_running);
    set_button_enabled(g_clock_app.stopwatch_pause_btn, g_clock_app.stopwatch_running);
}

static void update_clock_display(uint32_t tick_ms)
{
    char buffer[16];
    format_hms(tick_ms / 1000U, buffer, sizeof(buffer));
    lv_label_set_text(g_clock_app.clock_value_label, buffer);
}

static void set_active_page(clock_page_t page)
{
    for (uint32_t i = 0; i < CLOCK_TAB_COUNT; ++i) {
        bool active = (i == (uint32_t)page);
        if (active) {
            lv_obj_clear_flag(g_clock_app.pages[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_bg_color(g_clock_app.tab_btns[i], COLOR_TAB_ACTIVE, 0);
            lv_obj_set_style_bg_grad_color(g_clock_app.tab_btns[i], COLOR_TAB_ACTIVE2, 0);
            lv_obj_set_style_text_color(g_clock_app.tab_btns[i], lv_color_white(), 0);
            lv_obj_set_style_shadow_width(g_clock_app.tab_btns[i], 18, 0);
        } else {
            lv_obj_add_flag(g_clock_app.pages[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_bg_color(g_clock_app.tab_btns[i], COLOR_TAB_IDLE, 0);
            lv_obj_set_style_bg_grad_color(g_clock_app.tab_btns[i], COLOR_TAB_IDLE, 0);
            lv_obj_set_style_text_color(g_clock_app.tab_btns[i], COLOR_TEXT_SOFT, 0);
            lv_obj_set_style_shadow_width(g_clock_app.tab_btns[i], 0, 0);
        }
    }
}

static void tab_event_cb(lv_event_t *event)
{
    clock_page_t page = (clock_page_t)(uintptr_t)lv_event_get_user_data(event);
    set_active_page(page);
}

static void back_event_cb(lv_event_t *event)
{
    LV_UNUSED(event);

    if (g_clock_app.tick_timer != NULL) {
        lv_timer_del(g_clock_app.tick_timer);
        g_clock_app.tick_timer = NULL;
    }

    if (g_exit_callback != NULL) {
        g_exit_callback(g_exit_user_data);
    }
}

static void timer_adjust_event_cb(lv_event_t *event)
{
    const timer_adjust_t *adjust = (const timer_adjust_t *)lv_event_get_user_data(event);
    int16_t next_value;

    if (g_clock_app.timer_running) {
        return;
    }

    next_value = (int16_t)g_clock_app.timer_set[adjust->unit] + adjust->delta;
    if (next_value < 0) {
        next_value = (int16_t)k_unit_limits[adjust->unit];
    } else if (next_value > (int16_t)k_unit_limits[adjust->unit]) {
        next_value = 0;
    }

    g_clock_app.timer_set[adjust->unit] = (uint8_t)next_value;
    g_clock_app.timer_remaining_ms = timer_total_ms();
    update_timer_display();
}

static void timer_start_event_cb(lv_event_t *event)
{
    LV_UNUSED(event);

    if (g_clock_app.timer_running || g_clock_app.timer_remaining_ms == 0U) {
        return;
    }

    g_clock_app.timer_running = true;
    g_clock_app.timer_last_tick = HAL_GetTick();
    update_timer_display();
}

static void timer_pause_event_cb(lv_event_t *event)
{
    LV_UNUSED(event);

    if (!g_clock_app.timer_running) {
        return;
    }

    g_clock_app.timer_running = false;
    update_timer_display();
}

static void timer_reset_event_cb(lv_event_t *event)
{
    LV_UNUSED(event);

    g_clock_app.timer_running = false;
    g_clock_app.timer_remaining_ms = timer_total_ms();
    update_timer_display();
}

static void stopwatch_start_event_cb(lv_event_t *event)
{
    LV_UNUSED(event);

    if (g_clock_app.stopwatch_running) {
        return;
    }

    g_clock_app.stopwatch_running = true;
    g_clock_app.stopwatch_last_tick = HAL_GetTick();
    update_stopwatch_display();
}

static void stopwatch_pause_event_cb(lv_event_t *event)
{
    LV_UNUSED(event);

    if (!g_clock_app.stopwatch_running) {
        return;
    }

    g_clock_app.stopwatch_running = false;
    update_stopwatch_display();
}

static void stopwatch_reset_event_cb(lv_event_t *event)
{
    LV_UNUSED(event);

    g_clock_app.stopwatch_running = false;
    g_clock_app.stopwatch_elapsed_ms = 0U;
    update_stopwatch_display();
}

static lv_obj_t *create_metric_box(lv_obj_t *parent, const char *title, uint32_t index)
{
    lv_obj_t *column = lv_obj_create(parent);
    lv_obj_set_size(column, 72, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(column, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(column, 0, 0);
    lv_obj_set_style_pad_all(column, 0, 0);
    lv_obj_set_style_pad_gap(column, 4, 0);
    lv_obj_set_flex_flow(column, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(column, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(column, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title_label = lv_label_create(column);
    lv_label_set_text(title_label, title);
    lv_obj_set_style_text_font(title_label, FONT_CJK, 0);
    lv_obj_set_style_text_color(title_label, COLOR_TEXT_SOFT, 0);

    lv_obj_t *plus_button = lv_btn_create(column);
    lv_obj_set_size(plus_button, 28, 20);
    lv_obj_set_style_radius(plus_button, 11, 0);
    lv_obj_set_style_border_width(plus_button, 0, 0);
    lv_obj_set_style_bg_color(plus_button, COLOR_ACCENT_SOFT, 0);
    lv_obj_set_style_bg_color(plus_button, COLOR_ACCENT, LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(plus_button, LV_OPA_30, LV_STATE_DISABLED);
    lv_obj_t *plus_label = lv_label_create(plus_button);
    lv_label_set_text(plus_label, "+");
    lv_obj_set_style_text_color(plus_label, COLOR_TEXT_MAIN, 0);
    lv_obj_center(plus_label);
    lv_obj_add_event_cb(plus_button, timer_adjust_event_cb, LV_EVENT_CLICKED,
                        (void *)&k_timer_adjusts[index][0]);

    lv_obj_t *value_box = lv_obj_create(column);
    lv_obj_set_size(value_box, 66, 36);
    lv_obj_set_style_bg_color(value_box, lv_color_white(), 0);
    lv_obj_set_style_border_color(value_box, COLOR_ACCENT_SOFT, 0);
    lv_obj_set_style_border_width(value_box, 2, 0);
    lv_obj_set_style_radius(value_box, 12, 0);
    lv_obj_set_style_pad_all(value_box, 0, 0);
    lv_obj_set_style_shadow_width(value_box, 0, 0);
    lv_obj_clear_flag(value_box, LV_OBJ_FLAG_SCROLLABLE);

    g_clock_app.timer_unit_labels[index] = lv_label_create(value_box);
    lv_obj_set_style_text_font(g_clock_app.timer_unit_labels[index], &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(g_clock_app.timer_unit_labels[index], COLOR_TEXT_MAIN, 0);
    lv_obj_center(g_clock_app.timer_unit_labels[index]);

    lv_obj_t *minus_button = lv_btn_create(column);
    lv_obj_set_size(minus_button, 28, 20);
    lv_obj_set_style_radius(minus_button, 11, 0);
    lv_obj_set_style_border_width(minus_button, 0, 0);
    lv_obj_set_style_bg_color(minus_button, COLOR_ACCENT_SOFT, 0);
    lv_obj_set_style_bg_color(minus_button, COLOR_ACCENT, LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(minus_button, LV_OPA_30, LV_STATE_DISABLED);
    lv_obj_t *minus_label = lv_label_create(minus_button);
    lv_label_set_text(minus_label, "-");
    lv_obj_set_style_text_color(minus_label, COLOR_TEXT_MAIN, 0);
    lv_obj_center(minus_label);
    lv_obj_add_event_cb(minus_button, timer_adjust_event_cb, LV_EVENT_CLICKED,
                        (void *)&k_timer_adjusts[index][1]);

    g_clock_app.timer_plus_btns[index] = plus_button;
    g_clock_app.timer_minus_btns[index] = minus_button;

    return column;
}

static lv_obj_t *create_page_shell(lv_obj_t *parent)
{
    lv_obj_t *page = lv_obj_create(parent);
    lv_obj_set_size(page, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(page, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(page, 0, 0);
    lv_obj_set_style_pad_left(page, 0, 0);
    lv_obj_set_style_pad_right(page, 0, 0);
    lv_obj_set_style_pad_top(page, 2, 0);
    lv_obj_set_style_pad_bottom(page, 0, 0);
    lv_obj_set_style_pad_gap(page, 8, 0);
    lv_obj_set_flex_flow(page, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(page, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    return page;
}

static lv_obj_t *create_clock_page(lv_obj_t *parent)
{
    lv_obj_t *page = create_page_shell(parent);

    lv_obj_t *caption = lv_label_create(page);
    lv_label_set_text(caption, "\xe7\xb3\xbb\xe7\xbb\x9f\xe8\xbf\x90\xe8\xa1\x8c\xe6\x97\xb6\xe9\x97\xb4");
    lv_obj_set_style_text_font(caption, FONT_CJK, 0);
    lv_obj_set_style_text_color(caption, COLOR_TEXT_SOFT, 0);

    g_clock_app.clock_value_label = lv_label_create(page);
    lv_label_set_text(g_clock_app.clock_value_label, "00:00:00");
    lv_obj_set_style_text_font(g_clock_app.clock_value_label, FONT_TIME_LARGE, 0);
    lv_obj_set_style_text_color(g_clock_app.clock_value_label, COLOR_TEXT_MAIN, 0);

    lv_obj_t *hint = lv_label_create(page);
    lv_label_set_text(hint, "\xe7\xb3\xbb\xe7\xbb\x9f\xe8\xbf\x90\xe8\xa1\x8c\xe6\x97\xb6\xe9\x95\xbf\xe6\x98\xbe\xe7\xa4\xba");
    lv_obj_set_style_text_font(hint, FONT_CJK, 0);
    lv_obj_set_style_text_color(hint, COLOR_TEXT_SOFT, 0);

    return page;
}

static lv_obj_t *create_timer_page(lv_obj_t *parent)
{
    lv_obj_t *page = create_page_shell(parent);
    lv_obj_t *picker_row;
    lv_obj_t *button_row;

    lv_obj_t *caption = lv_label_create(page);
    lv_label_set_text(caption, "\xe8\xae\xbe\xe7\xbd\xae\xe6\x97\xb6\xe9\x95\xbf");
    lv_obj_set_style_text_font(caption, FONT_CJK, 0);
    lv_obj_set_style_text_color(caption, COLOR_TEXT_SOFT, 0);

    picker_row = lv_obj_create(page);
    lv_obj_set_size(picker_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(picker_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(picker_row, 0, 0);
    lv_obj_set_style_pad_all(picker_row, 0, 0);
    lv_obj_set_style_pad_gap(picker_row, 12, 0);
    lv_obj_set_flex_flow(picker_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(picker_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(picker_row, LV_OBJ_FLAG_SCROLLABLE);

    for (uint32_t i = 0; i < 3U; ++i) {
        (void)create_metric_box(picker_row, k_unit_titles[i], i);
    }

    g_clock_app.timer_value_label = lv_label_create(page);
    lv_label_set_text(g_clock_app.timer_value_label, "00:05:00");
    lv_obj_set_style_text_font(g_clock_app.timer_value_label, FONT_TIME_LARGE, 0);
    lv_obj_set_style_text_color(g_clock_app.timer_value_label, COLOR_TEXT_MAIN, 0);

    button_row = lv_obj_create(page);
    lv_obj_set_size(button_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(button_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(button_row, 0, 0);
    lv_obj_set_style_pad_all(button_row, 0, 0);
    lv_obj_set_style_pad_gap(button_row, 12, 0);
    lv_obj_set_flex_flow(button_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(button_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(button_row, LV_OBJ_FLAG_SCROLLABLE);

    g_clock_app.timer_start_btn = create_text_button(button_row, "\xe5\xbc\x80\xe5\xa7\x8b", lv_color_hex(0x74C56C), lv_color_hex(0x69B96A));
    lv_obj_add_event_cb(g_clock_app.timer_start_btn, timer_start_event_cb, LV_EVENT_CLICKED, NULL);

    g_clock_app.timer_pause_btn = create_text_button(button_row, "\xe6\x9a\x82\xe5\x81\x9c", lv_color_hex(0xCDE8C9), lv_color_hex(0xB8E0BE));
    lv_obj_add_event_cb(g_clock_app.timer_pause_btn, timer_pause_event_cb, LV_EVENT_CLICKED, NULL);

    g_clock_app.timer_reset_btn = create_text_button(button_row, "\xe9\x87\x8d\xe7\xbd\xae", lv_color_hex(0x68C8C7), lv_color_hex(0x56B4D4));
    lv_obj_add_event_cb(g_clock_app.timer_reset_btn, timer_reset_event_cb, LV_EVENT_CLICKED, NULL);

    return page;
}

static lv_obj_t *create_stopwatch_page(lv_obj_t *parent)
{
    lv_obj_t *page = create_page_shell(parent);
    lv_obj_t *caption = lv_label_create(page);
    lv_obj_t *button_row;

    lv_label_set_text(caption, "\xe7\xa7\x92\xe8\xa1\xa8");
    lv_obj_set_style_text_font(caption, FONT_CJK, 0);
    lv_obj_set_style_text_color(caption, COLOR_TEXT_SOFT, 0);

    g_clock_app.stopwatch_value_label = lv_label_create(page);
    lv_label_set_text(g_clock_app.stopwatch_value_label, "00:00:00");
    lv_obj_set_style_text_font(g_clock_app.stopwatch_value_label, FONT_TIME_LARGE, 0);
    lv_obj_set_style_text_color(g_clock_app.stopwatch_value_label, COLOR_TEXT_MAIN, 0);

    button_row = lv_obj_create(page);
    lv_obj_set_size(button_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(button_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(button_row, 0, 0);
    lv_obj_set_style_pad_all(button_row, 0, 0);
    lv_obj_set_style_pad_gap(button_row, 12, 0);
    lv_obj_set_flex_flow(button_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(button_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(button_row, LV_OBJ_FLAG_SCROLLABLE);

    g_clock_app.stopwatch_start_btn = create_text_button(button_row, "\xe5\xbc\x80\xe5\xa7\x8b", lv_color_hex(0x74C56C), lv_color_hex(0x69B96A));
    lv_obj_add_event_cb(g_clock_app.stopwatch_start_btn, stopwatch_start_event_cb, LV_EVENT_CLICKED, NULL);

    g_clock_app.stopwatch_pause_btn = create_text_button(button_row, "\xe6\x9a\x82\xe5\x81\x9c", lv_color_hex(0xCDE8C9), lv_color_hex(0xB8E0BE));
    lv_obj_add_event_cb(g_clock_app.stopwatch_pause_btn, stopwatch_pause_event_cb, LV_EVENT_CLICKED, NULL);

    g_clock_app.stopwatch_reset_btn = create_text_button(button_row, "\xe9\x87\x8d\xe7\xbd\xae", lv_color_hex(0x68C8C7), lv_color_hex(0x56B4D4));
    lv_obj_add_event_cb(g_clock_app.stopwatch_reset_btn, stopwatch_reset_event_cb, LV_EVENT_CLICKED, NULL);

    return page;
}

static void tick_timer_cb(lv_timer_t *timer)
{
    uint32_t now;
    LV_UNUSED(timer);

    now = HAL_GetTick();
    update_clock_display(now);

    if (g_clock_app.timer_running) {
        uint32_t elapsed = now - g_clock_app.timer_last_tick;
        g_clock_app.timer_last_tick = now;

        if (elapsed >= g_clock_app.timer_remaining_ms) {
            g_clock_app.timer_remaining_ms = 0U;
            g_clock_app.timer_running = false;
        } else {
            g_clock_app.timer_remaining_ms -= elapsed;
        }
        update_timer_display();
    }

    if (g_clock_app.stopwatch_running) {
        uint32_t elapsed = now - g_clock_app.stopwatch_last_tick;
        g_clock_app.stopwatch_last_tick = now;
        g_clock_app.stopwatch_elapsed_ms += elapsed;
        update_stopwatch_display();
    }
}

void clock_app_init(lv_obj_t *parent)
{
    lv_obj_t *card;
    lv_obj_t *header_row;
    lv_obj_t *back_button;
    lv_obj_t *title_label;
    lv_obj_t *tab_row;
    lv_obj_t *content;
    lv_obj_t *glow_a;
    lv_obj_t *glow_b;

    g_clock_app = (clock_app_t){0};
    g_clock_app.timer_set[1] = 5U;
    g_clock_app.timer_remaining_ms = timer_total_ms();

    lv_obj_clean(parent);
    lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(parent, COLOR_BG_TOP, 0);
    lv_obj_set_style_bg_grad_color(parent, COLOR_BG_BOTTOM, 0);
    lv_obj_set_style_bg_grad_dir(parent, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_border_width(parent, 0, 0);
    lv_obj_set_style_pad_all(parent, 0, 0);

    glow_a = lv_obj_create(parent);
    lv_obj_set_size(glow_a, 220, 220);
    lv_obj_set_style_radius(glow_a, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(glow_a, lv_color_hex(0xEAF9E6), 0);
    lv_obj_set_style_bg_opa(glow_a, LV_OPA_70, 0);
    lv_obj_set_style_border_width(glow_a, 0, 0);
    lv_obj_set_style_shadow_width(glow_a, 0, 0);
    lv_obj_align(glow_a, LV_ALIGN_TOP_LEFT, -60, -40);

    glow_b = lv_obj_create(parent);
    lv_obj_set_size(glow_b, 240, 240);
    lv_obj_set_style_radius(glow_b, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(glow_b, lv_color_hex(0xDDF8F2), 0);
    lv_obj_set_style_bg_opa(glow_b, LV_OPA_50, 0);
    lv_obj_set_style_border_width(glow_b, 0, 0);
    lv_obj_set_style_shadow_width(glow_b, 0, 0);
    lv_obj_align(glow_b, LV_ALIGN_BOTTOM_RIGHT, 70, 50);

    card = lv_obj_create(parent);
    lv_obj_set_size(card, LV_PCT(90), LV_PCT(92));
    lv_obj_center(card);
    lv_obj_set_style_bg_color(card, COLOR_CARD, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_radius(card, 24, 0);
    lv_obj_set_style_shadow_width(card, 34, 0);
    lv_obj_set_style_shadow_color(card, COLOR_SHADOW, 0);
    lv_obj_set_style_shadow_opa(card, LV_OPA_20, 0);
    lv_obj_set_style_pad_left(card, 16, 0);
    lv_obj_set_style_pad_right(card, 16, 0);
    lv_obj_set_style_pad_top(card, 12, 0);
    lv_obj_set_style_pad_bottom(card, 10, 0);
    lv_obj_set_style_pad_gap(card, 8, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    header_row = lv_obj_create(card);
    lv_obj_set_size(header_row, LV_PCT(100), 30);
    lv_obj_set_style_bg_opa(header_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(header_row, 0, 0);
    lv_obj_set_style_pad_all(header_row, 0, 0);
    lv_obj_set_flex_flow(header_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(header_row, LV_OBJ_FLAG_SCROLLABLE);

    back_button = lv_btn_create(header_row);
    lv_obj_set_size(back_button, 70, 30);
    lv_obj_set_style_radius(back_button, 12, 0);
    lv_obj_set_style_border_width(back_button, 0, 0);
    lv_obj_set_style_bg_color(back_button, COLOR_CARD_SOFT, 0);
    lv_obj_set_style_bg_color(back_button, COLOR_ACCENT_SOFT, LV_STATE_PRESSED);
    lv_obj_set_style_shadow_width(back_button, 0, 0);
    lv_obj_add_event_cb(back_button, back_event_cb, LV_EVENT_CLICKED, NULL);

    title_label = lv_label_create(back_button);
    lv_label_set_text(title_label, LV_SYMBOL_LEFT " \xe8\xbf\x94\xe5\x9b\x9e");
    lv_obj_set_style_text_font(title_label, FONT_CJK, 0);
    lv_obj_set_style_text_color(title_label, COLOR_TEXT_MAIN, 0);
    lv_obj_center(title_label);

    title_label = lv_label_create(header_row);
    lv_label_set_text(title_label, "\xe6\x97\xb6\xe9\x92\x9f");
    lv_obj_set_style_text_font(title_label, FONT_CJK, 0);
    lv_obj_set_style_text_color(title_label, COLOR_TEXT_MAIN, 0);

    lv_obj_t *placeholder = lv_obj_create(header_row);
    lv_obj_set_size(placeholder, 70, 30);
    lv_obj_set_style_bg_opa(placeholder, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(placeholder, 0, 0);
    lv_obj_set_style_pad_all(placeholder, 0, 0);
    lv_obj_clear_flag(placeholder, LV_OBJ_FLAG_SCROLLABLE);

    tab_row = lv_obj_create(card);
    lv_obj_set_size(tab_row, LV_PCT(100), 40);
    lv_obj_set_style_bg_color(tab_row, COLOR_CARD_SOFT, 0);
    lv_obj_set_style_bg_opa(tab_row, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(tab_row, 0, 0);
    lv_obj_set_style_radius(tab_row, 16, 0);
    lv_obj_set_style_pad_all(tab_row, 4, 0);
    lv_obj_set_style_pad_gap(tab_row, 6, 0);
    lv_obj_set_flex_flow(tab_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(tab_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(tab_row, LV_OBJ_FLAG_SCROLLABLE);

    for (uint32_t i = 0; i < CLOCK_TAB_COUNT; ++i) {
        lv_obj_t *label;

        g_clock_app.tab_btns[i] = lv_btn_create(tab_row);
        lv_obj_set_flex_grow(g_clock_app.tab_btns[i], 1);
        lv_obj_set_height(g_clock_app.tab_btns[i], LV_PCT(100));
        lv_obj_set_style_radius(g_clock_app.tab_btns[i], 14, 0);
        lv_obj_set_style_border_width(g_clock_app.tab_btns[i], 0, 0);
        lv_obj_set_style_bg_color(g_clock_app.tab_btns[i], COLOR_TAB_IDLE, 0);
        lv_obj_set_style_bg_grad_color(g_clock_app.tab_btns[i], COLOR_TAB_IDLE, 0);
        lv_obj_set_style_bg_grad_dir(g_clock_app.tab_btns[i], LV_GRAD_DIR_HOR, 0);
        lv_obj_set_style_shadow_color(g_clock_app.tab_btns[i], COLOR_SHADOW, 0);
        lv_obj_set_style_shadow_opa(g_clock_app.tab_btns[i], LV_OPA_20, 0);

        label = lv_label_create(g_clock_app.tab_btns[i]);
        lv_label_set_text(label, k_tab_titles[i]);
        lv_obj_set_style_text_font(label, FONT_CJK, 0);
        lv_obj_center(label);

        lv_obj_add_event_cb(g_clock_app.tab_btns[i], tab_event_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
    }

    content = lv_obj_create(card);
    lv_obj_set_size(content, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 0, 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    g_clock_app.pages[CLOCK_PAGE_CLOCK] = create_clock_page(content);
    g_clock_app.pages[CLOCK_PAGE_TIMER] = create_timer_page(content);
    g_clock_app.pages[CLOCK_PAGE_STOPWATCH] = create_stopwatch_page(content);

    set_active_page(CLOCK_PAGE_TIMER);
    update_clock_display(HAL_GetTick());
    update_timer_display();
    update_stopwatch_display();

    if (g_clock_app.tick_timer != NULL) {
        lv_timer_del(g_clock_app.tick_timer);
    }
    g_clock_app.tick_timer = lv_timer_create(tick_timer_cb, 100, NULL);
}

void clock_app_set_exit_callback(clock_app_exit_cb_t callback, void *user_data)
{
    g_exit_callback = callback;
    g_exit_user_data = user_data;
}