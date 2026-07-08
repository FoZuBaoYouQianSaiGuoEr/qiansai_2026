#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"
#include "led.h"
#include "usart.h"
#include "lcd_spi_154.h"
#include "dcmi_ov5640.h"
#include "network.h"
#include "network_data.h"

void Error_Handler(void);

/* AI activations 静态 buffer (不走堆避免碎片) */
#define AI_ACTIVATIONS_SIZE  96112
extern uint8_t ai_act_buf[AI_ACTIVATIONS_SIZE];

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
