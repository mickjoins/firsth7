#include "ui.h"

#include <stdio.h>
#include <string.h>

#include "gpio.h"
#include "i2c.h"
#include "spi.h"
#include "lcd.h"
#include "lcd_port.h"
#include "ft6336.h"

#define LCD_WIDTH_PIXELS 320U
#define LCD_HEIGHT_PIXELS 480U

#define TOUCH_SCAN_INTERVAL_MS 20U

static uint32_t game_tick_ms = 16U;

#define SKY_COLOR CREAM_WHITE
#define GROUND_COLOR BLACK
#define TRACK_COLOR GRAY
#define CLOUD_COLOR LGRAYBLUE
#define DINO_COLOR BLACK
#define OBSTACLE_COLOR BLACK
#define PANEL_COLOR WHITE

#define HUD_HEIGHT 28U
#define GROUND_Y 360U
#define TRACK_Y (GROUND_Y + 12U)
#define TRACK_HEIGHT 8U

#define DINO_X 48
#define DINO_WIDTH 30U
#define DINO_HEIGHT 34U
#define DINO_GROUND_Y ((int16_t)(GROUND_Y - DINO_HEIGHT))
#define JUMP_VELOCITY (-20)
#define GRAVITY 2

#define OBSTACLE_WIDTH 22U

#define CLOUD_WIDTH 34U
#define CLOUD_HEIGHT 12U

#define TRACK_PATTERN_SPACING 24U

typedef enum {
	UI_SCREEN_TITLE = 0,
	UI_SCREEN_PLAYING,
	UI_SCREEN_GAME_OVER,
} ui_screen;

typedef struct {
	int16_t y;
	int16_t velocity;
	uint8_t airborne;
} dino_state;

typedef struct {
	int16_t x;
	uint16_t width;
	uint16_t height;
} obstacle_state;

typedef struct {
	int16_t x;
	uint16_t y;
	uint16_t width;
} cloud_state;

typedef struct {
	ui_screen screen;
	dino_state dino;
	obstacle_state obstacle;
	cloud_state cloud;
	uint32_t score;
	uint32_t best_score;
	uint32_t displayed_score;
	uint32_t displayed_best_score;
	uint32_t game_tick;
	uint32_t rng_state;
	uint16_t track_offset;
	uint8_t touch_latched;
} game_state;

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

static uint32_t touch_tick = 0U;
static game_state game = {
	.screen = UI_SCREEN_TITLE,
	.dino = { .y = DINO_GROUND_Y, .velocity = 0, .airborne = 0U },
	.obstacle = { .x = (int16_t)(LCD_WIDTH_PIXELS + 48U), .width = OBSTACLE_WIDTH, .height = 48U },
	.cloud = { .x = 220, .y = 86U, .width = CLOUD_WIDTH },
	.score = 0U,
	.best_score = 0U,
	.displayed_score = UINT32_MAX,
	.displayed_best_score = UINT32_MAX,
	.game_tick = 0U,
	.rng_state = 0x13572468UL,
	.track_offset = 0U,
	.touch_latched = 0U,
};

static uint32_t NextRandom(void)
{
	game.rng_state = (game.rng_state * 1664525UL) + 1013904223UL;
	return game.rng_state;
}

static void FillRectClamped(int16_t x, int16_t y, uint16_t width, uint16_t height, uint16_t color)
{
	int16_t x2 = x + (int16_t)width - 1;
	int16_t y2 = y + (int16_t)height - 1;

	if ((x >= (int16_t)LCD_WIDTH_PIXELS) || (y >= (int16_t)LCD_HEIGHT_PIXELS) || (x2 < 0) || (y2 < 0))
	{
		return;
	}

	if (x < 0)
	{
		x = 0;
	}

	if (y < 0)
	{
		y = 0;
	}

	if (x2 >= (int16_t)LCD_WIDTH_PIXELS)
	{
		x2 = (int16_t)LCD_WIDTH_PIXELS - 1;
	}

	if (y2 >= (int16_t)LCD_HEIGHT_PIXELS)
	{
		y2 = (int16_t)LCD_HEIGHT_PIXELS - 1;
	}

	lcd_fill(&lcd_desc, (uint16_t)x, (uint16_t)y, (uint16_t)x2, (uint16_t)y2, color);
}

