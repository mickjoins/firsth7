#include "calculator_app.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

LV_FONT_DECLARE(lv_font_custom_cn16);
#define FONT_CJK &lv_font_custom_cn16

#define COLOR_BG_TOP        lv_color_hex(0xFFF6E8)
#define COLOR_BG_BOTTOM     lv_color_hex(0xF7D6AE)
#define COLOR_CARD          lv_color_hex(0xFFFDFC)
#define COLOR_CARD_SOFT     lv_color_hex(0xF6E6D1)
#define COLOR_DISPLAY       lv_color_hex(0x241B17)
#define COLOR_DISPLAY_SOFT  lv_color_hex(0x8E786A)
#define COLOR_TEXT_MAIN     lv_color_hex(0x2C201A)
#define COLOR_TEXT_SOFT     lv_color_hex(0x6F5A4E)
#define COLOR_KEY_DARK      lv_color_hex(0x45322B)
#define COLOR_KEY_LIGHT     lv_color_hex(0xFFF3E5)
#define COLOR_KEY_ACCENT    lv_color_hex(0xE6814E)
#define COLOR_KEY_ACCENT_2  lv_color_hex(0xD36732)
#define COLOR_SHADOW        lv_color_hex(0xD9A56D)

#if LV_FONT_MONTSERRAT_48
#define FONT_RESULT &lv_font_montserrat_48
#else
#define FONT_RESULT &lv_font_montserrat_20
#endif

typedef enum {
    CALC_OP_NONE = 0,
    CALC_OP_ADD,
    CALC_OP_SUB,
    CALC_OP_MUL,
    CALC_OP_DIV,
} calc_operator_t;

typedef struct {
    lv_obj_t *display_label;
    lv_obj_t *status_label;
    int32_t accumulator;
    calc_operator_t pending_op;
    bool entering_new_value;
    bool error;
    char display_text[24];
    char status_text[32];
} calculator_app_t;

static calculator_app_t g_calculator;
static calculator_app_exit_cb_t g_exit_callback;
static void *g_exit_user_data;

static const char *k_button_rows[][4] = {
    {"C", "BS", "+/-", "/"},
    {"7", "8", "9", "*"},
    {"4", "5", "6", "-"},
    {"1", "2", "3", "+"},
};

static int32_t calculator_current_value(void)
{
    return (int32_t)strtol(g_calculator.display_text, NULL, 10);
}

static const char *calculator_operator_text(calc_operator_t op)
{
    switch (op) {
    case CALC_OP_ADD:
        return "+";
    case CALC_OP_SUB:
        return "-";
    case CALC_OP_MUL:
        return "*";
    case CALC_OP_DIV:
        return "/";
    default:
        return "";
    }
}

static void calculator_refresh(void)
{
    lv_label_set_text(g_calculator.display_label, g_calculator.display_text);
    lv_label_set_text(g_calculator.status_label, g_calculator.status_text);
}

static void calculator_reset_state(void)
{
    g_calculator.accumulator = 0;
    g_calculator.pending_op = CALC_OP_NONE;
    g_calculator.entering_new_value = true;
    g_calculator.error = false;
    (void)snprintf(g_calculator.display_text, sizeof(g_calculator.display_text), "0");
    (void)snprintf(g_calculator.status_text, sizeof(g_calculator.status_text), "Ready");
    calculator_refresh();
}

static bool calculator_apply_pending(void)
{
    int32_t value = calculator_current_value();

    switch (g_calculator.pending_op) {
    case CALC_OP_ADD:
        g_calculator.accumulator += value;
        break;
    case CALC_OP_SUB:
        g_calculator.accumulator -= value;
        break;
    case CALC_OP_MUL:
        g_calculator.accumulator *= value;
        break;
    case CALC_OP_DIV:
        if (value == 0) {
            g_calculator.error = true;
            (void)snprintf(g_calculator.display_text, sizeof(g_calculator.display_text), "Error");
            (void)snprintf(g_calculator.status_text, sizeof(g_calculator.status_text), "Cannot divide by zero");
            calculator_refresh();
            return false;
        }
        g_calculator.accumulator /= value;
        break;
    case CALC_OP_NONE:
    default:
        g_calculator.accumulator = value;
        break;
    }

    (void)snprintf(g_calculator.display_text, sizeof(g_calculator.display_text), "%ld", (long)g_calculator.accumulator);
    return true;
}

