#include "hal_data.h"
#include "led_blank_nsc.h"
#include "RTT/SEGGER_RTT.h"

#define SEGGER_INDEX            (0)
#define APP_PRINT(fn_, ...)     SEGGER_RTT_printf (SEGGER_INDEX,(fn_), ##__VA_ARGS__)
#define APP_CHECK_DATA          SEGGER_RTT_HasKey()
#define APP_READ(read_data)     SEGGER_RTT_Read (SEGGER_INDEX, read_data, sizeof(read_data))


FSP_CPP_HEADER
void R_BSP_WarmStart(bsp_warm_start_event_t event);
FSP_CPP_FOOTER

void print_main_menu(void);

void print_main_menu(void)
{
    APP_PRINT("MENU to Select \r\n");

    /* Menu for User Selection */
    APP_PRINT("Press 1 to Start blank LED\r\n");
    APP_PRINT("Press 2 to Stop blank LED\r\n");
}


/*******************************************************************************************************************//**
 * main() is generated by the RA Configuration editor and is used to generate threads if an RTOS is used.  This function
 * is called by main() when no RTOS is used.
 **********************************************************************************************************************/
void hal_entry(void)
{
    /* TODO: add your own code here */
    uint8_t readBuff[16] =  { 0 };
    uint32_t rByte;
    int32_t inputRead;

    APP_PRINT("System Reset, enter non-secure region\r\n");
    print_main_menu();

    /* TODO: add your own code here */
    while (1)
    {
        if (APP_CHECK_DATA)
        {
            rByte = APP_READ(readBuff);
            if(rByte > 0)
            {
                inputRead = atoi((char *)readBuff);

                switch(inputRead)
                {
                    case 1:
                        APP_PRINT("1 to Start blank LED\r\n");
                        led_blank_start_guard();
                        break;
                    case 2:
                        APP_PRINT("2 to Stop blank LED\r\n");
                        led_blank_stop_guard();
                        break;
                    default:
                        print_main_menu();
                        break;
                }
            }
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
