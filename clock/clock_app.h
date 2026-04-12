#ifndef CLOCK_APP_H
#define CLOCK_APP_H

#include "lvgl.h"

typedef void (*clock_app_exit_cb_t)(void *user_data);

void clock_app_init(lv_obj_t *parent);
void clock_app_set_exit_callback(clock_app_exit_cb_t callback, void *user_data);

#endif