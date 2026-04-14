#ifndef CALCULATOR_APP_H
#define CALCULATOR_APP_H

#include "lvgl.h"

typedef void (*calculator_app_exit_cb_t)(void *user_data);

void calculator_app_init(lv_obj_t *parent);
void calculator_app_set_exit_callback(calculator_app_exit_cb_t callback, void *user_data);

#endif