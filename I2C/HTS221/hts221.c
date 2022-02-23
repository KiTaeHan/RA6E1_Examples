#include <string.h>

#include "hts221.h"

#include "RTT/SEGGER_RTT.h"

#define HTS221_WHO_AM_I         0x0FU
#define HTS221_CTRL_REG1        0x20U
#define HTS221_HUMIDITY_OUT_L   0x28U
#define HTS221_TEMP_OUT_L       0x2AU
#define HTS221_H0_RH_X2         0x30U
#define HTS221_H1_RH_X2         0x31U
#define HTS221_T0_DEGC_X8       0x32U
#define HTS221_T1_DEGC_X8       0x33U
#define HTS221_T1_T0_MSB        0x35U
#define HTS221_H0_T0_OUT_L      0x36U
#define HTS221_H1_T0_OUT_L      0x3AU
#define HTS221_T0_OUT_L         0x3CU
#define HTS221_T1_OUT_L         0x3EU

#define HTS221_ID               0xBCU

static fsp_err_t I2C0_Write(uint8_t reg, uint8_t* data,  uint32_t size, uint32_t timeout);
static fsp_err_t I2C0_Read(uint8_t reg, uint8_t* data,  uint32_t size, uint32_t timeout);

volatile i2c_master_event_t     g_i2c_callback_event;

void I2C0_Callback(i2c_master_callback_args_t *p_args)
{
    g_i2c_callback_event = p_args->event;
}

static fsp_err_t I2C0_Write(uint8_t reg, uint8_t* data, uint32_t size, uint32_t timeout)
{
    fsp_err_t err;
    uint8_t datas[size+1];

    datas[0] = reg;
    if(size) memcpy(&datas[1], data, size);

    g_i2c_callback_event = I2C_MASTER_EVENT_ABORTED;
    err = R_IIC_MASTER_Write(&g_i2c_master0_ctrl, datas, size+1, false);
    if (FSP_SUCCESS != err)
    {
        return err;
    }
    while ((I2C_MASTER_EVENT_TX_COMPLETE != g_i2c_callback_event) && timeout)
    {
        R_BSP_SoftwareDelay(1U, BSP_DELAY_UNITS_MILLISECONDS);
        timeout--;
    }

    if(!timeout)
    {
        return FSP_ERR_TIMEOUT;
    }

    return err;
}


static fsp_err_t I2C0_Read(uint8_t reg, uint8_t* datas, uint32_t size, uint32_t timeout)
{
    fsp_err_t err;
    uint8_t data;
    uint8_t i;
    uint32_t out;

    for(i=0; i<size; i++, reg++)
    {
        out = timeout;

        g_i2c_callback_event = I2C_MASTER_EVENT_ABORTED;
        err = R_IIC_MASTER_Write(&g_i2c_master0_ctrl, &reg, 1, true);
        if (FSP_SUCCESS != err)
        {
            return err;
        }
        while ((I2C_MASTER_EVENT_TX_COMPLETE != g_i2c_callback_event) && out)
        {
            R_BSP_SoftwareDelay(1U, BSP_DELAY_UNITS_MILLISECONDS);
            out--;
        }

        g_i2c_callback_event = I2C_MASTER_EVENT_ABORTED;
        err = R_IIC_MASTER_Read(&g_i2c_master0_ctrl, &data, 1, false);
        if (FSP_SUCCESS != err)
        {
           return err;
        }

        while ((I2C_MASTER_EVENT_RX_COMPLETE != g_i2c_callback_event) && out)
        {
            R_BSP_SoftwareDelay(1U, BSP_DELAY_UNITS_MILLISECONDS);
            out--;
        }

        if(!out)
        {
           return FSP_ERR_TIMEOUT;
        }

        datas[i] = data;
    }

    return err;
}


bool HTS221_PowerOnSet(uint8_t set)
{
    uint8_t reg;
    uint8_t data;
    fsp_err_t err;

    reg = HTS221_CTRL_REG1;
    err = I2C0_Read(reg, &data, 1, I2C_TRANSACTION_BUSY_DELAY);
    if (FSP_SUCCESS != err)
    {
        SEGGER_RTT_printf(0, "I2C0 error\r\n");
        return false;
    }
    data = (set<<7) | (data & 0x7F);
    err = I2C0_Write(reg, &data, 1, I2C_TRANSACTION_BUSY_DELAY);
    if (FSP_SUCCESS != err)
    {
        SEGGER_RTT_printf(0, "I2C0 error\r\n");
        return false;
    }

    return true;
}

