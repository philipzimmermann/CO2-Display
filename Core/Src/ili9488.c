/*
 * ili9488.c
 *
 *  Created on: 04.10.2026
 *      Author: phili
 */

#include "ili9488.h"
#include "main.h"
#include "font8x16.h"

#include <stdio.h>
#include <stdlib.h>

static int SCREEN_WIDTH = 480;
static int SCREEN_HEIGHT = 320;

void ILI9488_Init(SPI_HandleTypeDef *hspi) {
	uint8_t data;

	// Hardware reset
	ILI9488_Reset();

	// Exit sleep mode
	ILI9488_WriteCommand(hspi, 0x11);
	HAL_Delay(120);

	// Pixel format: 18 bits per pixel (RGB666)
	ILI9488_WriteCommand(hspi, 0x3A);

	data = 0x66;
	ILI9488_WriteData(hspi, &data, 1);

	// Memory Access Control
	// Landscape orientation + BGR
	ILI9488_WriteCommand(hspi, 0x36);

	data = 0x28;
	ILI9488_WriteData(hspi, &data, 1);

	// Turn display on
	ILI9488_WriteCommand(hspi, 0x29);
	HAL_Delay(20);
}

void ILI9488_WriteCommand(SPI_HandleTypeDef *hspi, uint8_t cmd) {
	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET);

	HAL_SPI_Transmit(hspi, &cmd, 1, HAL_MAX_DELAY);

	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
}

void ILI9488_WriteData(SPI_HandleTypeDef *hspi, const uint8_t *data,
		uint16_t size) {
	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET);

	HAL_SPI_Transmit(hspi, (uint8_t*) data, size, HAL_MAX_DELAY);

	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
}

void ILI9488_Reset() {
	HAL_GPIO_WritePin(LCD_RESET_GPIO_Port, LCD_RESET_Pin, GPIO_PIN_RESET);
	HAL_Delay(10);

	HAL_GPIO_WritePin(LCD_RESET_GPIO_Port, LCD_RESET_Pin, GPIO_PIN_SET);
	HAL_Delay(120);
}

void ILI9488_SetBrightness(TIM_HandleTypeDef *htim, uint8_t brightness) {
	if (brightness > 100)
		brightness = 100;

	uint32_t compare = (brightness * 999) / 100;

	__HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_3, compare);
}

void ILI9488_SetAddressWindow(SPI_HandleTypeDef *hspi, uint16_t x0, uint16_t y0,
		uint16_t x1, uint16_t y1) {
	uint8_t data[4];

	// Column address (X)
	ILI9488_WriteCommand(hspi, 0x2A);

	data[0] = x0 >> 8;
	data[1] = x0 & 0xFF;
	data[2] = x1 >> 8;
	data[3] = x1 & 0xFF;

	ILI9488_WriteData(hspi, data, 4);

	// Page address (Y)
	ILI9488_WriteCommand(hspi, 0x2B);

	data[0] = y0 >> 8;
	data[1] = y0 & 0xFF;
	data[2] = y1 >> 8;
	data[3] = y1 & 0xFF;

	ILI9488_WriteData(hspi, data, 4);

	// Start memory write
	ILI9488_WriteCommand(hspi, 0x2C);
}

void ILI9488_FillScreen(SPI_HandleTypeDef *hspi, uint32_t color) {
	ILI9488_FillRectangle(hspi, 0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1,
			color);
}

static void ColorToRGB666(uint32_t color, uint8_t *pixel) {
	pixel[0] = (color >> 16) & 0xFC;
	pixel[1] = (color >> 8) & 0xFC;
	pixel[2] = color & 0xFC;
}

void ILI9488_DrawPixel(SPI_HandleTypeDef *hspi, uint16_t x, uint16_t y,
		uint32_t color) {
	uint8_t pixel[3];
	ColorToRGB666(color, pixel);

	// Select exactly one pixel
	ILI9488_SetAddressWindow(hspi, x, y, x, y);

	// Write that pixel
	ILI9488_WriteData(hspi, pixel, 3);
}

