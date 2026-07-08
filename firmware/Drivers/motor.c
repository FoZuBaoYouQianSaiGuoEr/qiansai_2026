#include "step_motor.h"
#include "usart.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

#define FD_CMD    0xFD
#define FD_CHECK  0x6B

static uint8_t hex_buf[78];
static uint8_t hex_count;
static uint8_t raw_buf[13];
static uint8_t raw_idx;

#define IS_HEX(c) (((c)>='0'&&(c)<='9')||((c)>='A'&&(c)<='F')||((c)>='a'&&(c)<='f'))

void step_motor_init(void)
{
    POSTURE_CLK_ENABLE;

    GPIO_InitTypeDef g = {0};
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_PULLDOWN;
    g.Speed = GPIO_SPEED_FREQ_LOW;

    g.Pin = POSTURE1_PIN;
    HAL_GPIO_Init(POSTURE1_PORT, &g);

    g.Pin = POSTURE2_PIN;
    HAL_GPIO_Init(POSTURE2_PORT, &g);

    g.Pin = STRANGER_PIN;
    HAL_GPIO_Init(STRANGER_PORT, &g);

    hex_count = 0;
    raw_idx = 0;
    memset(hex_buf, 0, sizeof(hex_buf));
    memset(raw_buf, 0, sizeof(raw_buf));

    printf("[MOTOR] Init done, posture: PD8/PD9, stranger: PD10\r\n");
}

void step_motor_fd_send(UART_HandleTypeDef *uart, uint8_t addr,
                         uint8_t dir, uint16_t rpm, uint8_t accel, uint32_t steps)
{
    uint8_t frame[13];
    frame[0]  = addr;
    frame[1]  = FD_CMD;
    frame[2]  = dir;
    frame[3]  = (uint8_t)(rpm >> 8);
    frame[4]  = (uint8_t)(rpm & 0xFF);
    frame[5]  = accel;
    frame[6]  = (uint8_t)(steps >> 24);
    frame[7]  = (uint8_t)(steps >> 16);
    frame[8]  = (uint8_t)(steps >> 8);
    frame[9]  = (uint8_t)(steps & 0xFF);
    frame[10] = 0x00;
    frame[11] = 0x00;
    frame[12] = FD_CHECK;

    HAL_UART_Transmit(uart, frame, 13, 20);
}

void step_motor_angle_move(uint8_t motor, float angle_deg,
                            uint8_t dir, uint16_t rpm, uint8_t accel)
{
    uint32_t steps = (uint32_t)(fabsf(angle_deg) * STEPS_360 / 360.0f);
    uint8_t addr = (motor == 0) ? MOTOR1_ADDR : MOTOR2_ADDR;
    UART_HandleTypeDef *uart = (motor == 0) ? &huart2_motor1 : &huart4_motor2;

    printf("[MOTOR] M%d addr=%d %.1f deg %d steps dir=%d rpm=%d\r\n",
           motor, addr, angle_deg, (int)steps, dir, rpm);

    step_motor_fd_send(uart, addr, dir, rpm, accel, steps);
}

uint8_t posture_check_1(void)
{
    return (HAL_GPIO_ReadPin(POSTURE1_PORT, POSTURE1_PIN) == GPIO_PIN_SET) ? 1 : 0;
}

uint8_t posture_check_2(void)
{
    return (HAL_GPIO_ReadPin(POSTURE2_PORT, POSTURE2_PIN) == GPIO_PIN_SET) ? 1 : 0;
}

uint8_t posture_check_stranger(void)
{
    return (HAL_GPIO_ReadPin(STRANGER_PORT, STRANGER_PIN) == GPIO_PIN_SET) ? 1 : 0;
}

/* K230 FD帧解析: 支持 ASCII hex 和 原始二进制两种格式 */
void step_motor_feed_byte(uint8_t c)
{
    if (IS_HEX(c))
    {
        raw_idx = 0;
        if (c >= 'a') c -= 32;
        if (hex_count >= 76)
        {
            for (uint8_t i = 0; i < hex_count - 2; i++)
                hex_buf[i] = hex_buf[i + 2];
            hex_count -= 2;
        }
        hex_buf[hex_count++] = c;

        for (uint8_t pos = 0; pos + 26 <= hex_count; pos++)
        {
            if (hex_buf[pos] == '0' &&
                hex_buf[pos+2] == 'F' && hex_buf[pos+3] == 'D' &&
                (hex_buf[pos+1] == '1' || hex_buf[pos+1] == '2'))
            {
                uint8_t bin[13];
                for (uint8_t i = 0; i < 13; i++)
                {
                    uint8_t hi = hex_buf[pos + i*2];
                    uint8_t lo = hex_buf[pos + i*2 + 1];
                    hi = (hi >= 'A') ? (hi - 'A' + 10) : (hi - '0');
                    lo = (lo >= 'A') ? (lo - 'A' + 10) : (lo - '0');
                    bin[i] = (hi << 4) | lo;
                }
                if (bin[12] == FD_CHECK)
                {
                    UART_HandleTypeDef *uart = (bin[0] == 1) ? &huart2_motor1 : &huart4_motor2;
                    HAL_UART_Transmit(uart, bin, 13, 20);
                    printf("[MOTOR FD] addr=%d dir=%d rpm=%d steps=%d\r\n",
                           bin[0], bin[2],
                           (bin[3]<<8)|bin[4],
                           (bin[6]<<24)|(bin[7]<<16)|(bin[8]<<8)|bin[9]);
                }
                hex_count -= (pos + 26);
                for (uint8_t i = 0; i < hex_count; i++)
                    hex_buf[i] = hex_buf[pos + 26 + i];
                break;
            }
        }
    }
    else if (c == 0x01 || c == 0x02)
    {
        hex_count = 0;
        raw_buf[0] = c;
        raw_idx = 1;
    }
    else if (raw_idx > 0 && raw_idx < 13)
    {
        raw_buf[raw_idx++] = c;
        if (raw_idx >= 13)
        {
            raw_idx = 0;
            if (raw_buf[1] == FD_CMD && raw_buf[12] == FD_CHECK)
            {
                UART_HandleTypeDef *uart = (raw_buf[0] == 1) ? &huart2_motor1 : &huart4_motor2;
                HAL_UART_Transmit(uart, raw_buf, 13, 20);
                printf("[MOTOR FD bin] addr=%d dir=%d\r\n", raw_buf[0], raw_buf[2]);
            }
        }
    }
    else
    {
        raw_idx = 0;
    }
}
