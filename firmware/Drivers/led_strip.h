#ifndef __SK9822_H
#define __SK9822_H

#include "stm32h7xx_hal.h"

#define SK9822_NUM_LEDS     30

#define SK9822_COLOR(r, g, b)  ((uint32_t)(0xE0 | ((b) & 0x1F)) << 24 | \
                                 (uint32_t)(g) << 16 | \
                                 (uint32_t)(r) << 8 | \
                                 (uint32_t)(b) >> 5)

void sk9822_init(uint16_t num_leds);
void sk9822_show(void);
void sk9822_set_led(uint16_t index, uint8_t r, uint8_t g, uint8_t b);
void sk9822_set_all(uint8_t r, uint8_t g, uint8_t b);
void sk9822_set_brightness(float factor);
void sk9822_set_color_temp(uint16_t kelvin, float brightness);

void sk9822_effect_breath(uint8_t r, uint8_t g, uint8_t b, uint16_t period_ms);
void sk9822_effect_rainbow(uint16_t offset);
void sk9822_effect_gradient(uint8_t r1, uint8_t g1, uint8_t b1,
                             uint8_t r2, uint8_t g2, uint8_t b2, float t);
void sk9822_effect_tick(void);

#endif
