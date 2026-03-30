/**
 * @file lv_port_disp.c
 * LVGL display driver port for ST7789 3.5" LCD (320x480)
 */

#include "lv_port_disp.h"
#include "lcd.h"
#include "lcd_port.h"
#include "main.h"
#include "spi.h"

#define DISP_HOR_RES 320
#define DISP_VER_RES 480

/* Draw buffer — 10 lines */
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[DISP_HOR_RES * 10];

/* Display driver */
static lv_disp_drv_t disp_drv;

/* LCD hardware instance (shared with the rest of the BSP) */
static lcd_io lcd_io_inst = {
    .spi = &hspi1,
    .rst = { .port = LCD_RST_GPIO_Port, .pin = LCD_RST_Pin, .invert = false },
    .bl  = { .port = LCD_LED_GPIO_Port, .pin = LCD_LED_Pin, .invert = false },
    .cs  = { .port = LCD_CS_GPIO_Port,  .pin = LCD_CS_Pin,  .invert = false },
    .dc  = { .port = LCD_DC_GPIO_Port,  .pin = LCD_DC_Pin,  .invert = false },
    .te  = { 0 }
};

static uint16_t line_buf[DISP_HOR_RES];

lcd lv_lcd = {
    .io           = &lcd_io_inst,
    .line_buffer  = line_buf,
    .frame_buffer = 0,
    .timeout      = 0U
};

/**
 * Flush callback — send a rendered area to the display
 */
static void disp_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    uint16_t w = (uint16_t)(area->x2 - area->x1 + 1);
    uint16_t h = (uint16_t)(area->y2 - area->y1 + 1);

    lcd_set_address(&lv_lcd,
                    (uint16_t)area->x1, (uint16_t)area->y1,
                    (uint16_t)area->x2, (uint16_t)area->y2);

    lcd_write_bulk(lv_lcd.io, (uint8_t *)color_p, (uint32_t)(w * h * 2));

    lv_disp_flush_ready(drv);
}

void lv_port_disp_init(void)
{
    /* Initialise the LCD hardware */
    lcd_init_dev(&lv_lcd, LCD_3_50_INCH, LCD_ROTATE_0);

    /* Initialise draw buffer */
    lv_disp_draw_buf_init(&draw_buf, buf1, NULL, DISP_HOR_RES * 10);

    /* Register the display driver */
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res  = DISP_HOR_RES;
    disp_drv.ver_res  = DISP_VER_RES;
    disp_drv.flush_cb = disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
}
