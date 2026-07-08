#include "sk9822.h"
#include <string.h>
#include <math.h>
#include <stdio.h>

static SPI_HandleTypeDef hspi1;
static uint32_t led_buf[SK9822_NUM_LEDS + 2];
static uint16_t led_count = SK9822_NUM_LEDS;
static float brightness_global = 1.0f;

void sk9822_init(uint16_t num_leds)
{
    led_count = num_leds;

    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_1LINE;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 7;
    HAL_SPI_Init(&hspi1);

    memset(led_buf, 0, sizeof(led_buf));
    led_buf[0] = 0x00000000;
    led_buf[led_count + 1] = 0x00000000;

    sk9822_set_all(0, 0, 0);
    sk9822_show();
    printf("[SK9822] Init done, %d LEDs\r\n", led_count);
}

void sk9822_set_led(uint16_t index, uint8_t r, uint8_t g, uint8_t b)
{
    if (index >= led_count) return;

    float br = brightness_global;
    r = (uint8_t)(r * br);
    g = (uint8_t)(g * br);
    b = (uint8_t)(b * br);

    led_buf[index + 1] = SK9822_COLOR(r, g, b);
}

void sk9822_set_all(uint8_t r, uint8_t g, uint8_t b)
{
    float br = brightness_global;
    uint8_t rr = (uint8_t)(r * br);
    uint8_t gg = (uint8_t)(g * br);
    uint8_t bb = (uint8_t)(b * br);
    uint32_t color = SK9822_COLOR(rr, gg, bb);

    for (uint16_t i = 0; i < led_count; i++)
    {
        led_buf[i + 1] = color;
    }
}

void sk9822_show(void)
{
    led_buf[led_count + 1] = 0x00000000;
    HAL_SPI_Transmit(&hspi1, (uint8_t *)led_buf,
                     (led_count + 2) * 4, 100);
}

void sk9822_set_brightness(float factor)
{
    if (factor < 0) factor = 0;
    if (factor > 1.0f) factor = 1.0f;
    brightness_global = factor;
}

static void hsv2rgb(float h, float s, float v, uint8_t *r, uint8_t *g, uint8_t *b)
{
    float c = v * s;
    float x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;
    float rr, gg, bb;

    if (h < 60)       { rr = c; gg = x; bb = 0; }
    else if (h < 120) { rr = x; gg = c; bb = 0; }
    else if (h < 180) { rr = 0; gg = c; bb = x; }
    else if (h < 240) { rr = 0; gg = x; bb = c; }
    else if (h < 300) { rr = x; gg = 0; bb = c; }
    else              { rr = c; gg = 0; bb = x; }

    *r = (uint8_t)((rr + m) * 255);
    *g = (uint8_t)((gg + m) * 255);
    *b = (uint8_t)((bb + m) * 255);
}

void sk9822_effect_breath(uint8_t r, uint8_t g, uint8_t b, uint16_t period_ms)
{
    static uint32_t last_tick;
    uint32_t now = HAL_GetTick();
    if (now - last_tick < 10) return;
    last_tick = now;

    float phase = (float)(now % period_ms) / period_ms;
    float brightness = (sinf(phase * 2.0f * 3.14159f) + 1.0f) / 2.0f;

    sk9822_set_all((uint8_t)(r * brightness),
                   (uint8_t)(g * brightness),
                   (uint8_t)(b * brightness));
}

void sk9822_effect_rainbow(uint16_t offset)
{
    uint8_t r, g, b;
    for (uint16_t i = 0; i < led_count; i++)
    {
        float hue = fmodf((float)((i * 256 / led_count + offset) % 256) * 360.0f / 256.0f, 360.0f);
        hsv2rgb(hue, 1.0f, 0.3f, &r, &g, &b);
        sk9822_set_led(i, r, g, b);
    }
}

void sk9822_effect_gradient(uint8_t r1, uint8_t g1, uint8_t b1,
                             uint8_t r2, uint8_t g2, uint8_t b2, float t)
{
    if (t < 0) t = 0;
    if (t > 1.0f) t = 1.0f;

    uint8_t r = (uint8_t)(r1 + (r2 - r1) * t);
    uint8_t g = (uint8_t)(g1 + (g2 - g1) * t);
    uint8_t b = (uint8_t)(b1 + (b2 - b1) * t);
    sk9822_set_all(r, g, b);
}

void sk9822_set_color_temp(uint16_t kelvin, float brightness)
{
    float temp = kelvin / 100.0f;
    uint8_t r, g, b;

    if (temp <= 66)
    {
        r = 255;
        g = (uint8_t)(99.4708025861f * logf(temp) - 161.1195681661f);
        if (temp <= 19) b = 0;
        else b = (uint8_t)(138.5177312231f * logf(temp - 10) - 305.0447927307f);
    }
    else
    {
        r = (uint8_t)(329.698727446f * powf(temp - 60, -0.1332047592f));
        g = (uint8_t)(288.1221695283f * powf(temp - 60, -0.0755148492f));
        b = 255;
    }

    if (r > 255) r = 255; if (g > 255) g = 255; if (b > 255) b = 255;

    float br = brightness * brightness_global;
    sk9822_set_all((uint8_t)(r * br), (uint8_t)(g * br), (uint8_t)(b * br));
}

void sk9822_effect_tick(void)
{
}
