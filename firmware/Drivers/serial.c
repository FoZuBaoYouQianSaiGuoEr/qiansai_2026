#include "serial_cmd.h"
#include "usart.h"
#include "step_motor.h"
#include "sk9822.h"
#include "light_ctrl.h"
#include "sys_state.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define CMD_BUF_SIZE 64
static char cmd_buf[CMD_BUF_SIZE];
static uint8_t cmd_idx;
static uint32_t last_rx_tick;
static volatile uint8_t rx_char;
static volatile uint8_t rx_flag;

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart1)
    {
        rx_char = huart->Instance->RDR;
        rx_flag = 1;
        HAL_UART_Receive_IT(&huart1, (uint8_t *)&rx_char, 1);
    }
}

static void exec_cmd(void)
{
    cmd_buf[cmd_idx] = '\0';
    char *p = cmd_buf;

    if (strncmp(p, "M ", 2) == 0 || strncmp(p, "m ", 2) == 0)
    {
        int motor = atoi(p + 2);
        char *sp = strchr(p + 2, ' ');
        float angle = sp ? atof(sp + 1) : 0;
        if (motor >= 0 && motor <= 1)
        {
            step_motor_angle_move((uint8_t)motor, angle, 0, 200, 10);
            printf("OK: motor %d -> %.1f deg\r\n", motor, angle);
        }
        else printf("ERR: motor 0(left) or 1(up)\r\n");
    }
    else if (strncmp(p, "MR ", 3) == 0 || strncmp(p, "mr ", 3) == 0)
    {
        int motor = atoi(p + 3);
        char *sp = strchr(p + 3, ' ');
        float angle = sp ? atof(sp + 1) : 0;
        if (motor >= 0 && motor <= 1)
        {
            step_motor_angle_move((uint8_t)motor, angle, 1, 200, 10);
            printf("OK: motor %d rev -> %.1f deg\r\n", motor, angle);
        }
    }
    else if (strncmp(p, "MODE ", 5) == 0 || strncmp(p, "mode ", 5) == 0)
    {
        int m = atoi(p + 5);
        sys_state_switch_mode((SystemMode)m);
    }
    else if (strncmp(p, "BR ", 3) == 0 || strncmp(p, "br ", 3) == 0)
    {
        int br = atoi(p + 3);
        sys_state.brightness_level = (uint8_t)br;
        sk9822_set_brightness(br / 100.0f);
        light_need_refresh = 1;
        printf("OK: br -> %d\r\n", br);
    }
    else if (strncmp(p, "TEMP ", 5) == 0 || strncmp(p, "temp ", 5) == 0)
    {
        int k = atoi(p + 5);
        sys_state.color_temp = (uint16_t)k;
        sk9822_set_color_temp((uint16_t)k, 1.0f);
        light_need_refresh = 1;
        printf("OK: temp -> %dK\r\n", k);
    }
    else if (strncmp(p, "HELP", 4) == 0 || strncmp(p, "help", 4) == 0)
    {
        serial_cmd_init();
    }
    else
    {
        printf("ERR: '%s' type HELP\r\n", cmd_buf);
    }
    cmd_idx = 0;
    memset(cmd_buf, 0, CMD_BUF_SIZE);
}

void serial_cmd_init(void)
{
    memset(cmd_buf, 0, CMD_BUF_SIZE);
    cmd_idx = 0;
    rx_flag = 0;
    printf("\r\n==== Serial CMD ====\r\n");
    printf("M 0 15  - motor0 +15 deg\r\n");
    printf("MR 0 15 - motor0 -15 deg\r\n");
    printf("MODE 2  - hand follow\r\n");
    printf("BR 80   - brightness\r\n");
    printf("HELP    - list\r\n");
    printf("====================\r\n");
    HAL_UART_Receive_IT(&huart1, (uint8_t *)&rx_char, 1);
}

void serial_cmd_poll(void)
{
    if (rx_flag)
    {
        uint8_t ch = rx_char;
        rx_flag = 0;
        last_rx_tick = HAL_GetTick();

        if (ch == '\r' || ch == '\n')
        {
            if (cmd_idx > 0) exec_cmd();
        }
        else if (ch == 0x08 || ch == 0x7F)
        {
            if (cmd_idx > 0) cmd_idx--;
        }
        else if (ch >= ' ' && ch <= '~')
        {
            if (cmd_idx < CMD_BUF_SIZE - 1)
                cmd_buf[cmd_idx++] = (char)ch;
            printf("%c", ch);
        }
    }

    if (cmd_idx > 0 && HAL_GetTick() - last_rx_tick > 200)
    {
        exec_cmd();
    }
}