static void DrawCenteredText(uint16_t y, const char *text, font_type font, uint16_t front_color, uint16_t back_color)
{
	uint16_t text_width = (uint16_t)(strlen(text) * lcd_fonts[font].width);
	uint16_t x = 0U;

	if (text_width < LCD_WIDTH_PIXELS)
	{
		x = (LCD_WIDTH_PIXELS - text_width) / 2U;
	}

	lcd_set_font(&lcd_desc, font, front_color, back_color);
	lcd_print(&lcd_desc, x, y, "%s", text);
}

static void DrawHud(void)
{
	char score_text[24];
	char best_text[24];

	(void)snprintf(score_text, sizeof(score_text), "SCORE %lu", (unsigned long)game.score);
	(void)snprintf(best_text, sizeof(best_text), "BEST %lu", (unsigned long)game.best_score);

	FillRectClamped(0, 0, LCD_WIDTH_PIXELS, HUD_HEIGHT, SKY_COLOR);
	lcd_set_font(&lcd_desc, FONT_1206, DARKBLUE, SKY_COLOR);
	lcd_print(&lcd_desc, 8U, 8U, "DINO RUN");
	lcd_print(&lcd_desc, 118U, 8U, "%s", score_text);
	lcd_print(&lcd_desc, 234U, 8U, "%s", best_text);
	game.displayed_score = game.score;
	game.displayed_best_score = game.best_score;
}

static void UpdateHudIfNeeded(void)
{
	char score_text[24];
	char best_text[24];

	if ((game.displayed_score == game.score) && (game.displayed_best_score == game.best_score))
	{
		return;
	}

	FillRectClamped(118, 0, 92U, HUD_HEIGHT, SKY_COLOR);
	FillRectClamped(234, 0, 82U, HUD_HEIGHT, SKY_COLOR);

	(void)snprintf(score_text, sizeof(score_text), "SCORE %lu", (unsigned long)game.score);
	(void)snprintf(best_text, sizeof(best_text), "BEST %lu", (unsigned long)game.best_score);

	lcd_set_font(&lcd_desc, FONT_1206, DARKBLUE, SKY_COLOR);
	lcd_print(&lcd_desc, 118U, 8U, "%s", score_text);
	lcd_print(&lcd_desc, 234U, 8U, "%s", best_text);

	game.displayed_score = game.score;
	game.displayed_best_score = game.best_score;
}

static void DrawTrack(void)
{
	uint16_t x = 0U;

	FillRectClamped(0, GROUND_Y - 1, LCD_WIDTH_PIXELS, 2U, GROUND_COLOR);
	FillRectClamped(0, TRACK_Y, LCD_WIDTH_PIXELS, TRACK_HEIGHT, SKY_COLOR);

	for (x = 0U; x < LCD_WIDTH_PIXELS; x = (uint16_t)(x + TRACK_PATTERN_SPACING))
	{
		FillRectClamped((int16_t)x, (int16_t)TRACK_Y + 2, 12U, 2U, TRACK_COLOR);
	}

	FillRectClamped(0, TRACK_Y + TRACK_HEIGHT + 6U, LCD_WIDTH_PIXELS, 2U, TRACK_COLOR);
}

static void RestoreTrackSegment(int16_t x, int16_t width)
{
	int16_t x2 = x + width - 1;
	int16_t dash_start = 0;

	if ((x >= (int16_t)LCD_WIDTH_PIXELS) || (x2 < 0))
	{
		return;
	}

	FillRectClamped(x, GROUND_Y - 1, (uint16_t)width, 2U, GROUND_COLOR);
	FillRectClamped(x, TRACK_Y, (uint16_t)width, TRACK_HEIGHT, SKY_COLOR);

	for (dash_start = 0; dash_start < (int16_t)LCD_WIDTH_PIXELS; dash_start += (int16_t)TRACK_PATTERN_SPACING)
	{
		int16_t dash_end = dash_start + 11;

		if ((dash_end < x) || (dash_start > x2))
		{
			continue;
		}

		FillRectClamped(dash_start, (int16_t)TRACK_Y + 2, 12U, 2U, TRACK_COLOR);
	}

	FillRectClamped(x, TRACK_Y + TRACK_HEIGHT + 6U, (uint16_t)width, 2U, TRACK_COLOR);
}

