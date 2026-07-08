#include "light_ctrl.h"
#include "sk9822.h"
#include <stdio.h>
#include <string.h>

LightState light_state;
volatile uint8_t light_need_refresh = 0;
volatile uint32_t pomodoro_timer_sec = 0;
volatile uint8_t pomodoro_phase = POMODORO_FOCUS;
volatile uint8_t pomodoro_running = 0;

void light_ctrl_init(void)
{
    memset(&light_state, 0, sizeof(light_state));
    light_state.effect = LIGHT_OFF;
    light_state.brightness = 80;
    light_state.color_temp = 4000;
    light_state.period_ms = 3000;

    sk9822_set_color_temp(4000, 0.3f);
    sk9822_show();
    printf("[LIGHT] Init done\r\n");
}

void light_ctrl_set_static(uint16_t kelvin, uint8_t bright)
{
    sk9822_set_brightness(bright / 100.0f);
    sk9822_set_color_temp(kelvin, 1.0f);
    light_need_refresh = 1;
}

void light_ctrl_set_emotion(uint8_t emotion_id)
{
    switch (emotion_id)
    {
    case EMOTION_ANGRY:
        light_state.effect = LIGHT_BREATH;
        light_state.r = 100; light_state.g = 150; light_state.b = 255;
        light_state.period_ms = 1500;
        light_state.brightness = 70;
        break;
    case EMOTION_DISGUST:
        light_state.effect = LIGHT_BREATH;
        light_state.r = 80; light_state.g = 200; light_state.b = 120;
        light_state.period_ms = 4000;
        light_state.brightness = 50;
        break;
    case EMOTION_FEAR:
        light_state.effect = LIGHT_BREATH;
        light_state.r = 255; light_state.g = 140; light_state.b = 40;
        light_state.period_ms = 3000;
        light_state.brightness = 60;
        break;
    case EMOTION_HAPPY:
        light_state.effect = LIGHT_RAINBOW;
        light_state.period_ms = 2000;
        light_state.brightness = 75;
        break;
    case EMOTION_SAD:
        light_state.effect = LIGHT_BREATH;
        light_state.r = 255; light_state.g = 180; light_state.b = 80;
        light_state.period_ms = 5000;
        light_state.brightness = 70;
        break;
    case EMOTION_SURPRISE:
        light_state.effect = LIGHT_PULSE;
        light_state.r = 200; light_state.g = 220; light_state.b = 255;
        light_state.period_ms = 800;
        light_state.brightness = 80;
        break;
    case EMOTION_CONFUSED:
        light_state.effect = LIGHT_BREATH;
        light_state.r = 180; light_state.g = 100; light_state.b = 255;
        light_state.period_ms = 1500;
        light_state.brightness = 60;
        break;
    case EMOTION_NEUTRAL:
    default:
        light_state.effect = LIGHT_STATIC;
        light_state.color_temp = 4000;
        light_state.brightness = 40;
        sk9822_set_brightness(0.4f);
        sk9822_set_color_temp(4000, 1.0f);
        light_need_refresh = 1;
        return;
    }
    sk9822_set_brightness(light_state.brightness / 100.0f);
}

void light_emotion_run(uint8_t emotion_id)
{
    light_ctrl_set_emotion(emotion_id);
}

void light_pomodoro_run(void)
{
    if (!pomodoro_running)
    {
        pomodoro_running = 1;
        pomodoro_phase = POMODORO_FOCUS;
        pomodoro_timer_sec = POMODORO_FOCUS_SEC;
        light_ctrl_set_static(5500, 80);
        printf("[POMODORO] Focus phase start, 25min\r\n");
    }

    if (pomodoro_timer_sec == 0)
    {
        if (pomodoro_phase == POMODORO_FOCUS)
        {
            pomodoro_phase = POMODORO_REST;
            pomodoro_timer_sec = POMODORO_REST_SEC;
            light_ctrl_set_static(3200, 60);
            printf("[POMODORO] Rest phase start, 5min\r\n");
        }
        else
        {
            pomodoro_phase = POMODORO_FOCUS;
            pomodoro_timer_sec = POMODORO_FOCUS_SEC;
            light_ctrl_set_static(5500, 80);
            printf("[POMODORO] Focus phase start, 25min\r\n");
        }
    }
}

void light_ctrl_tick(void)
{
    static uint32_t anim_tick;

    switch (light_state.effect)
    {
    case LIGHT_BREATH:
        sk9822_effect_breath(light_state.r, light_state.g, light_state.b,
                              light_state.period_ms);
        light_need_refresh = 1;
        break;
    case LIGHT_RAINBOW:
        anim_tick += 2;
        sk9822_effect_rainbow(anim_tick % 256);
        light_need_refresh = 1;
        break;
    case LIGHT_PULSE:
    {
        uint32_t now = HAL_GetTick();
        float phase = (float)(now % light_state.period_ms) / light_state.period_ms;
        float br = (sinf(phase * 2.0f * 3.14159f) + 1.0f) / 2.0f;
        sk9822_set_brightness(br);
        sk9822_set_all(light_state.r, light_state.g, light_state.b);
        light_need_refresh = 1;
        break;
    }
    default:
        break;
    }
}
