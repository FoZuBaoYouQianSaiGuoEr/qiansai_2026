#ifndef __STEP_MOTOR_H
#define __STEP_MOTOR_H

#include "stm32h7xx_hal.h"

#define MOTOR1_ADDR  1
#define MOTOR2_ADDR  2
#define STEPS_360    3200

#define POSTURE1_PIN  GPIO_PIN_8
#define POSTURE1_PORT GPIOD
#define POSTURE2_PIN  GPIO_PIN_9
#define POSTURE2_PORT GPIOD
#define STRANGER_PIN  GPIO_PIN_10
#define STRANGER_PORT GPIOD
#define POSTURE_CLK_ENABLE  __HAL_RCC_GPIOD_CLK_ENABLE()

void step_motor_init(void);
void step_motor_fd_send(UART_HandleTypeDef *uart, uint8_t addr,
                         uint8_t dir, uint16_t rpm, uint8_t accel, uint32_t steps);

void step_motor_angle_move(uint8_t motor, float angle_deg,
                            uint8_t dir, uint16_t rpm, uint8_t accel);

void step_motor_feed_byte(uint8_t byte);

uint8_t posture_check_1(void);
uint8_t posture_check_2(void);
uint8_t posture_check_stranger(void);

#endif