bool HTS221_BDataSet(uint8_t set)
{
    uint8_t reg;
    uint8_t data;
    fsp_err_t err;

    reg = HTS221_CTRL_REG1;
    err = I2C0_Read(reg, &data, 1, I2C_TRANSACTION_BUSY_DELAY);
    if (FSP_SUCCESS != err)
    {
        SEGGER_RTT_printf(0, "I2C0 error\r\n");
        return false;
    }
    data = (set<<2) | (data & 0xFB);
    err = I2C0_Write(reg, &data, 1, I2C_TRANSACTION_BUSY_DELAY);
    if (FSP_SUCCESS != err)
    {
        SEGGER_RTT_printf(0, "I2C0 error\r\n");
        return false;
    }

    return true;
}


bool HTS221_DataRateSet(HTS221_ODR_T odr)
{
    uint8_t reg;
    uint8_t data;
    fsp_err_t err;

    reg = HTS221_CTRL_REG1;
    err = I2C0_Read(reg, &data, 1, I2C_TRANSACTION_BUSY_DELAY);
    if (FSP_SUCCESS != err)
    {
        SEGGER_RTT_printf(0, "I2C0 error\r\n");
        return false;
    }
    data = (odr) | (data & 0xFC);
    err = I2C0_Write(reg, &data, 1, I2C_TRANSACTION_BUSY_DELAY);
    if (FSP_SUCCESS != err)
    {
        SEGGER_RTT_printf(0, "I2C0 error\r\n");
        return false;
    }

    return true;
}

bool HTS221_Init(HTS221_ODR_T odr)
{
    bool ret;

    R_IIC_MASTER_Open(&g_i2c_master0_ctrl, &g_i2c_master0_cfg);

    ret = HTS221_PowerOnSet(HTS221_NOT_SET);
    if(!ret) return ret;
    ret = HTS221_BDataSet(HTS221_ON_SET);
    if(!ret) return ret;
    ret = HTS221_DataRateSet(odr);
    if(!ret) return ret;
    ret = HTS221_PowerOnSet(HTS221_ON_SET);
    if(!ret) return ret;

    return true;
}

bool HTS221_IsWork()
{
    uint8_t reg;
    uint8_t data;
    fsp_err_t err;

    reg = HTS221_WHO_AM_I;
    err = I2C0_Read(reg, &data, 1, I2C_TRANSACTION_BUSY_DELAY);
    if (FSP_SUCCESS != err)
    {
        SEGGER_RTT_printf(0, "I2C0 error\r\n");
        return false;
    }

    if(HTS221_ID != data)
    {
        SEGGER_RTT_printf(0, "HTS221 is not working\r\n");
        return false;
    }

    return true;
}

