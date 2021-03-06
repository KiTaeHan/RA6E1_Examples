#include "hal_data.h"

#include <stdio.h>
#include <stdlib.h>
#include "RTT/SEGGER_RTT.h"

#define GPT_MAX_PERCENT             (100U)          /* Max Duty Cycle percentage */
#define BUF_SIZE                    (16U)           /* Size of buffer for RTT input data */
#define INITIAL_VALUE               '\0'
#define TIMER_UNITS_MILLISECONDS    (1000U)        /* timer unit in millisecond */
#define CLOCK_TYPE_SPECIFIER        (1ULL)         /* type specifier */

FSP_CPP_HEADER
void R_BSP_WarmStart(bsp_warm_start_event_t event);
FSP_CPP_FOOTER

void Period_Init(void);
void change_Period(void);
fsp_err_t set_timer_duty_cycle(uint8_t duty_cycle_percent);
void change_Duty(void);

void AGT0_Callback(timer_callback_args_t *p_args)
{
    if(p_args->event == TIMER_EVENT_CYCLE_END)
    {
        SEGGER_RTT_WriteString(0, "AGT0\r\n");
    }
}

void GPT1_callback(timer_callback_args_t *p_args)
{

    if(p_args->event == TIMER_EVENT_CYCLE_END)
    {
        SEGGER_RTT_WriteString(0, "~~~GPT0\r\n");
    }
}

void Period_Init(void)
{
    R_GPT_Open(&gpt_timer1_periodic_ctrl, &gpt_timer1_periodic_cfg);
    R_GPT_Start(&gpt_timer1_periodic_ctrl);

    R_GPT_Open(&gpt_timer2_pwm_ctrl, &gpt_timer2_pwm_cfg);
    R_GPT_Start(&gpt_timer2_pwm_ctrl);
}

void change_Period(void)
{
    uint64_t period_counts;
	uint32_t pclkd_freq_hz;
    uint32_t value;
    unsigned char read_data[BUF_SIZE] = {INITIAL_VALUE};
    int r = 0;

    SEGGER_RTT_WriteString(0, "\r\n Enter the desired period in millisecond\r\n");
    do
    {
        read_data[r] = (unsigned char)SEGGER_RTT_WaitKey();
        if((read_data[r] < 0x30) || (read_data[r] > 0x39))   // ASCII 0 ~ 9
        {
            read_data[r] = INITIAL_VALUE;
            break;
        }
        r++;
    } while(r<BUF_SIZE);
    value =  (uint32_t) (atoi((char *)read_data));

    /* Get the source clock frequency (in Hz) */
    pclkd_freq_hz = R_FSP_SystemClockHzGet(FSP_PRIV_CLOCK_PCLKD);
    pclkd_freq_hz >>= (uint32_t)(gpt_timer1_periodic_cfg.source_div);

    /* Convert period to PCLK counts so it can be set in hardware. */
    period_counts = (uint64_t)((value * (pclkd_freq_hz * CLOCK_TYPE_SPECIFIER))  / TIMER_UNITS_MILLISECONDS);

    /* Period Set API set the desired period counts on the on-board LED */
    R_GPT_PeriodSet(&gpt_timer1_periodic_ctrl, (uint32_t)period_counts);
}

fsp_err_t set_timer_duty_cycle(uint8_t duty_cycle_percent)
{
    fsp_err_t err                           = FSP_SUCCESS;
    uint32_t duty_cycle_counts              = 0;
    uint32_t current_period_counts          = 0;
    timer_info_t info                       = {(timer_direction_t)0, 0, 0};

    /* Get the current period setting. */
    err = R_GPT_InfoGet(&gpt_timer2_pwm_ctrl, &info);
    if (FSP_SUCCESS != err)
    {
        /* GPT Timer InfoGet Failure message */
        SEGGER_RTT_WriteString(0,"\r\n ** R_GPT_InfoGet API failed ** \r\n");
    }
    else
    {
        /* update period counts locally. */
        current_period_counts = info.period_counts;
        duty_cycle_counts =(uint32_t) ((uint64_t) (current_period_counts * duty_cycle_percent) / GPT_MAX_PERCENT);

        /* Duty Cycle Set API set the desired intensity on the on-board LED */
        err = R_GPT_DutyCycleSet(&gpt_timer2_pwm_ctrl, duty_cycle_counts, GPT_IO_PIN_GTIOCA);
        if(FSP_SUCCESS != err)
        {
            /* GPT Timer DutyCycleSet Failure message */
            SEGGER_RTT_WriteString(0,"\r\n ** R_GPT_DutyCycleSet API failed ** \r\n");
        }
    }
    return err;
}

