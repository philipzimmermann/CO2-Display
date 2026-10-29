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
#include <string.h>

static const int SCREEN_WIDTH = 320;
static const int SCREEN_HEIGHT = 480;

static const int FONT_HEIGHT = 16;
static const int FONT_WIDTH = 8;

static const int COLOR_BG = 0x000000;
static const int COLOR_FONT = 0xFFFFFF;

static const int Y_HEADER_START = 0;
static const int Y_HEADER_TEXT = 25;
static const int Y_HEADER_END = 65;

static const int Y_CO2_START = 85;
static const int Y_CO2_TEXT = 110;
static const int Y_CO2_VALUE = 140;
static const int Y_CO2_END = 185;

static const int Y_TEMP_START = 205;
static const int Y_TEMP_TEXT = 230;
static const int Y_TEMP_VALUE = 260;
static const int Y_TEMP_END = 305;

static const int Y_HUM_START = 325;
static const int Y_HUM_TEXT = 350;
static const int Y_HUM_VALUE = 380;
static const int Y_HUM_END = 425;

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
	// Portrait orientation + BGR
	ILI9488_WriteCommand(hspi, 0x36);

	data = 0x88;
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

void ILI9488_FillRectangle(SPI_HandleTypeDef *hspi, int x0, int y0, int x1,
		int y1, uint32_t color) {
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
	if (x1 < 0 || x0 >= SCREEN_WIDTH || y1 < 0 || y0 >= SCREEN_HEIGHT)
		return;

	// Clip to screen
	if (x0 < 0)
		x0 = 0;
	if (y0 < 0)
		y0 = 0;
	if (x1 >= SCREEN_WIDTH)
		x1 = SCREEN_WIDTH - 1;
	if (y1 >= SCREEN_HEIGHT)
		y1 = SCREEN_HEIGHT - 1;

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

void ILI9488_DrawChar(SPI_HandleTypeDef *hspi, int x, int y, char c,
		uint32_t color, int scale) {
	if (scale < 1) {
		return;
	}

	const uint8_t *glyph = font8x16[(uint8_t) c];

	for (int row = 0; row < 16; row++) {
		uint8_t bits = glyph[row];

		for (int col = 0; col < 8; col++) {
			if (bits & (1 << (7 - col))) {
				for (int dy = 0; dy < scale; dy++) {
					for (int dx = 0; dx < scale; dx++) {
						ILI9488_DrawPixel(hspi, x + col * scale + dx,
								y + row * scale + dy, color);
					}
				}
			}
		}
	}
}

void ILI9488_DrawString(SPI_HandleTypeDef *hspi, int x, int y, const char *str,
		uint32_t color, int scale) {
	if (scale < 1) {
		return;
	}

	while (*str) {
		ILI9488_DrawChar(hspi, x, y, *str, color, scale);
		x += 8 * scale;
		str++;
	}
}

void ILI9488_DrawStringBackground(SPI_HandleTypeDef *hspi, int x, int y,
		const char *str, uint32_t color, uint32_t bg_color) {
	int length = strlen(str);
	ILI9488_FillRectangle(hspi, x, y, x + length * FONT_WIDTH, y + FONT_HEIGHT,
			bg_color);
	ILI9488_DrawString(hspi, x, y, str, color,1);
}

void ILI9488_DrawDashboardBackground(SPI_HandleTypeDef *hspi) {
	// Background
	ILI9488_FillScreen(hspi, COLOR_BG);

	// -------------------------
	// Header
	// -------------------------

	ILI9488_FillRectangle(hspi, 0, Y_HEADER_START, 319, Y_HEADER_END, COLOR_BG);

	const char *title = "AIR MONITOR";
	ILI9488_DrawString(hspi,
			SCREEN_WIDTH / 2 - (strlen(title) * FONT_WIDTH), Y_HEADER_TEXT,
			title, COLOR_FONT, 2);

	// -------------------------
	// CO2
	// -------------------------
	ILI9488_DrawRectangle(hspi, 20, Y_CO2_START, 299, Y_CO2_END, COLOR_FONT);

	const char *co2_text = "CO\x01 concentration";
	ILI9488_DrawString(hspi, 70, Y_CO2_TEXT, co2_text, COLOR_FONT,1);

	ILI9488_DrawString(hspi, 70, Y_CO2_VALUE, "Measuring", COLOR_FONT,1);

	// -------------------------
	// Temperature panel
	// -------------------------

	ILI9488_DrawRectangle(hspi, 20, Y_TEMP_START, 299, Y_TEMP_END, COLOR_FONT);

	ILI9488_DrawString(hspi, 70, Y_TEMP_TEXT, "Temperature", COLOR_FONT,1);

	ILI9488_DrawString(hspi, 70, Y_TEMP_VALUE, "Measuring", COLOR_FONT,1);

	// -------------------------
	// Humidity panel
	// -------------------------

	ILI9488_DrawRectangle(hspi, 20, Y_HUM_START, 299, Y_HUM_END, COLOR_FONT);

	ILI9488_DrawString(hspi, 70, Y_HUM_TEXT, "Humidity", COLOR_FONT,1);

	ILI9488_DrawString(hspi, 70, Y_HUM_VALUE, "Measuring", COLOR_FONT,1);
}

void ILI9488_DrawDashboardData(SPI_HandleTypeDef *hspi, SCD41_Data scd_data) {
	char co2_str[32];
	char temp_str[32];
	char humidity_str[32];
	// -------------------------
	// CO2
	// -------------------------
	ILI9488_FillRectangle(hspi, 70, Y_CO2_VALUE, 70 + 10 * FONT_WIDTH,
			Y_CO2_VALUE + FONT_HEIGHT, COLOR_BG);
	sprintf(co2_str, "%d ppm", scd_data.co2);
	ILI9488_DrawString(hspi, 70, Y_CO2_VALUE, co2_str, COLOR_FONT,1);

	// -------------------------
	// Temperature panel
	// -------------------------
	ILI9488_FillRectangle(hspi, 70, Y_TEMP_VALUE, 70 + 10 * FONT_WIDTH,
			Y_TEMP_VALUE + FONT_HEIGHT, COLOR_BG);
	sprintf(temp_str, "%.1f\x02 C", scd_data.temperature);
	ILI9488_DrawString(hspi, 70, Y_TEMP_VALUE, temp_str, COLOR_FONT,1);

	// -------------------------
	// Humidity panel
	// -------------------------
	ILI9488_FillRectangle(hspi, 70, Y_HUM_VALUE, 70 + 10 * FONT_WIDTH,
			Y_HUM_VALUE + FONT_HEIGHT, COLOR_BG);
	sprintf(humidity_str, "%.1f %%", scd_data.humidity);
	ILI9488_DrawString(hspi, 70, Y_HUM_VALUE, humidity_str, COLOR_FONT,1);
}
