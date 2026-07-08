#ifndef __LIGHT_CTRL_H
#define __LIGHT_CTRL_H

#include "stm32h7xx_hal.h"

#define EMOTION_ANGRY     0
#define EMOTION_DISGUST   1
#define EMOTION_FEAR      2
#define EMOTION_HAPPY     3
#define EMOTION_SAD       4
#define EMOTION_SURPRISE  5
#define EMOTION_NEUTRAL   6
#define EMOTION_CONFUSED  7

#define POMODORO_FOCUS    0
#define POMODORO_REST     1

#define POMODORO_FOCUS_SEC   (25 * 60)
#define POMODORO_REST_SEC    (5 * 60)

typedef enum {
    LIGHT_OFF = 0,
    LIGHT_BREATH,
    LIGHT_RAINBOW,
    LIGHT_PULSE,
    LIGHT_GRADIENT,
    LIGHT_STATIC
} LightEffect;

typedef struct {
    LightEffect effect;
    uint16_t    color_temp;
    uint8_t     brightness;
    uint8_t     r, g, b;
    uint16_t    period_ms;
    float       gradient_t;
} LightState;

extern LightState light_state;
extern volatile uint8_t light_need_refresh;

void light_ctrl_init(void);
void light_ctrl_set_emotion(uint8_t emotion_id);
void light_pomodoro_run(void);
void light_emotion_run(uint8_t emotion_id);
void light_ctrl_set_static(uint16_t kelvin, uint8_t bright);
void light_ctrl_tick(void);

extern volatile uint32_t pomodoro_timer_sec;
extern volatile uint8_t pomodoro_running;

#endif