static void RestoreGameBackgroundRect(int16_t x, int16_t y, uint16_t width, uint16_t height)
{
	FillRectClamped(x, y, width, height, SKY_COLOR);

	if (((y + (int16_t)height) >= ((int16_t)GROUND_Y - 1)) && (y <= (int16_t)(TRACK_Y + TRACK_HEIGHT + 7U)))
	{
		RestoreTrackSegment(x, (int16_t)width);
	}
}

static void DrawCloud(int16_t x, uint16_t y, uint16_t color)
{
	FillRectClamped(x + 4, (int16_t)y + 4, 24U, 6U, color);
	FillRectClamped(x + 8, (int16_t)y, 10U, 6U, color);
	FillRectClamped(x + 16, (int16_t)y + 1, 10U, 7U, color);
}

static void DrawDino(int16_t x, int16_t y, uint16_t color)
{
	FillRectClamped(x + 16, y, 10U, 12U, color);
	FillRectClamped(x + 10, y + 8, 14U, 12U, color);
	FillRectClamped(x + 4, y + 12, 8U, 6U, color);
	FillRectClamped(x + 8, y + 20, 16U, 6U, color);
	FillRectClamped(x + 8, y + 26, 5U, 8U, color);
	FillRectClamped(x + 18, y + 26, 5U, 8U, color);

	if (color == DINO_COLOR)
	{
		FillRectClamped(x + 20, y + 4, 2U, 2U, SKY_COLOR);
	}
}

static void DrawObstacle(int16_t x, uint16_t height, uint16_t color)
{
	int16_t top = (int16_t)GROUND_Y - (int16_t)height;
	uint16_t arm_y = (height > 24U) ? (height / 2U) : 12U;

	FillRectClamped(x + 8, top, 8U, height, color);
	FillRectClamped(x + 2, top + (int16_t)arm_y, 6U, 6U, color);
	FillRectClamped(x + 14, top + (int16_t)(arm_y / 2U), 6U, 6U, color);
	FillRectClamped(x + 4, top + (int16_t)arm_y - 6, 3U, 10U, color);
	FillRectClamped(x + 15, top + (int16_t)(arm_y / 2U) - 6, 3U, 10U, color);
}

static void DrawSceneBase(void)
{
	lcd_clear(&lcd_desc, SKY_COLOR);
	DrawHud();
	DrawTrack();
	DrawCloud(game.cloud.x, game.cloud.y, CLOUD_COLOR);
	DrawObstacle(game.obstacle.x, game.obstacle.height, OBSTACLE_COLOR);
	DrawDino(DINO_X, game.dino.y, DINO_COLOR);
}

static void ResetRound(void)
{
	game.dino.y = DINO_GROUND_Y;
	game.dino.velocity = 0;
	game.dino.airborne = 0U;
	game.obstacle.x = (int16_t)(LCD_WIDTH_PIXELS + 52U);
	game.obstacle.width = OBSTACLE_WIDTH;
	game.obstacle.height = 44U + (uint16_t)(NextRandom() % 18U);
	game.cloud.x = (int16_t)(LCD_WIDTH_PIXELS - CLOUD_WIDTH - 18U);
	game.cloud.y = 74U + (uint16_t)(NextRandom() % 46U);
	game.cloud.width = CLOUD_WIDTH;
	game.score = 0U;
	game.track_offset = 0U;
	game.displayed_score = UINT32_MAX;
	game.displayed_best_score = UINT32_MAX;
	game.game_tick = HAL_GetTick();
	game.touch_latched = 0U;
}

static void DrawTitleScreen(void)
{
	game.screen = UI_SCREEN_TITLE;
	ResetRound();
	DrawSceneBase();
	DrawCenteredText(126U, "DINO RUN", FONT_1608, DARKBLUE, SKY_COLOR);
	DrawCenteredText(160U, "tap anywhere to start", FONT_1206, GROUND_COLOR, SKY_COLOR);
	DrawCenteredText(182U, "tap while running to jump", FONT_1206, GRAYBLUE, SKY_COLOR);

	FillRectClamped(50, 232, 220U, 44U, PANEL_COLOR);
	lcd_draw_rectangle(&lcd_desc, 50U, 232U, 269U, 275U, DARKBLUE);
	DrawCenteredText(246U, "avoid the cactus", FONT_1206, DARKBLUE, PANEL_COLOR);
}

