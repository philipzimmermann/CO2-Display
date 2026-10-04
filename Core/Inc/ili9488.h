/*
 * ili9488.h
 *
 *  Created on: 04.10.2026
 *      Author: phili
 */

#ifndef INC_ILI9488_H_
#define INC_ILI9488_H_

#include "stm32f4xx_hal.h"
#include <stdint.h>

typedef enum {
	ILI9488_OK, ILI9488_NOT_READY, SCD41_SPI_ERROR
} ILI9488_Status;

void ILI9488_Init(SPI_HandleTypeDef *hspi);
void ILI9488_WriteCommand(SPI_HandleTypeDef *hspi, uint8_t cmd);
void ILI9488_WriteData(SPI_HandleTypeDef *hspi, const uint8_t *data,
		uint16_t size);
void ILI9488_Reset();

void ILI9488_SetAddressWindow(SPI_HandleTypeDef *hspi, uint16_t x0, uint16_t y0,
		uint16_t x1, uint16_t y1);

void ILI9488_FillScreen(SPI_HandleTypeDef *hspi, uint32_t color);
void ILI9488_DrawPixel(SPI_HandleTypeDef *hspi, uint16_t x, uint16_t y,
		uint32_t color);
void ILI9488_FillRectangle(SPI_HandleTypeDef *hspi, uint16_t x0, uint16_t y0,
		uint16_t x1, uint16_t y1, uint32_t color);

#endif /* INC_ILI9488_H_ */