void ILI9488_FillRectangle(SPI_HandleTypeDef *hspi, int x0, int y0,
		int x1, int y1, uint32_t color) {
	// Sort coordinates
	if (x0 > x1) {
		int temp = x0;
		x0 = x1;
		x1 = temp;
	}

	if (y0 > y1) {
		int temp = y0;
		y0 = y1;
		y1 = temp;
	}

	// Completely outside screen
	if (x1 < 0 || x0 >= 480 || y1 < 0 || y0 >= 320)
		return;

	// Clip to screen
	if (x0 < 0)
		x0 = 0;
	if (y0 < 0)
		y0 = 0;
	if (x1 >= 480)
		x1 = 479;
	if (y1 >= 320)
		y1 = 319;

	uint8_t pixel[3];
	ColorToRGB666(color, pixel);

	uint8_t buffer[64 * 3];

	// Fill buffer with the same pixel
	for (int i = 0; i < 64; i++) {
		buffer[i * 3 + 0] = pixel[0];
		buffer[i * 3 + 1] = pixel[1];
		buffer[i * 3 + 2] = pixel[2];
	}

	// Select rectangle
	ILI9488_SetAddressWindow(hspi, x0, y0, x1, y1);

	// Calculate number of pixels
	uint32_t width = x1 - x0 + 1;
	uint32_t height = y1 - y0 + 1;
	uint32_t pixelsRemaining = width * height;

	while (pixelsRemaining > 0) {
		uint32_t pixelsToSend = pixelsRemaining;

		if (pixelsToSend > 64) {
			pixelsToSend = 64;
		}

		ILI9488_WriteData(hspi, buffer, pixelsToSend * 3);

		pixelsRemaining -= pixelsToSend;
	}
}

void ILI9488_DrawLine(SPI_HandleTypeDef *hspi, uint16_t x0, uint16_t y0,
		uint16_t x1, uint16_t y1, uint32_t color) {
	int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
	int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
	int err = dx + dy, e2; /* error value e_xy */

	for (;;) { /* loop */
		ILI9488_DrawPixel(hspi, x0, y0, color);
		if (x0 == x1 && y0 == y1)
			break;
		e2 = 2 * err;
		if (e2 >= dy) {
			err += dy;
			x0 += sx;
		} /* e_xy+e_x > 0 */
		if (e2 <= dx) {
			err += dx;
			y0 += sy;
		} /* e_xy+e_y < 0 */
	}
}

void ILI9488_DrawRectangle(SPI_HandleTypeDef *hspi, uint16_t x0, uint16_t y0,
		uint16_t x1, uint16_t y1, uint32_t color) {
	ILI9488_DrawLine(hspi, x0, y0, x1, y0, color); // top
	ILI9488_DrawLine(hspi, x0, y1, x1, y1, color); // bottom
	ILI9488_DrawLine(hspi, x0, y0, x0, y1, color); // left
	ILI9488_DrawLine(hspi, x1, y0, x1, y1, color); // right
}

void ILI9488_DrawCircle(SPI_HandleTypeDef *hspi, uint16_t x0, uint16_t y0,
		uint16_t r, uint32_t color) {
	int x = -r, y = 0, err = 2 - 2 * r; /* II. Quadrant */
	do {
		ILI9488_DrawPixel(hspi, x0 - x, y0 + y, color); /*   I. Quadrant */
		ILI9488_DrawPixel(hspi, x0 - y, y0 - x, color); /*  II. Quadrant */
		ILI9488_DrawPixel(hspi, x0 + x, y0 - y, color); /* III. Quadrant */
		ILI9488_DrawPixel(hspi, x0 + y, y0 + x, color); /*  IV. Quadrant */
		int e = err;
		if (e > x)
			err += ++x * 2 + 1; /* e_xy+e_x > 0 */
		if (e <= y)
			err += ++y * 2 + 1; /* e_xy+e_y < 0 */
	} while (x < 0);
}

void ILI9488_FillCircle(SPI_HandleTypeDef *hspi, int x0, int y0, int radius,
		uint32_t color) {
	int x = -radius;
	int y = 0;
	int err = 2 - 2 * radius;

	do {
		ILI9488_FillRectangle(hspi, x0 + x, y0 + y, x0 - x, y0 + y, color);

		ILI9488_FillRectangle(hspi, x0 + y, y0 - x, x0 - y, y0 - x, color);

		ILI9488_FillRectangle(hspi, x0 + x, y0 - y, x0 - x, y0 - y, color);

		ILI9488_FillRectangle(hspi, x0 + y, y0 + x, x0 - y, y0 + x, color);

		int e = err;

		if (e > x)
			err += ++x * 2 + 1;

		if (e <= y)
			err += ++y * 2 + 1;

	} while (x < 0);
}

void ILI9488_DrawChar(SPI_HandleTypeDef *hspi,
                      int x, int y,
                      char c,
                      uint32_t color)
{
    const uint8_t *glyph = font8x16[(uint8_t)c];

    for (int row = 0; row < 16; row++)
    {
        uint8_t bits = glyph[row];

        for (int col = 0; col < 8; col++)
        {
            if (bits & (1 << (7 - col)))
            {
                ILI9488_DrawPixel(hspi,
                                  x + col,
                                  y + row,
                                  color);
            }
        }
    }
}

void ILI9488_DrawString(SPI_HandleTypeDef *hspi,
                        int x, int y,
                        const char *str,
                        uint32_t color)
{
    while (*str)
    {
        ILI9488_DrawChar(hspi, x, y, *str, color);

        x += 8;       // next character
        str++;
    }
}