static void DrawGameOverPanel(void)
{
	char score_text[24];
	char best_text[24];

	FillRectClamped(40, 130, 240U, 170U, PANEL_COLOR);
	lcd_draw_rectangle(&lcd_desc, 40U, 130U, 279U, 299U, GROUND_COLOR);
	DrawCenteredText(142U, "GAME OVER", FONT_1608, RED, PANEL_COLOR);
	(void)snprintf(score_text, sizeof(score_text), "score %lu", (unsigned long)game.score);
	(void)snprintf(best_text, sizeof(best_text), "best  %lu", (unsigned long)game.best_score);
	DrawCenteredText(170U, score_text, FONT_1206, GROUND_COLOR, PANEL_COLOR);
	DrawCenteredText(188U, best_text, FONT_1206, GRAYBLUE, PANEL_COLOR);

	FillRectClamped(60, 214, 200U, 30U, GREEN);
	lcd_draw_rectangle(&lcd_desc, 60U, 214U, 259U, 243U, DARKBLUE);
	DrawCenteredText(220U, "RETRY", FONT_1608, WHITE, GREEN);

	FillRectClamped(60, 256, 200U, 30U, GRAY);
	lcd_draw_rectangle(&lcd_desc, 60U, 256U, 259U, 285U, DARKBLUE);
	DrawCenteredText(262U, "QUIT", FONT_1608, WHITE, GRAY);
}

static void StartGame(void)
{
	ResetRound();
	game.screen = UI_SCREEN_PLAYING;
	DrawSceneBase();
}

static uint8_t TouchPressedEdge(void)
{
	if (pressed == 0U)
	{
		game.touch_latched = 0U;
		return 0U;
	}

	if (game.touch_latched != 0U)
	{
		return 0U;
	}

	game.touch_latched = 1U;
	return 1U;
}

static void JumpIfPossible(void)
{
	if (game.dino.airborne == 0U)
	{
		game.dino.airborne = 1U;
		game.dino.velocity = JUMP_VELOCITY;
	}
}

static uint8_t CheckCollision(void)
{
	int16_t dino_left = DINO_X + 6;
	int16_t dino_right = DINO_X + 24;
	int16_t dino_top = game.dino.y + 2;
	int16_t dino_bottom = game.dino.y + (int16_t)DINO_HEIGHT;

	int16_t obstacle_left = game.obstacle.x + 2;
	int16_t obstacle_right = game.obstacle.x + 18;
	int16_t obstacle_top = (int16_t)GROUND_Y - (int16_t)game.obstacle.height;
	int16_t obstacle_bottom = GROUND_Y;

	if (dino_right <= obstacle_left)
	{
		return 0U;
	}

	if (dino_left >= obstacle_right)
	{
		return 0U;
	}

	if (dino_bottom <= obstacle_top)
	{
		return 0U;
	}

	if (dino_top >= obstacle_bottom)
	{
		return 0U;
	}

	return 1U;
}

