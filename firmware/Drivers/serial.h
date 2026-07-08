#ifndef __SERIAL_CMD_H
#define __SERIAL_CMD_H

#include "stm32h7xx_hal.h"

void serial_cmd_init(void);
void serial_cmd_feed(uint8_t ch);
void serial_cmd_poll(void);

#endif
