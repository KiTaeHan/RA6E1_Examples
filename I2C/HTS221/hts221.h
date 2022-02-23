#ifndef HTS221_HTS221_H_
#define HTS221_HTS221_H_

#include "hal_data.h"

#define I2C_TRANSACTION_BUSY_DELAY 100

#define HTS221_ON_SET   1
#define HTS221_NOT_SET  0

typedef enum {
  HTS221_ONE_SHOT  = 0,
  HTS221_ODR_1Hz   = 1,
  HTS221_ODR_7Hz   = 2,
  HTS221_ODR_12Hz5 = 3,
  HTS221_ODR_ND    = 4,
} HTS221_ODR_T;

bool HTS221_PowerOnSet(uint8_t set);
bool HTS221_BDataSet(uint8_t set);
bool HTS221_DataRateSet(HTS221_ODR_T odr);
bool HTS221_IsWork();
bool HTS221_Init(HTS221_ODR_T odr);
bool HTS221_GetHumidity(float* hum);
bool HTS221_GetTemperature(float*tmp);

#endif /* HTS221_HTS221_H_ */
