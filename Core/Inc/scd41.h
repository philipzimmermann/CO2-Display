/*
 * scd41.h
 *
 *  Created on: 30.08.2026
 *      Author: phili
 */

#ifndef INC_SCD41_H_
#define INC_SCD41_H_

#include "main.h"
#include <stdint.h>

#define SCD41_I2C_ADDRESS   (0x62 <<1)

typedef enum
{
    SCD41_OK,
    SCD41_NOT_READY,
    SCD41_CRC_ERROR,
    SCD41_I2C_ERROR
} SCD41_Status;

typedef struct {
	uint16_t co2;
	float temperature;
	float humidity;
} SCD41_Data;

SCD41_Status SCD41_Init(I2C_HandleTypeDef *hi2c);
SCD41_Status SCD41_StartMeasurement(I2C_HandleTypeDef *hi2c);
SCD41_Status SCD41_DataReady(I2C_HandleTypeDef *hi2c);
SCD41_Status SCD41_ReadMeasurement(I2C_HandleTypeDef *hi2c, SCD41_Data *scd_data);

#endif /* INC_SCD41_H_ */