static void calculator_input_digit(const char *digit)
{
    size_t len;

    if (g_calculator.error) {
        calculator_reset_state();
    }

    if (g_calculator.entering_new_value) {
        (void)snprintf(g_calculator.display_text, sizeof(g_calculator.display_text), "%s", digit);
        g_calculator.entering_new_value = false;
        (void)snprintf(g_calculator.status_text, sizeof(g_calculator.status_text), "Input");
        calculator_refresh();
        return;
    }

    if ((strcmp(g_calculator.display_text, "0") == 0) && (strcmp(digit, "0") != 0)) {
        (void)snprintf(g_calculator.display_text, sizeof(g_calculator.display_text), "%s", digit);
        calculator_refresh();
        return;
    }

    len = strlen(g_calculator.display_text);
    if (len >= sizeof(g_calculator.display_text) - 2U) {
        return;
    }

    g_calculator.display_text[len] = digit[0];
    g_calculator.display_text[len + 1U] = '\0';
    calculator_refresh();
}

static void calculator_backspace(void)
{
    size_t len;

    if (g_calculator.error) {
        calculator_reset_state();
        return;
    }

    if (g_calculator.entering_new_value) {
        return;
    }

    len = strlen(g_calculator.display_text);
    if (len <= 1U || (len == 2U && g_calculator.display_text[0] == '-')) {
        (void)snprintf(g_calculator.display_text, sizeof(g_calculator.display_text), "0");
        g_calculator.entering_new_value = true;
    } else {
        g_calculator.display_text[len - 1U] = '\0';
    }

    (void)snprintf(g_calculator.status_text, sizeof(g_calculator.status_text), "Edited");
    calculator_refresh();
}

static void calculator_toggle_sign(void)
{
    int32_t value;

    if (g_calculator.error) {
        calculator_reset_state();
        return;
    }

    value = -calculator_current_value();
    (void)snprintf(g_calculator.display_text, sizeof(g_calculator.display_text), "%ld", (long)value);
    g_calculator.entering_new_value = false;
    (void)snprintf(g_calculator.status_text, sizeof(g_calculator.status_text), "Signed");
    calculator_refresh();
}

static void calculator_set_operator(calc_operator_t op)
{
    if (g_calculator.error) {
        calculator_reset_state();
    }

    if (!g_calculator.entering_new_value || g_calculator.pending_op == CALC_OP_NONE) {
        if (!calculator_apply_pending()) {
            return;
        }
    }

    g_calculator.pending_op = op;
    g_calculator.entering_new_value = true;
    (void)snprintf(g_calculator.status_text, sizeof(g_calculator.status_text), "%ld %s",
                   (long)g_calculator.accumulator, calculator_operator_text(op));
    calculator_refresh();
}

static void calculator_eval(void)
{
    if (g_calculator.error) {
        calculator_reset_state();
        return;
    }

    if (g_calculator.pending_op == CALC_OP_NONE) {
        (void)snprintf(g_calculator.status_text, sizeof(g_calculator.status_text), "Result");
        calculator_refresh();
        return;
    }

    if (!calculator_apply_pending()) {
        return;
    }

    g_calculator.pending_op = CALC_OP_NONE;
    g_calculator.entering_new_value = true;
    (void)snprintf(g_calculator.status_text, sizeof(g_calculator.status_text), "Result");
    calculator_refresh();
}

static void back_event_cb(lv_event_t *event)
{
    LV_UNUSED(event);

    if (g_exit_callback != NULL) {
        g_exit_callback(g_exit_user_data);
    }
}

static void button_event_cb(lv_event_t *event)
{
    const char *key = (const char *)lv_event_get_user_data(event);

    if (key == NULL) {
        return;
    }

    if (strcmp(key, "C") == 0) {
        calculator_reset_state();
        return;
    }

    if (strcmp(key, "BS") == 0) {
        calculator_backspace();
        return;
    }

    if (strcmp(key, "+/-") == 0) {
        calculator_toggle_sign();
        return;
    }

    if (strcmp(key, "+") == 0) {
        calculator_set_operator(CALC_OP_ADD);
        return;
    }

    if (strcmp(key, "-") == 0) {
        calculator_set_operator(CALC_OP_SUB);
        return;
    }

    if (strcmp(key, "*") == 0) {
        calculator_set_operator(CALC_OP_MUL);
        return;
    }

    if (strcmp(key, "/") == 0) {
        calculator_set_operator(CALC_OP_DIV);
        return;
    }

    if (strcmp(key, "=") == 0) {
        calculator_eval();
        return;
    }

    calculator_input_digit(key);
}

