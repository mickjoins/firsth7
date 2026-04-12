#include "main.h"
#include "lcd.h"
#include "lcd_port.h"

typedef struct {
    SPI_HandleTypeDef *hspi;
    lcd_dma_complete_cb_t complete_cb;
    void *user_data;
    volatile bool busy;
} lcd_dma_state_t;

static lcd_dma_state_t lcd_dma_state = { 0 };

static SPI_HandleTypeDef *lcd_get_spi_handle(lcd_io *lcdio)
{
    return (SPI_HandleTypeDef *)lcdio->spi;
}

/************ Hardware Port ************/
void lcd_delay(uint32_t delay)
{
    HAL_Delay(delay);
}

static void lcd_wait_for_ready(SPI_HandleTypeDef *hspi)
{
    while (hspi && ((lcd_dma_state.busy && lcd_dma_state.hspi == hspi) || HAL_SPI_GetState(hspi) != HAL_SPI_STATE_READY)) {
    }
}

static void lcd_clean_dcache(const void *data, uint32_t len)
{
    uintptr_t addr;
    uintptr_t start;
    uintptr_t end;

    if (!data || len == 0U) {
        return;
    }

    addr = (uintptr_t)data;
    start = addr & ~((uintptr_t)31U);
    end = (addr + len + 31U) & ~((uintptr_t)31U);
    SCB_CleanDCache_by_Addr((uint32_t *)start, (int32_t)(end - start));
}

static void lcd_io_ctrl(gpio_io* io, bool flag)
{
    if(io && io->port)
        HAL_GPIO_WritePin(io->port, io->pin, flag ^ io->invert);
}

static void lcd_spi_transmit(void* spi, uint8_t* data, uint32_t len)
{
    SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)spi;

    lcd_wait_for_ready(hspi);

    while(spi && len) {
        if(len > 0xffff) {
            HAL_SPI_Transmit(spi, data, 0xffff, 0xffff);
            len -= 0xffff;
            data += 0xffff;
        } else {
            HAL_SPI_Transmit(spi, data, len, 0xffff);
            break;
        }
    }
}

/************ GPIO ************/
void lcd_io_rst(lcd_io* lcdio, bool flag)
{
    lcd_io_ctrl(&lcdio->rst, flag);
}

void lcd_io_bl(lcd_io* lcdio, bool flag)
{
    lcd_io_ctrl(&lcdio->bl, flag);
}

void lcd_io_cs(lcd_io* lcdio, bool flag)
{
    lcd_io_ctrl(&lcdio->cs, flag);
}

void lcd_io_dc(lcd_io* lcdio, bool flag)
{
    lcd_io_ctrl(&lcdio->dc, flag);
}

/************ SPI ************/
void lcd_write_byte(lcd_io* lcdio, uint8_t data)
{
    lcd_io_dc(lcdio, 1);
    lcd_spi_transmit(lcdio->spi, &data, 0x01);
}

void lcd_write_halfword(lcd_io* lcdio, uint16_t data)
{
    lcd_io_dc(lcdio, 1);
    /* note: 使用HAL库一次发送两个字节顺序与屏幕定义顺序相反 */
    data = (data << 8) | (data >> 8);
    lcd_spi_transmit(lcdio->spi, (uint8_t *)&data, 0x02);
}

void lcd_write_bulk(lcd_io* lcdio, uint8_t* data, uint32_t len)
{
    lcd_io_dc(lcdio, 1);
    lcd_spi_transmit(lcdio->spi, (uint8_t *)data, len);
}

bool lcd_write_bulk_dma(lcd_io* lcdio, uint8_t* data, uint32_t len, lcd_dma_complete_cb_t complete_cb, void *user_data)
{
    SPI_HandleTypeDef *hspi;

    if (!lcdio || !lcdio->spi || !data || len == 0U || len > 0xffffU) {
        return false;
    }

    hspi = lcd_get_spi_handle(lcdio);
    lcd_wait_for_ready(hspi);

    lcd_io_dc(lcdio, 1);
    lcd_clean_dcache(data, len);

    lcd_dma_state.hspi = hspi;
    lcd_dma_state.complete_cb = complete_cb;
    lcd_dma_state.user_data = user_data;
    lcd_dma_state.busy = true;

    if (HAL_SPI_Transmit_DMA(hspi, data, (uint16_t)len) != HAL_OK) {
        lcd_dma_state.hspi = NULL;
        lcd_dma_state.complete_cb = NULL;
        lcd_dma_state.user_data = NULL;
        lcd_dma_state.busy = false;
        return false;
    }

    return true;
}

void lcd_write_reg(lcd_io* lcdio, uint8_t data)	 
{	
    lcd_io_dc(lcdio, 0);
    lcd_spi_transmit(lcdio->spi, &data, 0x01);
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    lcd_dma_complete_cb_t complete_cb;
    void *user_data;

    if (!lcd_dma_state.busy || lcd_dma_state.hspi != hspi) {
        return;
    }

    complete_cb = lcd_dma_state.complete_cb;
    user_data = lcd_dma_state.user_data;

    lcd_dma_state.hspi = NULL;
    lcd_dma_state.complete_cb = NULL;
    lcd_dma_state.user_data = NULL;
    lcd_dma_state.busy = false;

    if (complete_cb) {
        complete_cb(user_data);
    }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    if (lcd_dma_state.hspi == hspi) {
        lcd_dma_state.hspi = NULL;
        lcd_dma_state.complete_cb = NULL;
        lcd_dma_state.user_data = NULL;
        lcd_dma_state.busy = false;
    }
}