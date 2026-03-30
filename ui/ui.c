#include "ui.h"

#include <string.h>

#include "gpio.h"
#include "i2c.h"
#include "spi.h"
#include "lcd.h"
#include "lcd_port.h"
#include "ft6336.h"

#define LCD_WIDTH_PIXELS 320U
#define LCD_HEIGHT_PIXELS 480U
#define BUTTON_X1 60U
#define BUTTON_Y1 220U
#define BUTTON_X2 260U
#define BUTTON_Y2 270U
#define TOUCH_SCAN_INTERVAL_MS 20U

typedef enum {
	PAGE_HOME = 0,
	PAGE_2,
} page_id;

static uint16_t lcd_line_buffer[LCD_WIDTH_PIXELS];

static lcd_io lcd_io_desc = {
	.spi = &hspi1,
	.rst = { .port = LCD_RST_GPIO_Port, .pin = LCD_RST_Pin, .invert = false },
	.bl = { .port = LCD_LED_GPIO_Port, .pin = LCD_LED_Pin, .invert = false },
	.cs = { .port = LCD_CS_GPIO_Port, .pin = LCD_CS_Pin, .invert = false },
	.dc = { .port = LCD_DC_GPIO_Port, .pin = LCD_DC_Pin, .invert = false },
	.te = { 0 }
};

lcd lcd_desc = {
	.io = &lcd_io_desc,
	.line_buffer = lcd_line_buffer,
	.frame_buffer = 0,
	.timeout = 0U
};

uint8_t pressed = 0U;
uint16_t x_pos = 0U;
uint16_t y_pos = 0U;

static page_id current_page = PAGE_HOME;
static uint8_t touch_latched = 0U;
static uint32_t touch_tick = 0U;

static uint8_t PointInButtonRect(uint16_t x, uint16_t y)
{
	return (x >= BUTTON_X1) && (x <= BUTTON_X2) && (y >= BUTTON_Y1) && (y <= BUTTON_Y2);
}

static uint8_t PointInButton(uint16_t raw_x, uint16_t raw_y)
{
	if (PointInButtonRect(raw_x, raw_y) != 0U)
	{
		return 1U;
	}

	if ((raw_y < LCD_WIDTH_PIXELS) && (raw_x < LCD_HEIGHT_PIXELS))
	{
		if (PointInButtonRect(raw_y, (LCD_HEIGHT_PIXELS - 1U) - raw_x) != 0U)
		{
			return 1U;
		}

		if (PointInButtonRect((LCD_WIDTH_PIXELS - 1U) - raw_y, raw_x) != 0U)
		{
			return 1U;
		}

		if (PointInButtonRect(raw_y, raw_x) != 0U)
		{
			return 1U;
		}

		if (PointInButtonRect((LCD_WIDTH_PIXELS - 1U) - raw_y,
													(LCD_HEIGHT_PIXELS - 1U) - raw_x) != 0U)
		{
			return 1U;
		}
	}

	if ((raw_x < LCD_WIDTH_PIXELS) && (raw_y < LCD_HEIGHT_PIXELS))
	{
		if (PointInButtonRect((LCD_WIDTH_PIXELS - 1U) - raw_x,
													(LCD_HEIGHT_PIXELS - 1U) - raw_y) != 0U)
		{
			return 1U;
		}
	}

	return 0U;
}

static void DrawCenteredText(uint16_t y, const char *text, uint16_t front_color, uint16_t back_color)
{
	uint16_t text_width = (uint16_t)(strlen(text) * lcd_desc.font.width);
	uint16_t x = 0U;

	if (text_width < LCD_WIDTH_PIXELS)
	{
		x = (LCD_WIDTH_PIXELS - text_width) / 2U;
	}

	lcd_set_font(&lcd_desc, FONT_1608, front_color, back_color);
	lcd_print(&lcd_desc, x, y, "%s", text);
}

static void DrawCenteredButtonLabel(const char *text, uint16_t front_color, uint16_t back_color)
{
	uint16_t text_width = (uint16_t)(strlen(text) * lcd_desc.font.width);
	uint16_t text_height = lcd_desc.font.height;
	uint16_t button_width = BUTTON_X2 - BUTTON_X1 + 1U;
	uint16_t button_height = BUTTON_Y2 - BUTTON_Y1 + 1U;
	uint16_t x = BUTTON_X1;
	uint16_t y = BUTTON_Y1;

	if (text_width < button_width)
	{
		x = BUTTON_X1 + (button_width - text_width) / 2U;
	}

	if (text_height < button_height)
	{
		y = BUTTON_Y1 + (button_height - text_height) / 2U;
	}

	lcd_set_font(&lcd_desc, FONT_1608, front_color, back_color);
	lcd_print(&lcd_desc, x, y, "%s", text);
}

static void DrawHomePage(void)
{
	lcd_clear(&lcd_desc, CREAM_WHITE);
	DrawCenteredText(150U, "Hello World", DARKBLUE, CREAM_WHITE);
	DrawCenteredText(190U, "touch the button to next page", GRAY, CREAM_WHITE);

	lcd_fill(&lcd_desc, BUTTON_X1, BUTTON_Y1, BUTTON_X2, BUTTON_Y2, BLUE);
	lcd_draw_rectangle(&lcd_desc, BUTTON_X1, BUTTON_Y1, BUTTON_X2, BUTTON_Y2, DARKBLUE);
	DrawCenteredButtonLabel("NEXT PAGE", WHITE, BLUE);
}

static void DrawPage2(void)
{
	lcd_clear(&lcd_desc, CREAM_WHITE);
	DrawCenteredText(200U, "Page 2", DARKBLUE, CREAM_WHITE);

	lcd_fill(&lcd_desc, BUTTON_X1, BUTTON_Y1, BUTTON_X2, BUTTON_Y2, GREEN);
	lcd_draw_rectangle(&lcd_desc, BUTTON_X1, BUTTON_Y1, BUTTON_X2, BUTTON_Y2, DARKBLUE);
	DrawCenteredButtonLabel("BACK HOME", BLACK, GREEN);
}

static void RenderPage(page_id page)
{
	current_page = page;

	if (page == PAGE_HOME)
	{
		DrawHomePage();
	}
	else
	{
		DrawPage2();
	}
}

static void HandleTouch(void)
{
	if (pressed == 0U)
	{
		touch_latched = 0U;
		return;
	}

	if (touch_latched != 0U)
	{
		return;
	}

	touch_latched = 1U;

	if (PointInButton(x_pos, y_pos) == 0U)
	{
		return;
	}

	if (current_page == PAGE_HOME)
	{
		RenderPage(PAGE_2);
	}
	else
	{
		RenderPage(PAGE_HOME);
	}
}

void ui_init(void)
{
	lcd_init_dev(&lcd_desc, LCD_3_50_INCH, LCD_ROTATE_0);
	RenderPage(PAGE_HOME);
	ft6336_init(&hi2c1);
	touch_tick = HAL_GetTick();
}

void ui_process(void)
{
	uint32_t now = HAL_GetTick();

	if ((now - touch_tick) < TOUCH_SCAN_INTERVAL_MS)
	{
		return;
	}

	touch_tick = now;
	ft6336_scan();
	HandleTouch();
}
