#ifndef LCD_BRIGHTNESS_H
#define LCD_BRIGHTNESS_H

#include <stdint.h>

/**
 * Initialise software-PWM backlight control (full brightness).
 * Call once after GPIO init.
 */
void lcd_brightness_init(void);

/**
 * Set backlight brightness.
 * @param percent  0 (off) … 100 (full on)
 */
void lcd_brightness_set(uint8_t percent);

/**
 * Software-PWM tick — must be called every 1 ms from SysTick_Handler.
 */
void lcd_brightness_tick(void);

#endif /* LCD_BRIGHTNESS_H */
