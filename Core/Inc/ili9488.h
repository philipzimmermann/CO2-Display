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
#include "scd41.h"

typedef enum {
	ILI9488_OK, ILI9488_NOT_READY, SCD41_SPI_ERROR
} ILI9488_Status;

void ILI9488_Init(SPI_HandleTypeDef *hspi);
void ILI9488_WriteCommand(SPI_HandleTypeDef *hspi, uint8_t cmd);
void ILI9488_WriteData(SPI_HandleTypeDef *hspi, const uint8_t *data,
		uint16_t size);
void ILI9488_Reset();

void ILI9488_SetBrightness(TIM_HandleTypeDef *htim, uint8_t brightness);

void ILI9488_SetAddressWindow(SPI_HandleTypeDef *hspi, uint16_t x0, uint16_t y0,
		uint16_t x1, uint16_t y1);

void ILI9488_FillScreen(SPI_HandleTypeDef *hspi, uint32_t color);
void ILI9488_DrawPixel(SPI_HandleTypeDef *hspi, uint16_t x, uint16_t y,
		uint32_t color);
void ILI9488_DrawLine(SPI_HandleTypeDef *hspi, uint16_t x0, uint16_t y0,
		uint16_t x1, uint16_t y1, uint32_t color);
void ILI9488_FillRectangle(SPI_HandleTypeDef *hspi, int x0, int y0, int x1,
		int y1, uint32_t color);
void ILI9488_DrawRectangle(SPI_HandleTypeDef *hspi, uint16_t x0, uint16_t y0,
		uint16_t x1, uint16_t y1, uint32_t color);
void ILI9488_DrawCircle(SPI_HandleTypeDef *hspi, uint16_t x0, uint16_t y0,
		uint16_t r, uint32_t color);
void ILI9488_FillCircle(SPI_HandleTypeDef *hspi, int x0, int y0, int radius,
		uint32_t color);
void ILI9488_DrawChar(SPI_HandleTypeDef *hspi,
                      int x, int y,
                      char c,
                      uint32_t color,int scale);
void ILI9488_DrawString(SPI_HandleTypeDef *hspi,
                        int x, int y,
                        const char *str,
                        uint32_t color,int scale);
void ILI9488_DrawStringBackground(SPI_HandleTypeDef *hspi,
                        int x, int y,
                        const char *str,
                        uint32_t color,
						uint32_t bg_color);
void ILI9488_DrawDashboardBackground(SPI_HandleTypeDef *hspi);
void ILI9488_DrawDashboardData(SPI_HandleTypeDef *hspi,
                           SCD41_Data scd_data);

#endif /* INC_ILI9488_H_ */
