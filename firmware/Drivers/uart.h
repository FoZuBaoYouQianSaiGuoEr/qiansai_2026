#ifndef __USART_H
#define __USART_H

#include "stm32h7xx_hal.h"
#include "stdio.h"

/* USART1: 调试日志 (PA9/PA10 → CH340 → PC) */
#define USART1_TX_PIN               GPIO_PIN_9
#define USART1_TX_PORT              GPIOA
#define USART1_RX_PIN               GPIO_PIN_10
#define USART1_RX_PORT              GPIOA
#define GPIO_USART1_CLK_ENABLE      __HAL_RCC_GPIOA_CLK_ENABLE()

/* USART2: 步进电机1 左右 (PD5=TX → 闭环驱动器) */
#define MOTOR1_TX_PIN               GPIO_PIN_5
#define MOTOR1_TX_PORT              GPIOD
#define MOTOR1_RX_PIN               GPIO_PIN_6
#define MOTOR1_RX_PORT              GPIOD
#define GPIO_MOTOR1_CLK_ENABLE      __HAL_RCC_GPIOD_CLK_ENABLE()

/* UART4: 步进电机2 上下 (PA0=TX → 闭环驱动器) */
#define MOTOR2_TX_PIN               GPIO_PIN_0
#define MOTOR2_TX_PORT              GPIOA
#define MOTOR2_RX_PIN               GPIO_PIN_1
#define MOTOR2_RX_PORT              GPIOA
#define GPIO_MOTOR2_CLK_ENABLE      __HAL_RCC_GPIOA_CLK_ENABLE()

/* USART3: K230视觉数据 (PB10/PB11) */
#define K230_TX_PIN                 GPIO_PIN_10
#define K230_TX_PORT                GPIOB
#define K230_RX_PIN                 GPIO_PIN_11
#define K230_RX_PORT                GPIOB
#define GPIO_K230_CLK_ENABLE        __HAL_RCC_GPIOB_CLK_ENABLE()

/* UART7: ESP32语音 (PF7/PF6) */
#define ESP32_TX_PIN                GPIO_PIN_7
#define ESP32_TX_PORT               GPIOF
#define ESP32_RX_PIN                GPIO_PIN_6
#define ESP32_RX_PORT               GPIOF
#define GPIO_ESP32_CLK_ENABLE       __HAL_RCC_GPIOF_CLK_ENABLE()

void USART_All_Init(void);
void USART1_SendByte(uint8_t ch);
void USART1_SendStr(char *str);

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2_motor1;
extern UART_HandleTypeDef huart3_k230;
extern UART_HandleTypeDef huart4_motor2;
extern UART_HandleTypeDef huart7_esp32;

#endif