void change_Duty(void)
{
    fsp_err_t err = FSP_SUCCESS;
    uint32_t gpt_desired_duty_cycle_percent;
    unsigned char read_data[BUF_SIZE] = {INITIAL_VALUE};
    int r = 0;

    SEGGER_RTT_WriteString(0, "\r\n Enter The Desired Duty Cycle in Percentage of Range 0 - 100:\r\n");
    do
    {
        read_data[r] = (unsigned char)SEGGER_RTT_WaitKey();
        if((read_data[r] < 0x30) || (read_data[r] > 0x39))   // ASCII 0 ~ 9
        {
            read_data[r] = INITIAL_VALUE;
            break;
        }
        r++;
    } while(r<BUF_SIZE);
    gpt_desired_duty_cycle_percent =  (uint32_t) (atoi((char *)read_data));

    /* Validate Duty cycle percentage */
    if (GPT_MAX_PERCENT < gpt_desired_duty_cycle_percent)
    {
        SEGGER_RTT_WriteString(0, " INVALID INPUT, DESIRED DUTY CYCLE IS 0 ~ 100\r\n");
    }
    else
    {
        SEGGER_RTT_printf(0, "Duty cycle is %d\r\n", gpt_desired_duty_cycle_percent);
        err = set_timer_duty_cycle((uint8_t)gpt_desired_duty_cycle_percent);
        if(FSP_SUCCESS != err)
        {
            SEGGER_RTT_WriteString(0, "\r\n ** GPT TIMER DUTYCYCLE SET FAILED ** \r\n");
            /*Close PWM Timer instance */
            R_GPT_Close(&gpt_timer2_pwm_ctrl);
        }
    }
}


/*******************************************************************************************************************//**
 * main() is generated by the RA Configuration editor and is used to generate threads if an RTOS is used.  This function
 * is called by main() when no RTOS is used.
 **********************************************************************************************************************/
void hal_entry(void)
{
    /* TODO: add your own code here */
    SEGGER_RTT_WriteString(0, "SEGGER RTT Terminal\r\n");
    SEGGER_RTT_ConfigUpBuffer(0, NULL, NULL, 0, SEGGER_RTT_MODE_NO_BLOCK_SKIP);

    Period_Init();
    R_AGT_Open(&agt_timer0_periodic_ctrl, &agt_timer0_periodic_cfg);
    R_AGT_Start(&agt_timer0_periodic_ctrl);

    while(1)
    {

        change_Period();
        change_Duty();
        R_BSP_SoftwareDelay(1000, BSP_DELAY_UNITS_MILLISECONDS);
    }

#if BSP_TZ_SECURE_BUILD
    /* Enter non-secure code */
    R_BSP_NonSecureEnter();
#endif
}

/*******************************************************************************************************************//**
 * This function is called at various points during the startup process.  This implementation uses the event that is
 * called right before main() to set up the pins.
 *
 * @param[in]  event    Where at in the start up process the code is currently at
 **********************************************************************************************************************/
void R_BSP_WarmStart(bsp_warm_start_event_t event)
{
    if (BSP_WARM_START_RESET == event)
    {
#if BSP_FEATURE_FLASH_LP_VERSION != 0

        /* Enable reading from data flash. */
        R_FACI_LP->DFLCTL = 1U;

        /* Would normally have to wait tDSTOP(6us) for data flash recovery. Placing the enable here, before clock and
         * C runtime initialization, should negate the need for a delay since the initialization will typically take more than 6us. */
#endif
    }

    if (BSP_WARM_START_POST_C == event)
    {
        /* C runtime environment and system clocks are setup. */

        /* Configure pins. */
        R_IOPORT_Open (&g_ioport_ctrl, g_ioport.p_cfg);
    }
}

#if BSP_TZ_SECURE_BUILD

BSP_CMSE_NONSECURE_ENTRY void template_nonsecure_callable ();

/* Trustzone Secure Projects require at least one nonsecure callable function in order to build (Remove this if it is not required to build). */
BSP_CMSE_NONSECURE_ENTRY void template_nonsecure_callable ()
{

}
#endif
