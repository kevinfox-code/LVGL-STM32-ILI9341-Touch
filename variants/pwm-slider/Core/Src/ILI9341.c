#include "ILI9341.h"
#include "main.h"

extern SPI_HandleTypeDef ILI9341_SPI;
#define CS_LOW() HAL_GPIO_WritePin(ILI9341_CS_PORT, ILI9341_CS_PIN, GPIO_PIN_RESET)
#define CS_HIGH() HAL_GPIO_WritePin(ILI9341_CS_PORT, ILI9341_CS_PIN, GPIO_PIN_SET)
#define DC_LOW() HAL_GPIO_WritePin(ILI9341_DC_PORT, ILI9341_DC_PIN, GPIO_PIN_RESET)
#define DC_HIGH() HAL_GPIO_WritePin(ILI9341_DC_PORT, ILI9341_DC_PIN, GPIO_PIN_SET)
#define SPI_TIMEOUT_MS 100U
#define DMA_MAX_BYTES 65534U

static uint16_t width = ILI9341_WIDTH;
static uint16_t height = ILI9341_HEIGHT;
static volatile bool dma_done;
static volatile HAL_StatusTypeDef dma_result;
static bool dma_active;
static uint32_t dma_started;

/* Callbacks only publish completion; the foreground owns CS and recovery. */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *spi)
{
    if (spi == &ILI9341_SPI) {
        dma_result = HAL_OK;
        dma_done = true;
    }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *spi)
{
    if (spi == &ILI9341_SPI) {
        dma_result = HAL_ERROR;
        dma_done = true;
    }
}

HAL_StatusTypeDef ILI9341_WaitTransfer(void)
{
    if (!dma_active) return HAL_OK;
    while (!dma_done && (uint32_t)(HAL_GetTick() - dma_started) < SPI_TIMEOUT_MS) {}
    HAL_StatusTypeDef result = dma_done ? dma_result : HAL_TIMEOUT;
    if (result != HAL_OK) {
        /* Stop DMA before LVGL can reuse its source buffer. */
        if (HAL_SPI_Abort(&ILI9341_SPI) != HAL_OK) Error_Handler();
    }
    CS_HIGH();
    dma_active = false;
    return result;
}

static HAL_StatusTypeDef write_bytes(uint8_t *data, uint16_t size, bool command)
{
    HAL_StatusTypeDef result = ILI9341_WaitTransfer();
    if (result != HAL_OK) return result;
    if (command) DC_LOW(); else DC_HIGH();
    CS_LOW();
    result = HAL_SPI_Transmit(&ILI9341_SPI, data, size, SPI_TIMEOUT_MS);
    CS_HIGH();
    return result;
}

HAL_StatusTypeDef ILI9341_WriteCommand(uint8_t cmd) { return write_bytes(&cmd, 1, true); }
HAL_StatusTypeDef ILI9341_WriteData(uint8_t data) { return write_bytes(&data, 1, false); }
HAL_StatusTypeDef ILI9341_WriteData16(uint16_t data)
{
    uint8_t bytes[2] = {data >> 8, data & 0xff};
    return write_bytes(bytes, sizeof(bytes), false);
}

static void ILI9341_Reset(void)
{
    CS_HIGH();
    HAL_GPIO_WritePin(ILI9341_RESET_PORT, ILI9341_RESET_PIN, GPIO_PIN_RESET);
    HAL_Delay(20);
    HAL_GPIO_WritePin(ILI9341_RESET_PORT, ILI9341_RESET_PIN, GPIO_PIN_SET);
    HAL_Delay(150);
}

