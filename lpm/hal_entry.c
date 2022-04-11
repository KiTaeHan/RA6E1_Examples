#include "hal_data.h"
#include "RTT/SEGGER_RTT.h"

#define LONG_KEY_DELAY  10      // 10 * 100ms = 1s

typedef enum e_u_event
{
    EV_PB_SHORT_PR,
    EV_PB_LONG_PR,
    EV_RTC_ALARM,
    EV_POWER_ON_RESET,
    EV_POWER_ON_RESET_DSSBY,
    EV_NONE
}u_event_t;

typedef enum e_app_lpm_state
{
    APP_LPM_SW_STANDBY_STATE = 0,        ///< SW Standby mode
    APP_LPM_DEEP_SW_STANDBY_STATE,       ///< Deep SW Standby mode
    APP_LPM_NORMAL_STATE                 ///< Normal mode
} app_lpm_states_t;

static bool is_reset_from_deep_standby = false;
static volatile bool user_button_down  = false;
static volatile uint8_t user_gpt1_timer_counter  = 0;

FSP_CPP_HEADER
void R_BSP_WarmStart(bsp_warm_start_event_t event);
FSP_CPP_FOOTER

fsp_err_t rtc_init(void);
fsp_err_t rtc_set_time(void);
fsp_err_t rtc_set_alarm(void);
fsp_err_t rtc_reset_alarm(void);
fsp_err_t user_led_toggle(void);

void system_init(void);

//volatile  int switch_state = 0;
u_event_t event_type =  EV_NONE;
app_lpm_states_t lpm_state = APP_LPM_NORMAL_STATE;

// RTC ISR callback
void rtc_callback(rtc_callback_args_t *p_args)
{
    if(RTC_EVENT_ALARM_IRQ == p_args->event)
    {
        event_type = EV_RTC_ALARM;
    }
}

void external_sw1_callback(external_irq_callback_args_t *p_args)
{
    if(p_args->channel == 1)
    {
        if(false == user_button_down)
        {
            user_button_down = true;
            user_gpt1_timer_counter = 0;
        }
        else
        {
            if(user_gpt1_timer_counter > LONG_KEY_DELAY)
                event_type = EV_PB_LONG_PR;
            else
                event_type = EV_PB_SHORT_PR;

            user_button_down = false;
            user_gpt1_timer_counter = 0;
        }
    }
}

void gpt1_timer_callback(timer_callback_args_t *p_args)
{
    if(TIMER_EVENT_CYCLE_END == p_args->event)
    {
        if(user_button_down)
            user_gpt1_timer_counter++;
    }
}

fsp_err_t rtc_init(void)
{
    fsp_err_t err = FSP_SUCCESS;
    err = R_RTC_Open(&g_rtc0_ctrl, &g_rtc0_cfg);
    return err;
}

fsp_err_t rtc_set_time(void)
{
    fsp_err_t err = FSP_SUCCESS;
    rtc_time_t config_time ={0};

    /* RTC Calendar Time is set to start from 01/01/2020 00:00:00 */
    config_time.tm_sec  = 0;
    config_time.tm_min  = 0;
    config_time.tm_hour = 0;
    config_time.tm_mday = 5;
    config_time.tm_mon  = 3;
    config_time.tm_year = 122;  // Add the User Year to the 1900. For 2020 = 1900 + 120

    /* Set the Calendar time  */
    err = R_RTC_CalendarTimeSet(&g_rtc0_ctrl, &config_time);
    /* Handle error */
    if (FSP_SUCCESS != err)
    {
        SEGGER_RTT_printf(0,"\r\nCalendarTime Set failed.\r\n");
        return err;
    }

    /* Get the current Calendar time */
    err = R_RTC_CalendarTimeGet(&g_rtc0_ctrl, &config_time);
    /* Handle error */
    if (FSP_SUCCESS != err)
    {
        SEGGER_RTT_printf(0,"\r\nCalendarTime Get failed.\r\n");
        return err;
    }

    /* Add the adjustment factor for Month and Year for User Readability*/
    config_time.tm_mon  +=  1;
    config_time.tm_year +=  1900;

    SEGGER_RTT_printf(0,"\r\n RTC calendar Time set to  Date : %d/%d/%d \n Time  : %d : %d : %d \r\n\n",
                   config_time.tm_year, config_time.tm_mon, config_time.tm_mday,
                   config_time.tm_hour,config_time.tm_min, config_time.tm_sec);
    return err;
}

