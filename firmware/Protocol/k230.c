#include "k230_proto.h"
#include "usart.h"
#include <string.h>

K230_Data k230_data = {0};
volatile uint8_t k230_frame_ready = 0;

static uint8_t rx_buf[K230_RX_BUF_SIZE];
static volatile uint8_t rx_idx;
static volatile uint8_t frame_state;
static uint8_t frame_len;
static uint8_t frame_cmd;

void k230_proto_init(void)
{
    memset(&k230_data, 0, sizeof(k230_data));
    memset(rx_buf, 0, sizeof(rx_buf));
    rx_idx = 0;
    frame_state = 0;
    printf("[K230] Proto init done\r\n");
}

static uint8_t k230_xor_check(const uint8_t *data, uint8_t len)
{
    uint8_t check = 0;
    for (uint8_t i = 0; i < len; i++) check ^= data[i];
    return check;
}

void k230_parse_byte(uint8_t byte)
{
    switch (frame_state)
    {
    case 0:
        if (byte == K230_FRAME_HEAD1) { rx_buf[0] = byte; frame_state = 1; }
        break;
    case 1:
        if (byte == K230_FRAME_HEAD2) { rx_buf[1] = byte; frame_state = 2; rx_idx = 2; }
        else frame_state = 0;
        break;
    case 2:
        frame_len = byte;
        rx_buf[rx_idx++] = byte;
        frame_state = 3;
        break;
    case 3:
        frame_cmd = byte;
        rx_buf[rx_idx++] = byte;
        frame_state = 4;
        break;
    case 4:
        if (rx_idx < frame_len + 5)
        {
            rx_buf[rx_idx++] = byte;
        }
        if (rx_idx >= frame_len + 5)
        {
            uint8_t calc = k230_xor_check(rx_buf, rx_idx - 1);
            if (calc == rx_buf[rx_idx - 1])
            {
                k230_parse_frame();
            }
            frame_state = 0;
            rx_idx = 0;
        }
        break;
    default:
        frame_state = 0;
        rx_idx = 0;
        break;
    }
}

void k230_parse_frame(void)
{
    uint8_t *payload = &rx_buf[4];

    switch (frame_cmd)
    {
    case K230_CMD_GESTURE:
        if (frame_len >= 5)
        {
            k230_data.gesture_id = payload[0];
            k230_data.hand_x = payload[1] | ((uint16_t)payload[2] << 8);
            k230_data.hand_y = payload[3] | ((uint16_t)payload[4] << 8);
            k230_data.valid = 1;
        }
        break;
    case K230_CMD_POSTURE:
        if (frame_len >= 1)
        {
            k230_data.posture = payload[0];
            k230_data.valid = 1;
        }
        break;
    case K230_CMD_EMOTION:
        if (frame_len >= 1)
        {
            k230_data.emotion = payload[0];
            k230_data.valid = 1;
        }
        break;
    case K230_CMD_POMODORO:
        if (frame_len >= 1)
        {
            k230_data.pomodoro_phase = payload[0];
        }
        break;
    case K230_CMD_SENTRY:
        if (frame_len >= 1)
        {
            k230_data.sentry_level = payload[0];
            k230_data.valid = 1;
        }
        break;
    default:
        break;
    }
    k230_frame_ready = 1;
}