static lv_obj_t *create_key_button(lv_obj_t *parent, const char *text, bool accent)
{
    lv_obj_t *button = lv_btn_create(parent);
    lv_obj_t *label = lv_label_create(button);

    lv_obj_set_height(button, 44);
    lv_obj_set_flex_grow(button, 1);
    lv_obj_set_style_radius(button, 14, 0);
    lv_obj_set_style_border_width(button, 0, 0);
    lv_obj_set_style_shadow_width(button, 14, 0);
    lv_obj_set_style_shadow_color(button, COLOR_SHADOW, 0);
    lv_obj_set_style_shadow_opa(button, LV_OPA_20, 0);
    lv_obj_set_style_bg_color(button, accent ? COLOR_KEY_ACCENT : COLOR_KEY_LIGHT, 0);
    lv_obj_set_style_bg_grad_color(button, accent ? COLOR_KEY_ACCENT_2 : lv_color_white(), 0);
    lv_obj_set_style_bg_grad_dir(button, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_color(button, accent ? lv_color_darken(COLOR_KEY_ACCENT, 10) : lv_color_darken(COLOR_KEY_LIGHT, 8), LV_STATE_PRESSED);
    lv_obj_set_style_bg_grad_color(button, accent ? lv_color_darken(COLOR_KEY_ACCENT_2, 10) : lv_color_darken(lv_color_white(), 8), LV_STATE_PRESSED);

    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, FONT_CJK, 0);
    lv_obj_set_style_text_color(label, accent ? lv_color_white() : COLOR_TEXT_MAIN, 0);
    lv_obj_center(label);

    lv_obj_add_event_cb(button, button_event_cb, LV_EVENT_CLICKED, (void *)text);

    return button;
}

