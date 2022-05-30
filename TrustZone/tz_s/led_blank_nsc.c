#include "led_blank_nsc.h"

int level = 1;

void AGT0_Callback(timer_callback_args_t *p_args)
{
    if(p_args->event == TIMER_EVENT_CYCLE_END)
    {
        R_IOPORT_PinWrite(&g_ioport_ctrl, LED2, level);
        level = (level+1)%2;
    }
}

BSP_CMSE_NONSECURE_ENTRY void led_blank_start_guard(void)
{
    R_AGT_Open(&g_timer0_ctrl, &g_timer0_cfg);
    R_AGT_Start(&g_timer0_ctrl);
}

BSP_CMSE_NONSECURE_ENTRY void led_blank_stop_guard(void)
{
    R_AGT_Stop(&g_timer0_ctrl);
    R_AGT_Close(&g_timer0_ctrl);
}


