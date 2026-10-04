/*
 * ssd1306.c
 *
 *  Created on: 25.08.2026
 *      Author: phili
 */

#include "ssd1306.h"

#include <string.h>
#include "font8x16.h"

#define SSD1306_WIDTH  128
#define SSD1306_HEIGHT 64

static uint8_t framebuffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];

static HAL_StatusTypeDef SSD1306_WriteCommand(I2C_HandleTypeDef *hi2c,
		uint8_t command) {
	uint8_t data[2] = { 0x00,       // Control byte: command
			command };

	return HAL_I2C_Master_Transmit(hi2c,
	SSD1306_I2C_ADDRESS, data, 2, 100);
}

HAL_StatusTypeDef SSD1306_Init(I2C_HandleTypeDef *hi2c) {
	HAL_Delay(100);

	HAL_StatusTypeDef status;

	status = HAL_I2C_IsDeviceReady(hi2c, SSD1306_I2C_ADDRESS, 3, 100); // Check I2C Device ready
	if (status != HAL_OK)
		return status;

	status = SSD1306_WriteCommand(hi2c, 0xAE); // Display OFF
	if (status != HAL_OK)
		return status;

	status = SSD1306_WriteCommand(hi2c, 0xD5); // Display clock
	if (status != HAL_OK)
		return status;

	status = SSD1306_WriteCommand(hi2c, 0x80);
	if (status != HAL_OK)
		return status;

	status = SSD1306_WriteCommand(hi2c, 0xA8); // Multiplex ratio
	if (status != HAL_OK)
		return status;

	status = SSD1306_WriteCommand(hi2c, 0x3F); // 64 rows
	if (status != HAL_OK)
		return status;

	status = SSD1306_WriteCommand(hi2c, 0xD3); // Display offset
	if (status != HAL_OK)
		return status;

	status = SSD1306_WriteCommand(hi2c, 0x00);
	if (status != HAL_OK)
		return status;

	status = SSD1306_WriteCommand(hi2c, 0x40); // Start line
	if (status != HAL_OK)
		return status;

	status = SSD1306_WriteCommand(hi2c, 0x8D); // Charge pump
	if (status != HAL_OK)
		return status;

	status = SSD1306_WriteCommand(hi2c, 0x14); // Enable charge pump
	if (status != HAL_OK)
		return status;

	status = SSD1306_WriteCommand(hi2c, 0x20); // Page addressing mode
	if (status != HAL_OK)
		return status;

	status = SSD1306_WriteCommand(hi2c, 0x02);
	if (status != HAL_OK)
		return status;

	status = SSD1306_WriteCommand(hi2c, 0xA1); // Segment remap
	if (status != HAL_OK)
		return status;

	status = SSD1306_WriteCommand(hi2c, 0xC8); // COM scan direction
	if (status != HAL_OK)
		return status;

	status = SSD1306_WriteCommand(hi2c, 0xDA); // COM pins
	if (status != HAL_OK)
		return status;

	status = SSD1306_WriteCommand(hi2c, 0x12);
	if (status != HAL_OK)
		return status;

	status = SSD1306_WriteCommand(hi2c, 0x81); // Contrast
	if (status != HAL_OK)
		return status;

	status = SSD1306_WriteCommand(hi2c, 0x7F);
	if (status != HAL_OK)
		return status;

	status = SSD1306_WriteCommand(hi2c, 0xD9); // Pre-charge
	if (status != HAL_OK)
		return status;

	status = SSD1306_WriteCommand(hi2c, 0xF1);
	if (status != HAL_OK)
		return status;

	status = SSD1306_WriteCommand(hi2c, 0xDB); // VCOMH
	if (status != HAL_OK)
		return status;

	status = SSD1306_WriteCommand(hi2c, 0x40);
	if (status != HAL_OK)
		return status;

	status = SSD1306_WriteCommand(hi2c, 0xA4); // Display follows RAM
	if (status != HAL_OK)
		return status;

	status = SSD1306_WriteCommand(hi2c, 0xA6); // Normal display
	if (status != HAL_OK)
		return status;

	status = SSD1306_WriteCommand(hi2c, 0xAF); // Display ON

	return status;
}

void SSD1306_FillWhite() {
	memset(framebuffer, 0xFF, sizeof(framebuffer));
}

void SSD1306_FillBlack() {
	memset(framebuffer, 0x00, sizeof(framebuffer));
}

void SSD1306_DrawPixel(uint8_t x, uint8_t y, uint8_t color) {
	if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) {
		return;
	}

	uint16_t index = x + (y / 8) * SSD1306_WIDTH;
	uint8_t bit = y % 8;

	if (color) {
		framebuffer[index] |= (1 << bit);
	} else {
		framebuffer[index] &= ~(1 << bit);
	}
}

void SSD1306_DrawBitmap(uint8_t x_offset, uint8_t y_offset,
		const uint64_t *bitmap, uint8_t width, uint8_t height, uint8_t scale) {
	for (uint8_t y = 0; y < height; y++) {
		for (uint8_t x = 0; x < width; x++) {
			if (bitmap[y] & (1ULL << (width - 1 - x))) {
				for (uint8_t dy = 0; dy < scale; dy++) {
					for (uint8_t dx = 0; dx < scale; dx++) {
						SSD1306_DrawPixel(x_offset + x * scale + dx,
								y_offset + y * scale + dy, 1);
					}
				}
			}
		}
	}
}

void SSD1306_DrawChar(uint8_t x, uint8_t y, char c) {
	const uint8_t *glyph = font8x16[(uint8_t) c];
	for (uint8_t row = 0; row < 16; row++) {
		for (uint8_t col = 0; col < 8; col++) {
			if (glyph[row] & (1 << (7-col))) {
				SSD1306_DrawPixel(x + col, y + row, 1);
			}
		}
	}
}

void SSD1306_DrawString(uint8_t x, uint8_t y, const char *str) {
	for (int i = 0; i < strlen(str); i++) {
		SSD1306_DrawChar(x+(i*9),y,str[i]);
	}
}

HAL_StatusTypeDef SSD1306_UpdateScreen(I2C_HandleTypeDef *hi2c) {
	for (uint8_t page = 0; page < 8; page++) {
		HAL_StatusTypeDef status;

		status = SSD1306_WriteCommand(hi2c, 0xB0 + page);
		if (status != HAL_OK) {
			return status;
		}

		status = SSD1306_WriteCommand(hi2c, 0x00);
		if (status != HAL_OK) {
			return status;
		}

		status = SSD1306_WriteCommand(hi2c, 0x10);
		if (status != HAL_OK) {
			return status;
		}

		uint8_t data[129];
		data[0] = 0x40;

		for (uint8_t x = 0; x < 128; x++) {
			data[x + 1] = framebuffer[page * 128 + x];
		}

		status = HAL_I2C_Master_Transmit(hi2c,
		SSD1306_I2C_ADDRESS, data, sizeof(data), 100);

		if (status != HAL_OK) {
			return status;
		}
	}

	return HAL_OK;
}
