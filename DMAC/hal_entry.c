#include "hal_data.h"
#include <string.h>
#include "RTT/SEGGER_RTT.h"

FSP_CPP_HEADER
void R_BSP_WarmStart(bsp_warm_start_event_t event);
FSP_CPP_FOOTER

#define BUFFER_SIZE 18

static volatile bool is_agt0_event = false;
uint8_t sci_dmac_data[BUFFER_SIZE] = {"DMAC Data String\r\n"};
uint8_t dmac_block_data[BUFFER_SIZE+1];

void sys_init(void);
fsp_err_t user_led_toggle(void);

void user_uart_callback(uart_callback_args_t *p_args)
{
    (void)p_args;
}

void AGT0_Callback(timer_callback_args_t *p_args)
{
    if(TIMER_EVENT_CYCLE_END == p_args->event)
    {
        is_agt0_event = true;
    }
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

void sys_init(void)
{
    R_SCI_UART_Open(&g_uart9_ctrl, &g_uart9_cfg);

    g_transfer0_cfg.p_info->p_src = &sci_dmac_data[0];
    g_transfer0_cfg.p_info->p_dest = (void*)&R_SCI9->TDR;

    g_transfer1_cfg.p_info->p_src = &sci_dmac_data[0];
    g_transfer1_cfg.p_info->p_dest = &dmac_block_data[0];
}

/*******************************************************************************************************************//**
 * main() is generated by the RA Configuration editor and is used to generate threads if an RTOS is used.  This function
 * is called by main() when no RTOS is used.
 **********************************************************************************************************************/
void hal_entry(void)
{
    uint8_t buff[] = {"Start Application\n\r"};
    /* TODO: add your own code here */
    sys_init();

    SEGGER_RTT_printf(0,"Start Application\r\n");
    R_SCI_UART_Write(&g_uart9_ctrl, buff, strlen(buff));

    R_DMAC_Open(&g_transfer0_ctrl, &g_transfer0_cfg);
    R_AGT_Open(&g_timer0_ctrl, &g_timer0_cfg);

    R_DMAC_Enable(&g_transfer0_ctrl);
    R_AGT_Start(&g_timer0_ctrl);

    R_BSP_SoftwareDelay(2000, BSP_DELAY_UNITS_MILLISECONDS);
    R_DMAC_Close(&g_transfer0_ctrl);

    while(1)
    {
        if(is_agt0_event)
        {
            R_DMAC_Open(&g_transfer1_ctrl, &g_transfer1_cfg);
            R_DMAC_Enable(&g_transfer1_ctrl);
            R_DMAC_SoftwareStart(&g_transfer1_ctrl, TRANSFER_START_MODE_SINGLE);
            SEGGER_RTT_printf(0,"%s", dmac_block_data);
            R_DMAC_Close(&g_transfer1_ctrl);

            user_led_toggle();
            is_agt0_event = false;
            memset(dmac_block_data, 0, sizeof(dmac_block_data));
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
