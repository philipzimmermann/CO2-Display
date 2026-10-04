/*
 * ili9488.c
 *
 *  Created on: 04.10.2026
 *      Author: phili
 */

#include "ili9488.h"
#include "main.h"
#include "stdio.h"

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

void ILI9488_DrawPixel(SPI_HandleTypeDef *hspi, uint16_t x, uint16_t y,
		uint32_t color) {
	uint8_t pixel[3];

	uint8_t r = (color >> 16) & 0xFF;
	uint8_t g = (color >> 8) & 0xFF;
	uint8_t b = color & 0xFF;

	// RGB888 → RGB666
	r &= 0xFC;
	g &= 0xFC;
	b &= 0xFC;

	pixel[0] = r;
	pixel[1] = g;
	pixel[2] = b;

	// Select exactly one pixel
	ILI9488_SetAddressWindow(hspi, x, y, x, y);

	// Write that pixel
	ILI9488_WriteData(hspi, pixel, 3);
}

void ILI9488_FillRectangle(SPI_HandleTypeDef *hspi, uint16_t x0, uint16_t y0,
		uint16_t x1, uint16_t y1, uint32_t color) {
	uint8_t r;
	uint8_t g;
	uint8_t b;

	// RGB888 → 8-bit RGB
	r = (color >> 16) & 0xFF;
	g = (color >> 8) & 0xFF;
	b = color & 0xFF;

	// Keep only the upper 6 bits
	r &= 0xFC;
	g &= 0xFC;
	b &= 0xFC;

	uint8_t buffer[64 * 3];

	// Fill buffer with the same pixel
	for (int i = 0; i < 64; i++) {
		buffer[i * 3 + 0] = r;
		buffer[i * 3 + 1] = g;
		buffer[i * 3 + 2] = b;
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
