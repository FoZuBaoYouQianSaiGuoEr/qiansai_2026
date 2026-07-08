#include "button.h"
#include <stdio.h>

void button_init(void)
{
    BTN_CLK_ENABLE_B;
    BTN_CLK_ENABLE_D;

    GPIO_InitTypeDef g = {0};
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_LOW;

    g.Pin = BTN_TOMATO_PIN;
    HAL_GPIO_Init(BTN_TOMATO_PORT, &g);

    g.Pin = BTN_HAND_PIN;
    HAL_GPIO_Init(BTN_HAND_PORT, &g);

    g.Pin = BTN_SENTRY_PIN;
    HAL_GPIO_Init(BTN_SENTRY_PORT, &g);

    g.Pin = BTN_BRUP_PIN;
    HAL_GPIO_Init(BTN_BRUP_PORT, &g);

    g.Pin = BTN_BRDOWN_PIN;
    HAL_GPIO_Init(BTN_BRDOWN_PORT, &g);

    printf("[BTN] PB12=TOMATO PB13=HAND PB14=SENTRY PD11=BR+ PD12=BR-\r\n");
}

static uint8_t btn_read(GPIO_TypeDef *port, uint16_t pin)
{
    return (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_RESET);
}

ButtonEvent button_scan(void)
{
    static uint32_t last_scan;
    uint32_t now = HAL_GetTick();
    if (now - last_scan < 30) return BTN_NONE;
    last_scan = now;

    if (btn_read(BTN_TOMATO_PORT, BTN_TOMATO_PIN))  return BTN_TOMATO;
    if (btn_read(BTN_HAND_PORT,   BTN_HAND_PIN))    return BTN_HAND;
    if (btn_read(BTN_SENTRY_PORT, BTN_SENTRY_PIN))  return BTN_SENTRY;
    if (btn_read(BTN_BRUP_PORT,   BTN_BRUP_PIN))    return BTN_BRUP;
    if (btn_read(BTN_BRDOWN_PORT, BTN_BRDOWN_PIN))  return BTN_BRDOWN;

    return BTN_NONE;
}
