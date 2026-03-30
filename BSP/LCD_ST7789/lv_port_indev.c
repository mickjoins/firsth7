/**
 * @file lv_port_indev.c
 * LVGL input device port for FT6336 capacitive touch
 */

#include "lv_port_indev.h"
#include "ft6336.h"
#include "i2c.h"

uint8_t pressed = 0U;
uint16_t x_pos = 0U;
uint16_t y_pos = 0U;

static lv_indev_drv_t indev_drv;

static void touchpad_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    ft6336_scan();

    if (pressed) {
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = x_pos;
        data->point.y = y_pos;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void lv_port_indev_init(void)
{
    ft6336_init(&hi2c1);

    lv_indev_drv_init(&indev_drv);
    indev_drv.type    = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    lv_indev_drv_register(&indev_drv);
}
