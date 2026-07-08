#include "esp32_proto.h"
#include "usart.h"
#include "light_ctrl.h"
#include "sk9822.h"
#include <string.h>
#include <stdio.h>

#define ESP32_BUF_SIZE 128
static char rx_buf[ESP32_BUF_SIZE];
static uint8_t rx_idx;

void esp32_proto_init(void)
{
    memset(rx_buf, 0, ESP32_BUF_SIZE);
    rx_idx = 0;
    printf("[ESP32] Text proto init done\r\n");
}

static uint8_t emotion_str_to_id(const char *s)
{
    if (strstr(s, "angry"))    return EMOTION_ANGRY;
    if (strstr(s, "disgust"))  return EMOTION_DISGUST;
    if (strstr(s, "fear"))     return EMOTION_FEAR;
    if (strstr(s, "happy"))    return EMOTION_HAPPY;
    if (strstr(s, "sad"))      return EMOTION_SAD;
    if (strstr(s, "surprise")) return EMOTION_SURPRISE;
    if (strstr(s, "confused")) return EMOTION_CONFUSED;
    if (strstr(s, "neutral"))  return EMOTION_NEUTRAL;
    return EMOTION_NEUTRAL;
}

static void process_line(char *line)
{
    printf("[ESP32] %s\r\n", line);

    if (strstr(line, "EMOTION:"))
    {
        uint8_t id = emotion_str_to_id(line);
        light_ctrl_set_emotion(id);
        light_need_refresh = 1;
    }
    else if (strstr(line, "LIGHT:MODE:focus"))
    {
        light_ctrl_set_static(5500, 80);
        light_need_refresh = 1;
    }
    else if (strstr(line, "LIGHT:MODE:rest"))
    {
        light_ctrl_set_static(3200, 60);
        light_need_refresh = 1;
    }
    else if (strstr(line, "LIGHT:MODE:off"))
    {
        sk9822_set_all(0, 0, 0);
        light_need_refresh = 1;
    }
    else if (strstr(line, "POMODORO:START:"))
    {
        char *p = strstr(line, "POMODORO:START:") + 15;
        int minutes = atoi(p);
        if (minutes > 0)
        {
            pomodoro_running = 1;
            pomodoro_timer_sec = (uint32_t)minutes * 60;
            light_ctrl_set_static(5500, 80);
            light_need_refresh = 1;
            printf("[POMODORO] AI start %dmin\r\n", minutes);
        }
    }
}

void esp32_poll(void)
{
    uint8_t ch;
    while (HAL_UART_Receive(&huart7_esp32, &ch, 1, 0) == HAL_OK)
    {
        if (ch == '\n')
        {
            if (rx_idx > 0 && rx_idx < ESP32_BUF_SIZE)
            {
                rx_buf[rx_idx] = '\0';
                process_line(rx_buf);
            }
            rx_idx = 0;
            memset(rx_buf, 0, ESP32_BUF_SIZE);
        }
        else if (ch >= ' ' && ch <= '~')
        {
            if (rx_idx < ESP32_BUF_SIZE - 1)
                rx_buf[rx_idx++] = (char)ch;
        }
    }
}