fsp_err_t rtc_set_alarm(void)
{
    fsp_err_t err = FSP_SUCCESS;
    rtc_alarm_time_t alarm_time_set = {0};
    rtc_time_t config_time = {0};

    /* Set the flags for which the alarm has to be generated Setting it for seconds match*/
    alarm_time_set.hour_match  = false;
    alarm_time_set.min_match   = false;
    alarm_time_set.sec_match   = true;
    alarm_time_set.mday_match  = false;
    alarm_time_set.mon_match   = false;
    alarm_time_set.year_match  = false;

    /* Get the current Calendar time */
    err = R_RTC_CalendarTimeGet(&g_rtc0_ctrl, &config_time);
    /* Handle error */
    if (FSP_SUCCESS != err)
    {
        SEGGER_RTT_printf(0,"\r\nCalendarTime Get failed.\r\n");
        return err;
    }
    /* Adjust the desired Alarm time in seconds, so that seconds count rolls over from 60  */
    if((config_time.tm_sec + 10) / 60)
    {
        alarm_time_set.time.tm_sec = ((config_time.tm_sec + 10) % 60);
    }
    else
    {
        alarm_time_set.time.tm_sec = config_time.tm_sec + 10;
    }
    /* Set the new alarm time calculated, to a specific second for the trigger to happen */
    err = R_RTC_CalendarAlarmSet(&g_rtc0_ctrl, &alarm_time_set);
    /* Handle error */
    if (FSP_SUCCESS != err)
    {
        SEGGER_RTT_printf(0,"\r\nCalendar alarm Set failed.\r\n");
        return err;
    }
    return err;
}

fsp_err_t rtc_reset_alarm(void)
{
    fsp_err_t err = FSP_SUCCESS;     // Error status
    rtc_alarm_time_t alarm_time_set = {0};

	/* Set the flags to false for which the alarm has to be reset */
	alarm_time_set.hour_match = false;
	alarm_time_set.min_match  = false;
	alarm_time_set.sec_match  = false;
	alarm_time_set.mday_match = false;
	alarm_time_set.mon_match  = false;
	alarm_time_set.year_match = false;

	/* Reset  the alarm Flag, so that no alarm interrupt triggers from here onwards
	 * When needed, FSM Action Functions will be setting it back.
	 */
	err = R_RTC_CalendarAlarmSet(&g_rtc0_ctrl, &alarm_time_set);
	/* Handle error */
	if (FSP_SUCCESS != err)
	{
		SEGGER_RTT_printf(0,"\r\nCalendar alarm Set failed.\r\n");
	}
    return err;
}

fsp_err_t user_led_toggle(void)
{
    fsp_err_t err = FSP_SUCCESS;
    static bsp_io_level_t level = BSP_IO_LEVEL_HIGH;

    err = R_IOPORT_PinWrite(&g_ioport_ctrl, LED1, level);
    /* Handle error */
    if (FSP_SUCCESS == err)
    {
        /* Delay 500 milliseconds */
        R_BSP_SoftwareDelay(300, BSP_DELAY_UNITS_MILLISECONDS);
    }

    if(BSP_IO_LEVEL_HIGH == level)
    {
        level = BSP_IO_LEVEL_LOW;
    }
    else
    {
        level = BSP_IO_LEVEL_HIGH;
    }
    return err;
}

