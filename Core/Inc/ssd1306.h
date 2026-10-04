/*
 * ssd1306.h
 *
 *  Created on: 25.08.2026
 *      Author: phili
 */

#ifndef INC_SSD1306_H_
#define INC_SSD1306_H_

#include "main.h"

#define SSD1306_I2C_ADDRESS    (0x3C << 1)

HAL_StatusTypeDef SSD1306_Init(I2C_HandleTypeDef *hi2c);

void SSD1306_FillWhite();
void SSD1306_FillBlack();
void SSD1306_DrawPixel(uint8_t x, uint8_t y, uint8_t color);
void SSD1306_DrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
void SSD1306_DrawRectangle(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
void SSD1306_DrawBitmap(uint8_t x, uint8_t y, const uint64_t *bitmap, uint8_t width, uint8_t height, uint8_t scale);
void SSD1306_DrawChar(uint8_t x, uint8_t y, char c);
void SSD1306_DrawString(uint8_t x, uint8_t y, const char *str);

HAL_StatusTypeDef SSD1306_UpdateScreen(I2C_HandleTypeDef *hi2c);


#endif /* INC_SSD1306_H_ */
