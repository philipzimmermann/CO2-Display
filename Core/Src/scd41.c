/*
 * scd41.c
 *
 *  Created on: 30.08.2026
 *      Author: phili
 */

#include "scd41.h"

#define START_PERIODIC_MEASUREMENT 0x21B1
#define GET_DATA_READY_STATUS 0xE4B8
#define READ_MEASUREMENT 0xEC05

#define CRC8_POLYNOMIAL 0x31
#define CRC8_INIT 0xff

static uint8_t SCD41_CRC8(uint8_t *data) {
	uint8_t crc = CRC8_INIT;

	for (uint8_t current_byte = 0; current_byte < 2; ++current_byte) {
		crc ^= data[current_byte];

		for (uint8_t bit = 0; bit < 8; bit++) {
			if (crc & 0x80)
				crc = (crc << 1) ^ CRC8_POLYNOMIAL;
			else
				crc <<= 1;
		}
	}

	return crc;
}

static SCD41_Status SCD41_SendCommand(I2C_HandleTypeDef *hi2c, uint16_t command) {
	uint8_t cmd[2];

	cmd[0] = (uint8_t) (command >> 8);
	cmd[1] = (uint8_t) (command & 0xFF);

	if (HAL_I2C_Master_Transmit(hi2c,
	SCD41_I2C_ADDRESS, cmd, 2,
	HAL_MAX_DELAY) == HAL_OK) {
		return SCD41_OK;
	}
	return SCD41_I2C_ERROR;
}

SCD41_Status SCD41_Init(I2C_HandleTypeDef *hi2c) {
	if(HAL_I2C_IsDeviceReady(hi2c,
	SCD41_I2C_ADDRESS, 3, 100)==HAL_OK){
		return SCD41_OK;
	}
	return SCD41_I2C_ERROR;
}

SCD41_Status SCD41_StartMeasurement(I2C_HandleTypeDef *hi2c) {
	return SCD41_SendCommand(hi2c, START_PERIODIC_MEASUREMENT);
}

SCD41_Status SCD41_DataReady(I2C_HandleTypeDef *hi2c) {
	SCD41_Status status;
	status = SCD41_SendCommand(hi2c, GET_DATA_READY_STATUS);

	if (status != SCD41_OK)
		return status;

	uint8_t data[3];
	status = HAL_I2C_Master_Receive(hi2c,
	SCD41_I2C_ADDRESS, data, 3,
	HAL_MAX_DELAY);

	if (status != SCD41_OK)
		return status;

	if (SCD41_CRC8(data) != data[2])
		return SCD41_CRC_ERROR;

	uint16_t status_word = ((uint16_t) data[0] << 8) | data[1];

	status_word &= 0x07FF;

	if (status_word == 0)
		return SCD41_NOT_READY;

	return SCD41_OK;
}

SCD41_Status SCD41_ReadMeasurement(I2C_HandleTypeDef *hi2c, SCD41_Data *scd_data) {

	SCD41_Status status;

	status = SCD41_SendCommand(hi2c, READ_MEASUREMENT);
	if (status != SCD41_OK)
		return status;

	HAL_Delay(1);

	uint8_t data[9];
	status = HAL_I2C_Master_Receive(hi2c,
	SCD41_I2C_ADDRESS, data, 9,
	HAL_MAX_DELAY);

	if (status != SCD41_OK)
		return status;

	// Check CRCs
	if (SCD41_CRC8(&data[0]) != data[2])
		return SCD41_CRC_ERROR;

	if (SCD41_CRC8(&data[3]) != data[5])
		return SCD41_CRC_ERROR;

	if (SCD41_CRC8(&data[6]) != data[8])
		return SCD41_CRC_ERROR;

	// CO2
	scd_data->co2 = ((uint16_t) data[0] << 8) | data[1];

	// Temperature
	uint16_t raw_temperature = ((uint16_t) data[3] << 8) | data[4];

	scd_data->temperature = -45.0f + 175.0f * ((float) raw_temperature / 65535.0f);

	// Humidity
	uint16_t raw_humidity = ((uint16_t) data[6] << 8) | data[7];

	scd_data->humidity = 100.0f * ((float) raw_humidity / 65535.0f);

	return SCD41_OK;
}

