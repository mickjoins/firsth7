#include "main.h"
#include "ft6336.h"
#include "lcd.h"

#define FT6336_ADDR       0x38

extern lcd lcd_desc;

static I2C_HandleTypeDef* pi2c;

void ft6336_read(uint8_t reg, uint8_t *buf, uint8_t len)
{
	HAL_I2C_Master_Transmit(pi2c, FT6336_ADDR << 1, &reg, 1, 100);

	HAL_I2C_Master_Receive(pi2c, FT6336_ADDR << 1 | 0x01, buf, len, 100);
} 

void ft6336_init(I2C_HandleTypeDef* p)
{
    uint8_t temp[2];
    pi2c = p;

    HAL_GPIO_WritePin(TP_CS_GPIO_Port, TP_CS_Pin, GPIO_PIN_RESET);
	HAL_Delay(10);
 	HAL_GPIO_WritePin(TP_CS_GPIO_Port, TP_CS_Pin, GPIO_PIN_SET);
	HAL_Delay(300);

    ft6336_read(FT_ID_G_FOCALTECH_ID, temp, 1);
    ft6336_read(FT_ID_G_CIPHER_MID, temp, 2);
    ft6336_read(FT_ID_G_CIPHER_HIGH, temp, 1);
}

void ft6336_scan(void)
{
    uint8_t buf[4];
    uint8_t mode;
    ft6336_read(FT_REG_NUM_FINGER, &mode, 1);

    if (mode) {
        ft6336_read(FT_TP1_REG, buf, 4);	//读取XY坐标值 
        uint16_t x = ((buf[0] & 0X0F)<<8) + buf[1];
        uint16_t y = ((buf[2] & 0X0F)<<8) + buf[3];

        uint16_t xmap = x;
        uint16_t ymap = y;

        if (lcd_desc.hw->rotate == LCD_ROTATE_90) {
            xmap = lcd_desc.hw->width - y - 1;
            ymap = x;
        }

        if (lcd_desc.hw->rotate == LCD_ROTATE_180) {
            xmap = lcd_desc.hw->width - x - 1;
            ymap = lcd_desc.hw->height - y - 1;
        }

        if (lcd_desc.hw->rotate == LCD_ROTATE_270) {
            xmap = y;
            ymap = lcd_desc.hw->width - x - 1;
        }

        pressed = 1;
        x_pos = xmap;
        y_pos = ymap;
    } else {
        pressed = 0;
    }
}
