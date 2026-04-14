#include "ui.h"

#include "calculator_app.h"
#include "clock_app.h"
#include "main.h"

LV_FONT_DECLARE(lv_font_custom_cn16);
#define FONT_CN &lv_font_custom_cn16

#define COLOR_BG         lv_color_hex(0x1A1A2E)
#define COLOR_PANEL      lv_color_hex(0x16213E)
#define COLOR_ACCENT     lv_color_hex(0x0F3460)
#define COLOR_HIGHLIGHT  lv_color_hex(0xE94560)
#define COLOR_TEXT       lv_color_hex(0xECECEC)
#define COLOR_TEXT_SEC   lv_color_hex(0x8892A0)

#define ICON_DISPLAY     LV_SYMBOL_IMAGE
#define ICON_WIFI        LV_SYMBOL_WIFI
#define ICON_AUDIO       LV_SYMBOL_AUDIO
#define ICON_LED         LV_SYMBOL_CHARGE
#define ICON_CLOCK       LV_SYMBOL_BELL
#define ICON_CALC        LV_SYMBOL_EDIT
#define ICON_INFO        LV_SYMBOL_LIST

static void build_main_menu(void);
static lv_obj_t *create_menu_page(lv_obj_t *menu, char *title);
static lv_obj_t *create_menu_item(lv_obj_t *parent, const char *icon,
                                  const char *text, const char *value);
static lv_obj_t *create_switch_item(lv_obj_t *parent, const char *icon,
                                    const char *text, bool on);
static lv_obj_t *create_slider_item(lv_obj_t *parent, const char *icon,
                                    const char *text, int32_t value);
static void build_display_page(lv_obj_t *menu, lv_obj_t *page);
static void build_network_page(lv_obj_t *menu, lv_obj_t *page);
static void build_audio_page(lv_obj_t *menu, lv_obj_t *page);
static void build_led_page(lv_obj_t *menu, lv_obj_t *page);
static void build_about_page(lv_obj_t *menu, lv_obj_t *page);

static void app_exit_cb(void *user_data)
{
    LV_UNUSED(user_data);
    build_main_menu();
}

static void open_calculator_event_cb(lv_event_t *event)
{
    LV_UNUSED(event);
    calculator_app_set_exit_callback(app_exit_cb, NULL);
    calculator_app_init(lv_scr_act());
}

static void open_clock_event_cb(lv_event_t *event)
{
    LV_UNUSED(event);
    clock_app_set_exit_callback(app_exit_cb, NULL);
    clock_app_init(lv_scr_act());
}