HAL_StatusTypeDef ILI9341_Init(void) {
    ILI9341_Reset();

    if (ILI9341_WriteCommand(0x01) != HAL_OK) return HAL_ERROR;
    HAL_Delay(10);
    if (ILI9341_WriteCommand(0x28) != HAL_OK) return HAL_ERROR;

    if (ILI9341_WriteCommand(0xCF) != HAL_OK) return HAL_ERROR;
    if (ILI9341_WriteData(0x00) != HAL_OK) return HAL_ERROR;
    if (ILI9341_WriteData(0xC1) != HAL_OK) return HAL_ERROR;
    if (ILI9341_WriteData(0x30) != HAL_OK) return HAL_ERROR;

    if (ILI9341_WriteCommand(0xED) != HAL_OK) return HAL_ERROR;
    if (ILI9341_WriteData(0x64) != HAL_OK) return HAL_ERROR;
    if (ILI9341_WriteData(0x03) != HAL_OK) return HAL_ERROR;
    if (ILI9341_WriteData(0x12) != HAL_OK) return HAL_ERROR;
    if (ILI9341_WriteData(0x81) != HAL_OK) return HAL_ERROR;

    if (ILI9341_WriteCommand(0xE8) != HAL_OK) return HAL_ERROR;
    if (ILI9341_WriteData(0x85) != HAL_OK) return HAL_ERROR;
    if (ILI9341_WriteData(0x00) != HAL_OK) return HAL_ERROR;
    if (ILI9341_WriteData(0x78) != HAL_OK) return HAL_ERROR;

    if (ILI9341_WriteCommand(0xCB) != HAL_OK) return HAL_ERROR;
    if (ILI9341_WriteData(0x39) != HAL_OK) return HAL_ERROR;
    if (ILI9341_WriteData(0x2C) != HAL_OK) return HAL_ERROR;
    if (ILI9341_WriteData(0x00) != HAL_OK) return HAL_ERROR;
    if (ILI9341_WriteData(0x34) != HAL_OK) return HAL_ERROR;
    if (ILI9341_WriteData(0x02) != HAL_OK) return HAL_ERROR;

    if (ILI9341_WriteCommand(0xF7) != HAL_OK) return HAL_ERROR;
    if (ILI9341_WriteData(0x20) != HAL_OK) return HAL_ERROR;

    if (ILI9341_WriteCommand(0xEA) != HAL_OK) return HAL_ERROR;
    if (ILI9341_WriteData(0x00) != HAL_OK) return HAL_ERROR;
    if (ILI9341_WriteData(0x00) != HAL_OK) return HAL_ERROR;

    if (ILI9341_WriteCommand(0xC0) != HAL_OK) return HAL_ERROR;
    if (ILI9341_WriteData(0x23) != HAL_OK) return HAL_ERROR;

    if (ILI9341_WriteCommand(0xC1) != HAL_OK) return HAL_ERROR;
    if (ILI9341_WriteData(0x10) != HAL_OK) return HAL_ERROR;

    if (ILI9341_WriteCommand(0xC5) != HAL_OK) return HAL_ERROR;
    if (ILI9341_WriteData(0x3e) != HAL_OK) return HAL_ERROR;
    if (ILI9341_WriteData(0x28) != HAL_OK) return HAL_ERROR;

    if (ILI9341_WriteCommand(0xC7) != HAL_OK) return HAL_ERROR;
    if (ILI9341_WriteData(0x86) != HAL_OK) return HAL_ERROR;


    if (ILI9341_WriteCommand(0x3A) != HAL_OK) return HAL_ERROR;
    if (ILI9341_WriteData(0x55) != HAL_OK) return HAL_ERROR;

    if (ILI9341_WriteCommand(0xB1) != HAL_OK) return HAL_ERROR;
    if (ILI9341_WriteData(0x00) != HAL_OK) return HAL_ERROR;
    if (ILI9341_WriteData(0x18) != HAL_OK) return HAL_ERROR;

    if (ILI9341_WriteCommand(0xB6) != HAL_OK) return HAL_ERROR;
    if (ILI9341_WriteData(0x08) != HAL_OK) return HAL_ERROR;
    if (ILI9341_WriteData(0x82) != HAL_OK) return HAL_ERROR;
    if (ILI9341_WriteData(0x27) != HAL_OK) return HAL_ERROR;

    if (ILI9341_WriteCommand(0xF2) != HAL_OK) return HAL_ERROR;
    if (ILI9341_WriteData(0x00) != HAL_OK) return HAL_ERROR;

    if (ILI9341_WriteCommand(0x26) != HAL_OK) return HAL_ERROR;
    if (ILI9341_WriteData(0x01) != HAL_OK) return HAL_ERROR;

    if (ILI9341_WriteCommand(0x11) != HAL_OK) return HAL_ERROR;
    HAL_Delay(120);
    if (ILI9341_WriteCommand(0x29) != HAL_OK) return HAL_ERROR;

    return ILI9341_SetRotation(3);
}

