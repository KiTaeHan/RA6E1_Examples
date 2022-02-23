#include "hal_data.h"

/* TODO: Enable if you want to open ZMOD4XXX */
#define G_ZMOD4XXX_SENSOR0_IRQ_ENABLE   (1)

typedef enum e_demo_sequence
{
    DEMO_SEQUENCE_1 = (1),
    DEMO_SEQUENCE_2,
    DEMO_SEQUENCE_3,
    DEMO_SEQUENCE_4,
    DEMO_SEQUENCE_5,
    DEMO_SEQUENCE_6,
    DEMO_SEQUENCE_7,
    DEMO_SEQUENCE_8,
    DEMO_SEQUENCE_9,
} demo_sequence_t;

typedef enum e_demo_callback_status
{
    DEMO_CALLBACK_STATUS_WAIT = (0),
    DEMO_CALLBACK_STATUS_SUCCESS,
    DEMO_CALLBACK_STATUS_REPEAT,
} demo_callback_status_t;

void g_comms_i2c_bus0_quick_setup(void);
void g_zmod4xxx_sensor0_quick_setup(void);
void start_demo(void);
void demo_err(void);

static volatile demo_callback_status_t  gs_i2c_callback_status = DEMO_CALLBACK_STATUS_WAIT;
#if G_ZMOD4XXX_SENSOR0_IRQ_ENABLE
static volatile demo_callback_status_t  gs_irq_callback_status = DEMO_CALLBACK_STATUS_WAIT;
#endif

static volatile rm_zmod4xxx_oaq_1st_data_t      gs_oaq_1st_gen_data;
static volatile rm_zmod4xxx_oaq_2nd_data_t      gs_oaq_2nd_gen_data;

void zmod4xxx_comms_i2c_callback(rm_zmod4xxx_callback_args_t * p_args)
{
    if (RM_ZMOD4XXX_EVENT_ERROR != p_args->event)
    {
        gs_i2c_callback_status = DEMO_CALLBACK_STATUS_SUCCESS;
    }
    else
    {
        gs_i2c_callback_status = DEMO_CALLBACK_STATUS_REPEAT;
    }
}

/* TODO: Enable if you want to use a IRQ callback */
void zmod4xxx_irq_callback(rm_zmod4xxx_callback_args_t * p_args)
{
#if G_ZMOD4XXX_SENSOR0_IRQ_ENABLE
    FSP_PARAMETER_NOT_USED(p_args);

    gs_irq_callback_status = DEMO_CALLBACK_STATUS_SUCCESS;
#else
    FSP_PARAMETER_NOT_USED(p_args);
#endif
}

/* Quick setup for g_zmod4xxx_sensor0. */
void g_zmod4xxx_sensor0_quick_setup(void)
{
    fsp_err_t err;

    /* Open ZMOD4XXX sensor instance, this must be done before calling any ZMOD4XXX API */
    err = g_zmod4xxx_sensor0.p_api->open(g_zmod4xxx_sensor0.p_ctrl, g_zmod4xxx_sensor0.p_cfg);
    if (FSP_SUCCESS != err)
    {
        demo_err();
    }
}

/* Quick setup for g_comms_i2c_bus0. */
void g_comms_i2c_bus0_quick_setup(void)
{
    fsp_err_t err;
    i2c_master_instance_t * p_driver_instance = (i2c_master_instance_t *) g_comms_i2c_bus0_extended_cfg.p_driver_instance;

    /* Open I2C driver, this must be done before calling any COMMS API */
    err = p_driver_instance->p_api->open(p_driver_instance->p_ctrl, p_driver_instance->p_cfg);
    if (FSP_SUCCESS != err)
    {
        demo_err();
    }
}