void system_init(void)
{
    user_button_down  = false;
    user_gpt1_timer_counter  = 0;

    rtc_init();
    rtc_set_time();

    R_ICU_ExternalIrqOpen(&g_external_irq1_ctrl, &g_external_irq1_cfg);
    R_ICU_ExternalIrqEnable(&g_external_irq1_ctrl);

    R_GPT_Open(&g_timer1_ctrl, &g_timer1_cfg);
    R_GPT_Start(&g_timer1_ctrl);
}

/*******************************************************************************************************************//**
 * main() is generated by the RA Configuration editor and is used to generate threads if an RTOS is used.  This function
 * is called by main() when no RTOS is used.
 **********************************************************************************************************************/
void hal_entry(void)
{
    /* TODO: add your own code here */
    SEGGER_RTT_printf(0,"Start Application\r\n");
    lpm_instance_ctrl_t lpm_ctrl_instance_ctrls[] = {g_lpm_standby_ctrl, g_lpm_deep_standby_ctrl};
    lpm_cfg_t           lpm_ctrl_instance_cfgs[]  = {g_lpm_standby_cfg, g_lpm_deep_standby_cfg};

    system_init();

    if(true == is_reset_from_deep_standby)
    {
        SEGGER_RTT_printf(0,"MCU Wake Up : from Deep SW standby Mode\r\n");
        event_type = EV_POWER_ON_RESET_DSSBY;
    }
    else
    {
        SEGGER_RTT_printf(0,"MCU Wake Up from Reset : Power on Reset\r\n");
        event_type = EV_POWER_ON_RESET;
    }

    while(1)
    {
        switch(event_type)
        {
            case EV_PB_SHORT_PR:
                SEGGER_RTT_printf(0,"Enter standby mode\r\n");
                R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);

                lpm_state = APP_LPM_SW_STANDBY_STATE;
                event_type = EV_NONE;
                rtc_set_alarm();

                R_LPM_Open(&lpm_ctrl_instance_ctrls[lpm_state], &lpm_ctrl_instance_cfgs[lpm_state]);
                R_LPM_LowPowerModeEnter(&lpm_ctrl_instance_ctrls[lpm_state]);
                R_LPM_Close(&lpm_ctrl_instance_ctrls[lpm_state]);

                lpm_state = APP_LPM_NORMAL_STATE;
                rtc_reset_alarm();

                SEGGER_RTT_printf(0,"Normal state\r\n");
                break;
            case EV_PB_LONG_PR:
                SEGGER_RTT_printf(0,"Enter deep standby mode\r\n");
                R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);

                lpm_state = APP_LPM_DEEP_SW_STANDBY_STATE;
                event_type = EV_NONE;
                rtc_set_alarm();

                R_LPM_Open(&lpm_ctrl_instance_ctrls[lpm_state], &lpm_ctrl_instance_cfgs[lpm_state]);
                R_LPM_LowPowerModeEnter(&lpm_ctrl_instance_ctrls[lpm_state]);
                R_LPM_Close(&lpm_ctrl_instance_ctrls[lpm_state]);

                lpm_state = APP_LPM_NORMAL_STATE;
                rtc_reset_alarm();

                SEGGER_RTT_printf(0,"Normal state\r\n");
                break;
            case EV_RTC_ALARM:
                SEGGER_RTT_printf(0,"RTC Alarm\r\n");
                event_type = EV_NONE;
                break;
            default:
                user_led_toggle();
                break;
        }
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

        /* Release IO from IOKEEP state if reset from Deep SW Standby mode */
        if(1 == R_SYSTEM->RSTSR0_b.DPSRSTF)
        {
            /* Input parameter is unused */
            R_LPM_IoKeepClear(NULL);

            /* Clear DPSRSTF flag */
            R_SYSTEM->RSTSR0_b.DPSRSTF = 0;
			is_reset_from_deep_standby = true;
        }

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
