#ifndef __BUTTON_H
#define __BUTTON_H

#include "stm32h7xx_hal.h"

/* PB12: 番茄灯 */
#define BTN_TOMATO_PIN     GPIO_PIN_12
#define BTN_TOMATO_PORT    GPIOB

/* PB13: 光随手动 */
#define BTN_HAND_PIN       GPIO_PIN_13
#define BTN_HAND_PORT      GPIOB

/* PB14: 哨兵 */
#define BTN_SENTRY_PIN     GPIO_PIN_14
#define BTN_SENTRY_PORT    GPIOB

/* PD11: 亮度+ */
#define BTN_BRUP_PIN       GPIO_PIN_11
#define BTN_BRUP_PORT      GPIOD

/* PD12: 亮度- */
#define BTN_BRDOWN_PIN     GPIO_PIN_12
#define BTN_BRDOWN_PORT    GPIOD

#define BTN_CLK_ENABLE_B   __HAL_RCC_GPIOB_CLK_ENABLE()
#define BTN_CLK_ENABLE_D   __HAL_RCC_GPIOD_CLK_ENABLE()

typedef enum {
    BTN_NONE = 0,
    BTN_TOMATO,
    BTN_HAND,
    BTN_SENTRY,
    BTN_BRUP,
    BTN_BRDOWN,
} ButtonEvent;

void button_init(void);
ButtonEvent button_scan(void);

#endif
