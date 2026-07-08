#ifndef __K230_PROTO_H
#define __K230_PROTO_H

#include "stm32h7xx_hal.h"

#define K230_FRAME_HEAD1    0xAA
#define K230_FRAME_HEAD2    0x55
#define K230_RX_BUF_SIZE    128

#define K230_CMD_GESTURE     0x01
#define K230_CMD_POSTURE     0x02
#define K230_CMD_EMOTION     0x03
#define K230_CMD_POMODORO    0x04
#define K230_CMD_SENTRY      0x05

typedef struct {
    uint8_t valid;

    uint8_t gesture_id;
    uint16_t hand_x;
    uint16_t hand_y;

    uint8_t posture;
    uint8_t emotion;
    uint8_t pomodoro_phase;
    uint8_t sentry_level;
} K230_Data;

extern K230_Data k230_data;
extern volatile uint8_t k230_frame_ready;

void k230_proto_init(void);
void k230_parse_byte(uint8_t byte);
void k230_parse_frame(void);

#endif