void start_demo(void)
{
    fsp_err_t               err;
    rm_zmod4xxx_raw_data_t  raw_data;
    demo_sequence_t         sequence = DEMO_SEQUENCE_1;
    rm_zmod4xxx_lib_type_t  lib_type = g_zmod4xxx_sensor0_extended_cfg.lib_type;
    float					temperature = 20.0F;
    float					humidity	= 50.0F;

    /* Clear status */
    gs_i2c_callback_status = DEMO_CALLBACK_STATUS_WAIT;
#if G_ZMOD4XXX_SENSOR0_IRQ_ENABLE
    gs_irq_callback_status = DEMO_CALLBACK_STATUS_WAIT;
#endif

    /* Open the Bus */
    g_comms_i2c_bus0_quick_setup();

    /* Open ZMOD4XXX */
    g_zmod4xxx_sensor0_quick_setup();

    while(1)
    {
        switch(sequence)
        {
            case DEMO_SEQUENCE_1 :
            {
                /* Clear status */
                gs_i2c_callback_status = DEMO_CALLBACK_STATUS_WAIT;

                /* Start measurement */
                err = g_zmod4xxx_sensor0.p_api->measurementStart(g_zmod4xxx_sensor0.p_ctrl);
                if (FSP_SUCCESS == err)
                {
                    sequence = DEMO_SEQUENCE_2;
                }
                else
                {
                    demo_err();
                }
            }
            break;

            case DEMO_SEQUENCE_2 :
            {
                /* Check I2C callback status */
                switch (gs_i2c_callback_status)
                {
                    case DEMO_CALLBACK_STATUS_WAIT :
                        break;
                    case DEMO_CALLBACK_STATUS_SUCCESS :
                        sequence = DEMO_SEQUENCE_3;
                        break;
                    case DEMO_CALLBACK_STATUS_REPEAT :
                        sequence = DEMO_SEQUENCE_1;
                        break;
                    default :
                        demo_err();
                        break;
                }
            }
            break;

#if G_ZMOD4XXX_SENSOR0_IRQ_ENABLE
            case DEMO_SEQUENCE_3 :
            {
                /* Check IRQ callback status */
                switch (gs_irq_callback_status)
                {
                    case DEMO_CALLBACK_STATUS_WAIT :
                        break;
                    case DEMO_CALLBACK_STATUS_SUCCESS :
                        gs_irq_callback_status = DEMO_CALLBACK_STATUS_WAIT;
                        sequence = DEMO_SEQUENCE_5;
                        break;
                    default :
                        demo_err();
                        break;
                }
            }
            break;
#else
            case DEMO_SEQUENCE_3 :
            {
                /* Clear status */
                gs_i2c_callback_status = DEMO_CALLBACK_STATUS_WAIT;

                /* Get status */
                err = g_zmod4xxx_sensor0.p_api->statusCheck(g_zmod4xxx_sensor0.p_ctrl);
                if (FSP_SUCCESS == err)
                {
                    sequence = DEMO_SEQUENCE_4;
                }
                else
                {
                    demo_err();
                }
            }
            break;

            case DEMO_SEQUENCE_4 :
            {
                /* Check I2C callback status */
                switch (gs_i2c_callback_status)
                {
                    case DEMO_CALLBACK_STATUS_WAIT :
                        break;
                    case DEMO_CALLBACK_STATUS_SUCCESS :
                        sequence = DEMO_SEQUENCE_5;
                        break;
                    case DEMO_CALLBACK_STATUS_REPEAT :
                        sequence = DEMO_SEQUENCE_3;
                        break;
                    default :
                        demo_err();
                        break;
                }
            }
            break;
#endif
            case DEMO_SEQUENCE_5 :
            {
                /* Clear status */
                gs_i2c_callback_status = DEMO_CALLBACK_STATUS_WAIT;

                /* Read data */
                err = g_zmod4xxx_sensor0.p_api->read(g_zmod4xxx_sensor0.p_ctrl, &raw_data);
                if (FSP_SUCCESS == err)
                {
                    sequence = DEMO_SEQUENCE_6;
                }
                else if (FSP_ERR_SENSOR_MEASUREMENT_NOT_FINISHED == err)
                {
                    sequence = DEMO_SEQUENCE_3;

                    /* Delay 50ms */
                    R_BSP_SoftwareDelay(50, BSP_DELAY_UNITS_MILLISECONDS);
                }
                else
                {
                    demo_err();
                }
            }
            break;

            case DEMO_SEQUENCE_6 :
            {
                /* Check I2C callback status */
                switch (gs_i2c_callback_status)
                {
                    case DEMO_CALLBACK_STATUS_WAIT :
                        break;
                    case DEMO_CALLBACK_STATUS_SUCCESS :
                        sequence = DEMO_SEQUENCE_7;
                        break;
                    case DEMO_CALLBACK_STATUS_REPEAT :
                        sequence = DEMO_SEQUENCE_5;
                        break;
                    default :
                        demo_err();
                        break;
                }
            }
            break;

            case DEMO_SEQUENCE_7 :
            {
                /* Calculate data */
                switch (lib_type)
                {
                    case RM_ZMOD4510_LIB_TYPE_OAQ_1ST_GEN :
                        err = g_zmod4xxx_sensor0.p_api->oaq1stGenDataCalculate(g_zmod4xxx_sensor0.p_ctrl,
                                                                               &raw_data,
                                                                               (rm_zmod4xxx_oaq_1st_data_t*)&gs_oaq_1st_gen_data);
                        break;
                    case RM_ZMOD4510_LIB_TYPE_OAQ_2ND_GEN :
                    	err = g_zmod4xxx_sensor0.p_api->temperatureAndHumiditySet(g_zmod4xxx_sensor0.p_ctrl,
																				  temperature,
																				  humidity);
                    	if (err != FSP_SUCCESS)
                    	{
                    		demo_err();
                    	}
                        err = g_zmod4xxx_sensor0.p_api->oaq2ndGenDataCalculate(g_zmod4xxx_sensor0.p_ctrl,
                                                                               &raw_data,
                                                                               (rm_zmod4xxx_oaq_2nd_data_t*)&gs_oaq_2nd_gen_data);
                        break;
                    default :
                        demo_err();
                        break;
                }

                if (FSP_SUCCESS == err)
                {
                    /* Gas data is valid. Describe the process by referring to each calculated gas data. */
                }
                else if (FSP_ERR_SENSOR_IN_STABILIZATION == err)
                {
                    /* Gas data is invalid. Sensor is in stabilization. */
                }
                else
                {
                    demo_err();
                }

                sequence = DEMO_SEQUENCE_8;
            }
            break;

            case DEMO_SEQUENCE_8 :
            {
                switch (lib_type)
                {
                    case RM_ZMOD4510_LIB_TYPE_OAQ_1ST_GEN :
                        break;
                    case RM_ZMOD4510_LIB_TYPE_OAQ_2ND_GEN :
                        /* See Table 4 in the ZMOD4510 Programming Manual. */
                        R_BSP_SoftwareDelay(1990, BSP_DELAY_UNITS_MILLISECONDS);
                        break;
                    default :
                        demo_err();
                        break;
                }

                sequence = DEMO_SEQUENCE_1;
            }
            break;

            default :
            {
                demo_err();
            }
            break;
        }
    }
}

void demo_err(void)
{
    while(1)
    {
        // nothing
    }
}
