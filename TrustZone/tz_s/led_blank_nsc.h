#ifndef LED_BLANK_NSC_H_
#define LED_BLANK_NSC_H_

#include "hal_data.h"

BSP_CMSE_NONSECURE_ENTRY void led_blank_start_guard(void);
BSP_CMSE_NONSECURE_ENTRY void led_blank_stop_guard(void);

#endif /* LED_BLANK_NSC_H_ */
