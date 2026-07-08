#ifndef __SYS_STATE_H
#define __SYS_STATE_H

#include "stm32h7xx_hal.h"

typedef enum {
    MODE_EMOTION = 0,
    MODE_TOMATO  = 1,
    MODE_HAND    = 2,
    MODE_SENTRY  = 3,
    MODE_COUNT
} SystemMode;

typedef struct {
    SystemMode  current_mode;
    uint8_t     running;
    uint8_t     brightness_level;
    uint16_t    color_temp;
} SystemState;

extern SystemState sys_state;

void sys_state_init(void);
void sys_state_switch_mode(SystemMode mode);
void sys_state_next_mode(void);
const char *sys_mode_name(SystemMode mode);

#endif