HAL_StatusTypeDef ILI9341_SetRotation(uint8_t rotation)
{
    static const uint8_t madctl[] = {0x48, 0x28, 0x88, 0xE8};
    if (rotation > 3) return HAL_ERROR;
    HAL_StatusTypeDef result = ILI9341_WriteCommand(0x36);
    if (result == HAL_OK) result = ILI9341_WriteData(madctl[rotation]);
    if (result == HAL_OK) {
        width = (rotation & 1U) ? ILI9341_HEIGHT : ILI9341_WIDTH;
        height = (rotation & 1U) ? ILI9341_WIDTH : ILI9341_HEIGHT;
    }
    return result;
}

HAL_StatusTypeDef ILI9341_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    if (x0 > x1 || y0 > y1 || x1 >= width || y1 >= height) return HAL_ERROR;
    HAL_StatusTypeDef result = ILI9341_WriteCommand(0x2A);
    if (result == HAL_OK) result = ILI9341_WriteData16(x0);
    if (result == HAL_OK) result = ILI9341_WriteData16(x1);
    if (result == HAL_OK) result = ILI9341_WriteCommand(0x2B);
    if (result == HAL_OK) result = ILI9341_WriteData16(y0);
    if (result == HAL_OK) result = ILI9341_WriteData16(y1);
    return result;
}

HAL_StatusTypeDef ILI9341_DrawBitmap(uint16_t w, uint16_t h, uint8_t *pixels)
{
    if (w == 0 || h == 0) return HAL_OK;
    if (!pixels || w > width || h > height) return HAL_ERROR;
    HAL_StatusTypeDef result = ILI9341_WriteCommand(0x2C);
    if (result != HAL_OK) return result;
    uint32_t remaining = (uint32_t)w * h * 2U;
    DC_HIGH();
    CS_LOW();
    while (remaining && result == HAL_OK) {
        uint16_t count = remaining > DMA_MAX_BYTES ? DMA_MAX_BYTES : (uint16_t)remaining;
        result = HAL_SPI_Transmit(&ILI9341_SPI, pixels, count, SPI_TIMEOUT_MS);
        pixels += count;
        remaining -= count;
    }
    CS_HIGH();
    return result;
}

HAL_StatusTypeDef ILI9341_DrawBitmapDMA(uint16_t w, uint16_t h, uint8_t *pixels)
{
    if (dma_active) return HAL_BUSY;
    if (w == 0 || h == 0) return HAL_OK;
    uint32_t count = (uint32_t)w * h * 2U;
    if (!pixels || w > width || h > height || count > DMA_MAX_BYTES) return HAL_ERROR;
    HAL_StatusTypeDef result = ILI9341_WriteCommand(0x2C);
    if (result != HAL_OK) return result;
    DC_HIGH();
    CS_LOW();
    dma_done = false;
    dma_result = HAL_OK;
    dma_started = HAL_GetTick();
    dma_active = true;
    result = HAL_SPI_Transmit_DMA(&ILI9341_SPI, pixels, (uint16_t)count);
    if (result != HAL_OK) {
        dma_active = false;
        CS_HIGH();
    }
    return result;
}

HAL_StatusTypeDef ILI9341_FillScreen(uint16_t color)
{
    static uint8_t row[ILI9341_HEIGHT * 2U];
    HAL_StatusTypeDef result = ILI9341_SetWindow(0, 0, width - 1U, height - 1U);
    if (result != HAL_OK) return result;
    for (uint16_t i = 0; i < width; ++i) {
        row[2U * i] = color >> 8;
        row[2U * i + 1U] = color & 0xff;
    }
    result = ILI9341_WriteCommand(0x2C);
    if (result != HAL_OK) return result;
    DC_HIGH();
    CS_LOW();
    for (uint16_t y = 0; y < height && result == HAL_OK; ++y)
        result = HAL_SPI_Transmit(&ILI9341_SPI, row, width * 2U, SPI_TIMEOUT_MS);
    CS_HIGH();
    return result;
}

HAL_StatusTypeDef ILI9341_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    HAL_StatusTypeDef result = ILI9341_SetWindow(x, y, x, y);
    if (result == HAL_OK) result = ILI9341_WriteCommand(0x2C);
    if (result == HAL_OK) result = ILI9341_WriteData16(color);
    return result;
}
