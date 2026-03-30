#ifndef UI_H
#define UI_H

#include <stdint.h>

void ui_set_game_tick_ms(uint32_t tick_ms);
void ui_init(void);
void ui_process(void);

#endif // UI_H