static void UpdateGameFrame(void)
{
	int16_t previous_dino_y = game.dino.y;
	int16_t previous_obstacle_x = game.obstacle.x;
	int16_t previous_cloud_x = game.cloud.x;
	uint16_t previous_obstacle_height = game.obstacle.height;
	uint32_t speed = 10U + (game.score / 80U);

	/* Erase obstacle and cloud at old positions (they always move) */
	RestoreGameBackgroundRect(previous_obstacle_x, (int16_t)GROUND_Y - (int16_t)previous_obstacle_height,
						game.obstacle.width, previous_obstacle_height);
	RestoreGameBackgroundRect(previous_cloud_x, (int16_t)game.cloud.y, game.cloud.width, CLOUD_HEIGHT);

	/* Update dino physics */
	if (game.dino.airborne != 0U)
	{
		game.dino.velocity += GRAVITY;
		game.dino.y = (int16_t)(game.dino.y + game.dino.velocity);

		if (game.dino.y >= DINO_GROUND_Y)
		{
			game.dino.y = DINO_GROUND_Y;
			game.dino.velocity = 0;
			game.dino.airborne = 0U;
		}
	}

	/* Only erase the exposed strip when dino Y actually changed */
	if (previous_dino_y != game.dino.y)
	{
		if (game.dino.y < previous_dino_y)
		{
			/* Moved up: erase bottom strip left behind */
			int16_t strip_y = game.dino.y + (int16_t)DINO_HEIGHT;
			uint16_t strip_h = (uint16_t)(previous_dino_y + (int16_t)DINO_HEIGHT - strip_y);
			RestoreGameBackgroundRect(DINO_X, strip_y, DINO_WIDTH, strip_h);
		}
		else
		{
			/* Moved down: erase top strip left behind */
			uint16_t strip_h = (uint16_t)(game.dino.y - previous_dino_y);
			RestoreGameBackgroundRect(DINO_X, previous_dino_y, DINO_WIDTH, strip_h);
		}
	}

	game.obstacle.x = (int16_t)(game.obstacle.x - (int16_t)speed);
	if ((game.obstacle.x + (int16_t)game.obstacle.width) < 0)
	{
		game.obstacle.x = (int16_t)(LCD_WIDTH_PIXELS + 80U + (NextRandom() % 80U));
		game.obstacle.height = 38U + (uint16_t)(NextRandom() % 24U);
		game.score += 25U;
	}

	game.cloud.x -= 3;
	if ((game.cloud.x + (int16_t)game.cloud.width) < 0)
	{
		game.cloud.x = (int16_t)(LCD_WIDTH_PIXELS + 20U + (NextRandom() % 40U));
		game.cloud.y = 72U + (uint16_t)(NextRandom() % 52U);
	}

	game.score += 1U;
	if (game.score > game.best_score)
	{
		game.best_score = game.score;
	}

	/* Draw order: cloud → obstacle → dino (dino last to repair any overlap) */
	DrawCloud(game.cloud.x, game.cloud.y, CLOUD_COLOR);
	DrawObstacle(game.obstacle.x, game.obstacle.height, OBSTACLE_COLOR);

	{
		/* Redraw dino if it moved, or if obstacle/cloud erase overlapped its area */
		uint8_t need_redraw = (previous_dino_y != game.dino.y) ? 1U : 0U;

		if (need_redraw == 0U)
		{
			int16_t dino_right = DINO_X + (int16_t)DINO_WIDTH;
			if ((previous_obstacle_x < dino_right) &&
				((previous_obstacle_x + (int16_t)game.obstacle.width) > DINO_X))
			{
				need_redraw = 1U;
			}
		}

		if (need_redraw != 0U)
		{
			DrawDino(DINO_X, game.dino.y, DINO_COLOR);
		}
	}
	UpdateHudIfNeeded();

	if (CheckCollision() != 0U)
	{
		game.screen = UI_SCREEN_GAME_OVER;
		DrawGameOverPanel();
	}
}

void ui_set_game_tick_ms(uint32_t tick_ms)
{
	if (tick_ms < 5U)
	{
		tick_ms = 5U;
	}
	game_tick_ms = tick_ms;
}

void ui_init(void)
{
	lcd_init_dev(&lcd_desc, LCD_3_50_INCH, LCD_ROTATE_0);
	ft6336_init(&hi2c1);
	touch_tick = HAL_GetTick();
	game.rng_state ^= touch_tick;
	DrawTitleScreen();
}

void ui_process(void)
{
	uint32_t now = HAL_GetTick();
	uint8_t touch_edge = 0U;

	if ((now - touch_tick) < TOUCH_SCAN_INTERVAL_MS)
	{
		return;
	}

	touch_tick = now;
	ft6336_scan();
	touch_edge = TouchPressedEdge();

	if (touch_edge != 0U)
	{
		if (game.screen == UI_SCREEN_TITLE)
		{
			StartGame();
			return;
		}

		if (game.screen == UI_SCREEN_GAME_OVER)
		{
			if ((y_pos >= 214U) && (y_pos <= 243U) && (x_pos >= 60U) && (x_pos <= 259U))
			{
				StartGame();
			}
			else if ((y_pos >= 256U) && (y_pos <= 285U) && (x_pos >= 60U) && (x_pos <= 259U))
			{
				DrawTitleScreen();
			}
			return;
		}

		JumpIfPossible();
	}

	if (game.screen != UI_SCREEN_PLAYING)
	{
		return;
	}

	if ((now - game.game_tick) < game_tick_ms)
	{
		return;
	}

	game.game_tick = now;
	UpdateGameFrame();
}
