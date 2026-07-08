#include "sys_state.h"
#include "led.h"
#include <string.h>
#include <stdio.h>

SystemState sys_state = {MODE_HAND, 0, 0, 0};

void sys_state_init(void)
{
    memset(&sys_state, 0, sizeof(sys_state));
    sys_state.current_mode = MODE_HAND;
    sys_state.running = 1;
    sys_state.brightness_level = 80;
    sys_state.color_temp = 4000;
    printf("[SYS] Init done, mode: %s\r\n", sys_mode_name(sys_state.current_mode));
}

void sys_state_switch_mode(SystemMode mode)
{
    if (mode >= MODE_COUNT) return;
    sys_state.current_mode = mode;
    LED1_Toggle;
    printf("[SYS] Mode switched: %s\r\n", sys_mode_name(mode));
}

void sys_state_next_mode(void)
{
    uint8_t next = (sys_state.current_mode + 1) % MODE_COUNT;
    sys_state_switch_mode((SystemMode)next);
}

const char *sys_mode_name(SystemMode mode)
{
    switch (mode)
    {
    case MODE_EMOTION: return "Emotion";
    case MODE_TOMATO:  return "Tomato";
    case MODE_HAND:    return "HandFollow";
    case MODE_SENTRY:  return "Sentry";
    default:           return "Unknown";
    }
}
