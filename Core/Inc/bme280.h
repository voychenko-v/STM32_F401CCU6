#ifndef BME280_H
#define BME280_H

#include "main.h"

typedef struct {
    int32_t  temp_c100;
    uint32_t press_pa;
    uint32_t hum_q10;
} Bme280Data;

HAL_StatusTypeDef bme280_init(SPI_HandleTypeDef *hspi);
HAL_StatusTypeDef bme280_read(Bme280Data *d);

#endif /* BME280_H */