static void ledr_switch_event_cb(lv_event_t *event)
{
    lv_obj_t *sw = lv_event_get_target(event);

    HAL_GPIO_WritePin(LEDR_GPIO_Port, LEDR_Pin,
                      lv_obj_has_state(sw, LV_STATE_CHECKED) ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

static void ledg_switch_event_cb(lv_event_t *event)
{
    lv_obj_t *sw = lv_event_get_target(event);

    HAL_GPIO_WritePin(LEDG_GPIO_Port, LEDG_Pin,
                      lv_obj_has_state(sw, LV_STATE_CHECKED) ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

static void ledb_switch_event_cb(lv_event_t *event)
{
    lv_obj_t *sw = lv_event_get_target(event);

    HAL_GPIO_WritePin(LEDB_GPIO_Port, LEDB_Pin,
                      lv_obj_has_state(sw, LV_STATE_CHECKED) ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

static void slider_event_cb(lv_event_t *event)
{
    lv_obj_t *slider = lv_event_get_target(event);
    lv_obj_t *label = (lv_obj_t *)lv_event_get_user_data(event);

    lv_label_set_text_fmt(label, "%ld%%", (long)lv_slider_get_value(slider));
}

static lv_obj_t *create_menu_page(lv_obj_t *menu, char *title)
{
    lv_obj_t *page = lv_menu_page_create(menu, title);

    lv_obj_set_style_bg_color(page, COLOR_BG, 0);
    lv_obj_set_style_bg_opa(page, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(page, 0, 0);
    lv_obj_set_style_pad_all(page, 10, 0);
    lv_obj_add_flag(page, LV_OBJ_FLAG_SCROLL_MOMENTUM);
    lv_obj_add_flag(page, LV_OBJ_FLAG_SCROLL_ELASTIC);
    lv_obj_set_scroll_dir(page, LV_DIR_VER);
    lv_obj_set_style_anim_time(page, 260, 0);

    return page;
}

static lv_obj_t *create_menu_item(lv_obj_t *parent, const char *icon,
                                  const char *text, const char *value)
{
    lv_obj_t *item = lv_obj_create(parent);
    lv_obj_set_size(item, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(item, COLOR_PANEL, 0);
    lv_obj_set_style_bg_opa(item, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(item, 10, 0);
    lv_obj_set_style_border_width(item, 0, 0);
    lv_obj_set_style_pad_all(item, 12, 0);
    lv_obj_set_flex_flow(item, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(item, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    if (icon != NULL) {
        lv_obj_t *ic = lv_label_create(item);
        lv_label_set_text(ic, icon);
        lv_obj_set_style_text_color(ic, COLOR_HIGHLIGHT, 0);
    }

    lv_obj_t *label = lv_label_create(item);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(label, FONT_CN, 0);
    lv_obj_set_style_pad_left(label, icon != NULL ? 10 : 0, 0);
    lv_obj_set_flex_grow(label, 1);

    if (value != NULL) {
        lv_obj_t *val = lv_label_create(item);
        lv_label_set_text(val, value);
        lv_obj_set_style_text_color(val, COLOR_TEXT_SEC, 0);
        lv_obj_set_style_text_font(val, FONT_CN, 0);
    }

    return item;
}

static lv_obj_t *create_switch_item(lv_obj_t *parent, const char *icon,
                                    const char *text, bool on)
{
    lv_obj_t *item = lv_obj_create(parent);
    lv_obj_set_size(item, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(item, COLOR_PANEL, 0);
    lv_obj_set_style_bg_opa(item, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(item, 10, 0);
    lv_obj_set_style_border_width(item, 0, 0);
    lv_obj_set_style_pad_all(item, 12, 0);
    lv_obj_set_flex_flow(item, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(item, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    if (icon != NULL) {
        lv_obj_t *ic = lv_label_create(item);
        lv_label_set_text(ic, icon);
        lv_obj_set_style_text_color(ic, COLOR_HIGHLIGHT, 0);
    }

    lv_obj_t *label = lv_label_create(item);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(label, FONT_CN, 0);
    lv_obj_set_style_pad_left(label, icon != NULL ? 10 : 0, 0);
    lv_obj_set_flex_grow(label, 1);

    lv_obj_t *sw = lv_switch_create(item);
    lv_obj_set_style_bg_color(sw, COLOR_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_bg_color(sw, COLOR_HIGHLIGHT, LV_PART_INDICATOR | LV_STATE_CHECKED);
    if (on) {
        lv_obj_add_state(sw, LV_STATE_CHECKED);
    }

    return sw;
}

static lv_obj_t *create_slider_item(lv_obj_t *parent, const char *icon,
                                    const char *text, int32_t value)
{
    lv_obj_t *item = lv_obj_create(parent);
    lv_obj_t *row;
    lv_obj_t *label;
    lv_obj_t *value_label;
    lv_obj_t *slider;

    lv_obj_set_size(item, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(item, COLOR_PANEL, 0);
    lv_obj_set_style_bg_opa(item, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(item, 10, 0);
    lv_obj_set_style_border_width(item, 0, 0);
    lv_obj_set_style_pad_all(item, 12, 0);
    lv_obj_set_flex_flow(item, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_gap(item, 8, 0);

    row = lv_obj_create(item);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    if (icon != NULL) {
        lv_obj_t *ic = lv_label_create(row);
        lv_label_set_text(ic, icon);
        lv_obj_set_style_text_color(ic, COLOR_HIGHLIGHT, 0);
    }

    label = lv_label_create(row);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, COLOR_TEXT, 0);
    lv_obj_set_style_text_font(label, FONT_CN, 0);
    lv_obj_set_style_pad_left(label, icon != NULL ? 10 : 0, 0);
    lv_obj_set_flex_grow(label, 1);

    value_label = lv_label_create(row);
    lv_label_set_text_fmt(value_label, "%ld%%", (long)value);
    lv_obj_set_style_text_color(value_label, COLOR_TEXT_SEC, 0);

    slider = lv_slider_create(item);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, value, LV_ANIM_OFF);
    lv_obj_set_width(slider, LV_PCT(100));
    lv_obj_set_style_bg_color(slider, COLOR_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider, COLOR_HIGHLIGHT, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, lv_color_white(), LV_PART_KNOB);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, value_label);

    return slider;
}

static void build_display_page(lv_obj_t *menu, lv_obj_t *page)
{
    LV_UNUSED(menu);
    create_menu_item(page, ICON_DISPLAY, "\xe5\x88\x86\xe8\xbe\xa8\xe7\x8e\x87", "320x480");
    create_menu_item(page, ICON_DISPLAY, "\xe9\x9d\xa2\xe6\x9d\xbf", "ST7789");
    (void)create_slider_item(page, ICON_DISPLAY, "\xe4\xba\xae\xe5\xba\xa6", 80);
}

static void build_network_page(lv_obj_t *menu, lv_obj_t *page)
{
    LV_UNUSED(menu);
    create_switch_item(page, ICON_WIFI, "\xe6\x97\xa0\xe7\xba\xbf\xe7\xbd\x91", true);
    create_menu_item(page, ICON_WIFI, "\xe7\x8a\xb6\xe6\x80\x81", "\xe6\x9c\xaa\xe8\xbf\x9e\xe6\x8e\xa5");
    create_menu_item(page, ICON_WIFI, "IP", "192.168.1.100");
}

static void build_audio_page(lv_obj_t *menu, lv_obj_t *page)
{
    LV_UNUSED(menu);
    (void)create_slider_item(page, ICON_AUDIO, "\xe9\x9f\xb3\xe9\x87\x8f", 50);
    create_switch_item(page, ICON_AUDIO, "\xe9\x9d\x99\xe9\x9f\xb3", false);
}

static void build_led_page(lv_obj_t *menu, lv_obj_t *page)
{
    LV_UNUSED(menu);

    lv_obj_t *ledr_sw = create_switch_item(page, ICON_LED, "\xe7\xba\xa2\xe7\x81\xaf", false);
    lv_obj_add_event_cb(ledr_sw, ledr_switch_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *ledg_sw = create_switch_item(page, ICON_LED, "\xe7\xbb\xbf\xe7\x81\xaf", false);
    lv_obj_add_event_cb(ledg_sw, ledg_switch_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *ledb_sw = create_switch_item(page, ICON_LED, "\xe8\x93\x9d\xe7\x81\xaf", false);
    lv_obj_add_event_cb(ledb_sw, ledb_switch_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

static void build_about_page(lv_obj_t *menu, lv_obj_t *page)
{
    LV_UNUSED(menu);
    create_menu_item(page, ICON_INFO, "\xe5\xbc\x80\xe5\x8f\x91\xe6\x9d\xbf", "STM32H750");
    create_menu_item(page, ICON_INFO, "\xe5\x9b\xbe\xe5\xbd\xa2\xe5\xba\x93", "LVGL");
    create_menu_item(page, ICON_INFO, "\xe6\x97\xb6\xe9\x92\x9f", "\xe5\x8f\xaf\xe7\x94\xa8");
}

static void build_main_menu(void)
{
    lv_obj_t *menu;
    lv_obj_t *root_page;
    lv_obj_t *title_cont;
    lv_obj_t *title_label;
    lv_obj_t *subtitle_label;
    lv_obj_t *list_cont;
    lv_obj_t *page_display;
    lv_obj_t *page_network;
    lv_obj_t *page_audio;
    lv_obj_t *page_led;
    lv_obj_t *page_about;

    struct menu_item_desc {
        const char *icon;
        const char *text;
        lv_obj_t *page;
        lv_event_cb_t cb;
    } items[7];

    lv_obj_clean(lv_scr_act());
    lv_obj_set_style_bg_color(lv_scr_act(), COLOR_BG, 0);

    menu = lv_menu_create(lv_scr_act());
    lv_obj_set_size(menu, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));
    lv_obj_center(menu);
    lv_obj_set_style_bg_color(menu, COLOR_BG, 0);
    lv_obj_set_style_bg_opa(menu, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(menu, 0, 0);
    lv_obj_set_style_border_width(menu, 0, 0);
    lv_obj_set_style_text_font(menu, FONT_CN, 0);

    lv_obj_t *header = lv_menu_get_main_header(menu);
    if (header != NULL) {
        lv_obj_set_style_bg_color(header, COLOR_ACCENT, 0);
        lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
        lv_obj_set_style_pad_ver(header, 10, 0);
    }
    lv_menu_set_mode_header(menu, LV_MENU_HEADER_TOP_FIXED);

    lv_obj_t *back_btn = lv_menu_get_main_header_back_btn(menu);
    if (back_btn != NULL) {
        lv_obj_set_size(back_btn, 56, 48);
        lv_obj_set_style_pad_all(back_btn, 10, 0);
        lv_obj_set_ext_click_area(back_btn, 15);
        lv_obj_t *back_label = lv_obj_get_child(back_btn, 0);
        if (back_label != NULL) {
            lv_obj_set_style_text_font(back_label, &lv_font_montserrat_20, 0);
        }
    }

    page_display = create_menu_page(menu, "\xe6\x98\xbe\xe7\xa4\xba");
    build_display_page(menu, page_display);

    page_network = create_menu_page(menu, "\xe7\xbd\x91\xe7\xbb\x9c");
    build_network_page(menu, page_network);

    page_audio = create_menu_page(menu, "\xe9\x9f\xb3\xe9\xa2\x91");
    build_audio_page(menu, page_audio);

    page_led = create_menu_page(menu, "\xe7\x81\xaf\xe6\x8e\xa7");
    build_led_page(menu, page_led);

    page_about = create_menu_page(menu, "\xe5\x85\xb3\xe4\xba\x8e");
    build_about_page(menu, page_about);

    root_page = lv_menu_page_create(menu, NULL);
    lv_obj_set_style_pad_hor(root_page, 0, 0);
    lv_obj_set_style_pad_ver(root_page, 0, 0);

    title_cont = lv_obj_create(root_page);
    lv_obj_set_size(title_cont, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(title_cont, COLOR_ACCENT, 0);
    lv_obj_set_style_bg_opa(title_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(title_cont, 0, 0);
    lv_obj_set_style_radius(title_cont, 0, 0);
    lv_obj_set_style_pad_all(title_cont, 16, 0);
    lv_obj_set_flex_flow(title_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(title_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    title_label = lv_label_create(title_cont);
    lv_label_set_text(title_label, LV_SYMBOL_SETTINGS "  \xe8\xae\xbe\xe7\xbd\xae");
    lv_obj_set_style_text_font(title_label, FONT_CN, 0);
    lv_obj_set_style_text_color(title_label, lv_color_white(), 0);

    subtitle_label = lv_label_create(title_cont);
    lv_label_set_text(subtitle_label, "STM32H750 Control Center");
    lv_obj_set_style_text_color(subtitle_label, COLOR_TEXT_SEC, 0);
    lv_obj_set_style_text_font(subtitle_label, FONT_CN, 0);

    list_cont = lv_obj_create(root_page);
    lv_obj_set_size(list_cont, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(list_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list_cont, 0, 0);
    lv_obj_set_style_pad_all(list_cont, 8, 0);
    lv_obj_set_style_pad_gap(list_cont, 6, 0);
    lv_obj_set_flex_flow(list_cont, LV_FLEX_FLOW_COLUMN);

    items[0] = (struct menu_item_desc){ ICON_DISPLAY, "\xe6\x98\xbe\xe7\xa4\xba", page_display, NULL };
    items[1] = (struct menu_item_desc){ ICON_WIFI, "\xe7\xbd\x91\xe7\xbb\x9c", page_network, NULL };
    items[2] = (struct menu_item_desc){ ICON_AUDIO, "\xe9\x9f\xb3\xe9\xa2\x91", page_audio, NULL };
    items[3] = (struct menu_item_desc){ ICON_LED, "\xe7\x81\xaf\xe6\x8e\xa7", page_led, NULL };
    items[4] = (struct menu_item_desc){ ICON_CALC, "\xe8\xae\xa1\xe7\xae\x97\xe5\x99\xa8", NULL, open_calculator_event_cb };
    items[5] = (struct menu_item_desc){ ICON_CLOCK, "\xe6\x97\xb6\xe9\x92\x9f", NULL, open_clock_event_cb };
    items[6] = (struct menu_item_desc){ ICON_INFO, "\xe5\x85\xb3\xe4\xba\x8e", page_about, NULL };

    for (uint32_t i = 0; i < 7U; ++i) {
        lv_obj_t *item = lv_obj_create(list_cont);
        lv_obj_set_size(item, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_bg_color(item, COLOR_PANEL, 0);
        lv_obj_set_style_bg_opa(item, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(item, 12, 0);
        lv_obj_set_style_border_width(item, 0, 0);
        lv_obj_set_style_pad_all(item, 14, 0);
        lv_obj_set_flex_flow(item, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(item, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        lv_obj_t *icon_label = lv_label_create(item);
        lv_label_set_text(icon_label, items[i].icon);
        lv_obj_set_style_text_color(icon_label, COLOR_HIGHLIGHT, 0);
        lv_obj_set_style_text_font(icon_label, &lv_font_montserrat_20, 0);

        lv_obj_t *text_label = lv_label_create(item);
        lv_label_set_text(text_label, items[i].text);
        lv_obj_set_style_text_color(text_label, COLOR_TEXT, 0);
        lv_obj_set_style_text_font(text_label, FONT_CN, 0);
        lv_obj_set_style_pad_left(text_label, 12, 0);
        lv_obj_set_flex_grow(text_label, 1);

        lv_obj_t *arrow = lv_label_create(item);
        lv_label_set_text(arrow, LV_SYMBOL_RIGHT);
        lv_obj_set_style_text_color(arrow, COLOR_TEXT_SEC, 0);

        if (items[i].page != NULL) {
            lv_menu_set_load_page_event(menu, item, items[i].page);
        } else if (items[i].cb != NULL) {
            lv_obj_add_event_cb(item, items[i].cb, LV_EVENT_CLICKED, NULL);
        }
    }

    lv_menu_set_page(menu, root_page);
}

void ui_init(void)
{
    build_main_menu();
}