static void create_key_row(lv_obj_t *parent, const char *labels[], uint32_t count)
{
    lv_obj_t *row = lv_obj_create(parent);

    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_style_pad_gap(row, 5, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    for (uint32_t i = 0; i < count; ++i) {
        bool accent = (strcmp(labels[i], "/") == 0) || (strcmp(labels[i], "*") == 0) ||
                      (strcmp(labels[i], "-") == 0) || (strcmp(labels[i], "+") == 0) ||
                      (strcmp(labels[i], "=") == 0);

        create_key_button(row, labels[i], accent);
    }
}

void calculator_app_init(lv_obj_t *parent)
{
    lv_obj_t *card;
    lv_obj_t *header_row;
    lv_obj_t *back_button;
    lv_obj_t *title_label;
    lv_obj_t *placeholder;
    lv_obj_t *display_box;
    lv_obj_t *keys_box;
    lv_obj_t *zero_row;
    static const char *zero_labels[] = {"0", "="};

    g_calculator = (calculator_app_t){0};

    lv_obj_clean(parent);
    lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(parent, COLOR_BG_TOP, 0);
    lv_obj_set_style_bg_grad_color(parent, COLOR_BG_BOTTOM, 0);
    lv_obj_set_style_bg_grad_dir(parent, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_border_width(parent, 0, 0);
    lv_obj_set_style_pad_all(parent, 0, 0);

    card = lv_obj_create(parent);
    lv_obj_set_size(card, LV_PCT(90), LV_PCT(92));
    lv_obj_center(card);
    lv_obj_set_style_bg_color(card, COLOR_CARD, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_radius(card, 24, 0);
    lv_obj_set_style_shadow_width(card, 30, 0);
    lv_obj_set_style_shadow_color(card, COLOR_SHADOW, 0);
    lv_obj_set_style_shadow_opa(card, LV_OPA_20, 0);
    lv_obj_set_style_pad_left(card, 16, 0);
    lv_obj_set_style_pad_right(card, 16, 0);
    lv_obj_set_style_pad_top(card, 12, 0);
    lv_obj_set_style_pad_bottom(card, 10, 0);
    lv_obj_set_style_pad_gap(card, 6, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    header_row = lv_obj_create(card);
    lv_obj_set_size(header_row, LV_PCT(100), 32);
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
    lv_obj_set_style_bg_color(back_button, lv_color_darken(COLOR_CARD_SOFT, 6), LV_STATE_PRESSED);
    lv_obj_set_style_shadow_width(back_button, 0, 0);
    lv_obj_add_event_cb(back_button, back_event_cb, LV_EVENT_CLICKED, NULL);

    title_label = lv_label_create(back_button);
    lv_label_set_text(title_label, LV_SYMBOL_LEFT " 返回");
    lv_obj_set_style_text_font(title_label, FONT_CJK, 0);
    lv_obj_set_style_text_color(title_label, COLOR_TEXT_MAIN, 0);
    lv_obj_center(title_label);

    title_label = lv_label_create(header_row);
    lv_label_set_text(title_label, "计算器");
    lv_obj_set_style_text_font(title_label, FONT_CJK, 0);
    lv_obj_set_style_text_color(title_label, COLOR_TEXT_MAIN, 0);

    placeholder = lv_obj_create(header_row);
    lv_obj_set_size(placeholder, 70, 30);
    lv_obj_set_style_bg_opa(placeholder, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(placeholder, 0, 0);
    lv_obj_set_style_pad_all(placeholder, 0, 0);
    lv_obj_clear_flag(placeholder, LV_OBJ_FLAG_SCROLLABLE);

    display_box = lv_obj_create(card);
    lv_obj_set_size(display_box, LV_PCT(100), 90);
    lv_obj_set_style_bg_color(display_box, COLOR_DISPLAY, 0);
    lv_obj_set_style_bg_opa(display_box, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(display_box, 22, 0);
    lv_obj_set_style_border_width(display_box, 0, 0);
    lv_obj_set_style_pad_left(display_box, 12, 0);
    lv_obj_set_style_pad_right(display_box, 12, 0);
    lv_obj_set_style_pad_top(display_box, 6, 0);
    lv_obj_set_style_pad_bottom(display_box, 6, 0);
    lv_obj_set_style_pad_gap(display_box, 4, 0);
    lv_obj_set_flex_flow(display_box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(display_box, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
    lv_obj_clear_flag(display_box, LV_OBJ_FLAG_SCROLLABLE);

    g_calculator.status_label = lv_label_create(display_box);
    lv_label_set_text(g_calculator.status_label, "Ready");
    lv_obj_set_width(g_calculator.status_label, LV_PCT(100));
    lv_obj_set_style_text_align(g_calculator.status_label, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_font(g_calculator.status_label, FONT_CJK, 0);
    lv_obj_set_style_text_color(g_calculator.status_label, COLOR_DISPLAY_SOFT, 0);

    g_calculator.display_label = lv_label_create(display_box);
    lv_label_set_text(g_calculator.display_label, "0");
    lv_obj_set_width(g_calculator.display_label, LV_PCT(100));
    lv_obj_set_style_text_align(g_calculator.display_label, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_font(g_calculator.display_label, FONT_RESULT, 0);
    lv_obj_set_style_text_color(g_calculator.display_label, lv_color_white(), 0);
    lv_label_set_long_mode(g_calculator.display_label, LV_LABEL_LONG_CLIP);

    keys_box = lv_obj_create(card);
    lv_obj_set_width(keys_box, LV_PCT(100));
    lv_obj_set_flex_grow(keys_box, 1);
    lv_obj_set_style_bg_opa(keys_box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(keys_box, 0, 0);
    lv_obj_set_style_pad_all(keys_box, 0, 0);
    lv_obj_set_style_pad_gap(keys_box, 5, 0);
    lv_obj_set_flex_flow(keys_box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(keys_box, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(keys_box, LV_OBJ_FLAG_SCROLLABLE);

    for (uint32_t i = 0; i < 4U; ++i) {
        create_key_row(keys_box, k_button_rows[i], 4U);
    }

    zero_row = lv_obj_create(keys_box);
    lv_obj_set_width(zero_row, LV_PCT(100));
    lv_obj_set_height(zero_row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(zero_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(zero_row, 0, 0);
    lv_obj_set_style_pad_all(zero_row, 0, 0);
    lv_obj_set_style_pad_gap(zero_row, 5, 0);
    lv_obj_set_flex_flow(zero_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(zero_row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(zero_row, LV_OBJ_FLAG_SCROLLABLE);

    (void)create_key_button(zero_row, zero_labels[0], false);
    (void)create_key_button(zero_row, zero_labels[1], true);

    calculator_reset_state();
}

void calculator_app_set_exit_callback(calculator_app_exit_cb_t callback, void *user_data)
{
    g_exit_callback = callback;
    g_exit_user_data = user_data;
}