bool HTS221_GetHumidity(float* hum)
{
    fsp_err_t err;

    float x0,x1,y0,y1;

    uint8_t i8bit;
    int16_t i16bit;
    uint8_t coeff_p[2];
    int16_t coeff;

    err = I2C0_Read(HTS221_H0_T0_OUT_L, coeff_p, 2, I2C_TRANSACTION_BUSY_DELAY);
    if (FSP_SUCCESS != err)
    {
        SEGGER_RTT_printf(0, "I2C0 error\r\n");
        return false;
    }
    coeff = ((int16_t)coeff_p[1] << 8) + (int16_t)coeff_p[0];
    x0 = (float)coeff * 1.0f;

    err = I2C0_Read(HTS221_H0_RH_X2, &i8bit, 1, I2C_TRANSACTION_BUSY_DELAY);
    if (FSP_SUCCESS != err)
    {
        SEGGER_RTT_printf(0, "I2C0 error\r\n");
        return false;
    }
    y0 = (float)i8bit / 2.0f;

    err = I2C0_Read(HTS221_H1_T0_OUT_L, coeff_p, 2, I2C_TRANSACTION_BUSY_DELAY);
    if (FSP_SUCCESS != err)
    {
        SEGGER_RTT_printf(0, "I2C0 error\r\n");
        return false;
    }
    coeff = ((int16_t)coeff_p[1] << 8) + (int16_t)coeff_p[0];
    x1 = (float)coeff * 1.0f;

    err = I2C0_Read(HTS221_H1_RH_X2, &i8bit, 1, I2C_TRANSACTION_BUSY_DELAY);
    if (FSP_SUCCESS != err)
    {
        SEGGER_RTT_printf(0, "I2C0 error\r\n");
        return false;
    }
    y1 = (float)i8bit / 2.0f;

    err = I2C0_Read(HTS221_HUMIDITY_OUT_L, coeff_p, 2, I2C_TRANSACTION_BUSY_DELAY);
    if (FSP_SUCCESS != err)
    {
        SEGGER_RTT_printf(0, "I2C0 error\r\n");
        return false;
    }
    i16bit = ((int16_t)coeff_p[1] << 8) +  (int16_t)coeff_p[0];

    *hum = (((y1 - y0) * (float)i16bit) + ((x1 * y0) - (x0 * y1))) / (x1 - x0);
    if (*hum < 0.0f)
    {
      *hum = 0.0f;
    }
    if (*hum > 100.0f)
    {
      *hum = 100.0f;
    }

    return true;
}

bool HTS221_GetTemperature(float*tmp)
{
    fsp_err_t err;

    float x0,x1,y0,y1;
    uint8_t t0_msb, t1_msb;
    uint8_t i8bit;
    int16_t i16bit;
    uint8_t coeff_p[2];
    int16_t coeff;

    err = I2C0_Read(HTS221_T1_T0_MSB, &i8bit, 1, I2C_TRANSACTION_BUSY_DELAY);
    if (FSP_SUCCESS != err)
    {
        SEGGER_RTT_printf(0, "I2C0 error\r\n");
        return false;
    }
    t0_msb = i8bit & 0x03;
    t1_msb = (i8bit>>2) & 0x03;

    err = I2C0_Read(HTS221_T0_OUT_L, coeff_p, 2, I2C_TRANSACTION_BUSY_DELAY);
    if (FSP_SUCCESS != err)
    {
        SEGGER_RTT_printf(0, "I2C0 error\r\n");
        return false;
    }
    coeff = ((int16_t)coeff_p[1]<<8) + (int16_t)coeff_p[0];
    x0 = (float)coeff * 1.0f;

    err = I2C0_Read(HTS221_T0_DEGC_X8, &i8bit, 1, I2C_TRANSACTION_BUSY_DELAY);
    if (FSP_SUCCESS != err)
    {
        SEGGER_RTT_printf(0, "I2C0 error\r\n");
        return false;
    }
    y0 = (float)((t0_msb<<8) + i8bit) /8.0f;       // t0

    err = I2C0_Read(HTS221_T1_OUT_L, coeff_p, 2, I2C_TRANSACTION_BUSY_DELAY);
    if (FSP_SUCCESS != err)
    {
        SEGGER_RTT_printf(0, "I2C0 error\r\n");
        return false;
    }
    coeff = ((int16_t)coeff_p[1]<<8) + (int16_t)coeff_p[0];
    x1 = (float)coeff * 1.0f;

    err = I2C0_Read(HTS221_T1_DEGC_X8, &i8bit, 1, I2C_TRANSACTION_BUSY_DELAY);
    if (FSP_SUCCESS != err)
    {
        SEGGER_RTT_printf(0, "I2C0 error\r\n");
        return false;
    }
    y1 = (float)((t1_msb<<8) + i8bit) /8.0f;          //  t1

    err = I2C0_Read(HTS221_TEMP_OUT_L, coeff_p, 2, I2C_TRANSACTION_BUSY_DELAY);
    if (FSP_SUCCESS != err)
    {
        SEGGER_RTT_printf(0, "I2C0 error\r\n");
        return false;
    }
    i16bit = ((int16_t)coeff_p[1]<<8) +  (int16_t)coeff_p[0];

    *tmp = (((y1 - y0) * (float)i16bit) + ((x1 * y0) - (x0 * y1))) / (x1 - x0);

    return true;
}
