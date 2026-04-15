#include "lcd_brightness.h"
#include "main.h"

/*
 * Software PWM for LCD backlight (LCD_LED_Pin / GPIOE_Pin0).
 *
 * Period  = 10 ms  (100 Hz)  — imperceptible flicker
 * Steps   = 10     (0 % … 100 % in 10 % increments)
 * Tick    = 1 ms   (called from SysTick_Handler)
 *
 * counter counts 0-9; pin is HIGH while counter < s_pwm_level.
 */

static volatile uint8_t s_pwm_level   = 10U;  /* 10 = 100 % (full on) */
static volatile uint8_t s_pwm_counter = 0U;

void lcd_brightness_init(void)
{
    s_pwm_level   = 10U;
    s_pwm_counter = 0U;
    HAL_GPIO_WritePin(LCD_LED_GPIO_Port, LCD_LED_Pin, GPIO_PIN_SET);
}

void lcd_brightness_set(uint8_t percent)
{
    if (percent > 100U) {
        percent = 100U;
    }
    /* Round percent to nearest 10-step level (0-10) */
    s_pwm_level = (uint8_t)((percent + 5U) / 10U);
}

void lcd_brightness_tick(void)
{
    uint8_t cnt = s_pwm_counter + 1U;
    if (cnt >= 10U) {
        cnt = 0U;
    }
    s_pwm_counter = cnt;

    if (s_pwm_level == 0U) {
        HAL_GPIO_WritePin(LCD_LED_GPIO_Port, LCD_LED_Pin, GPIO_PIN_RESET);
    } else if (s_pwm_level >= 10U) {
        HAL_GPIO_WritePin(LCD_LED_GPIO_Port, LCD_LED_Pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(LCD_LED_GPIO_Port, LCD_LED_Pin,
                          (cnt < s_pwm_level) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}
