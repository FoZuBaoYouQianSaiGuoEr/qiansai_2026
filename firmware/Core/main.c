#include "main.h"
#include "led.h"
#include "usart.h"
#include "lcd_spi_154.h"
#include "dcmi_ov5640.h"
#include "sk9822.h"
#include "k230_proto.h"
#include "esp32_proto.h"
#include "light_ctrl.h"
#include "step_motor.h"
#include "button.h"
#include "sys_state.h"
#include "serial_cmd.h"
#include "network.h"
#include "network_data.h"
#include <stdio.h>

#define Camera_Buffer  0x24000000

static ai_handle network = AI_HANDLE_NULL;
AI_ALIGNED(4) static int8_t in_data[64 * 64];
AI_ALIGNED(4) static int8_t out_data[7];
static ai_buffer ai_input[1];
static ai_buffer ai_output[1];
const char* emotion_labels[7] = {"Angry","Disgust","Fear","Happy","Sad","Surprise","Neutral"};
static volatile int8_t last_emotion_id = -1;
AI_ALIGNED(4) uint8_t ai_act_buf[AI_ACTIVATIONS_SIZE];

static void AI_Init(void)
{
    ai_error err = ai_network_create(&network, AI_NETWORK_DATA_CONFIG);
    if (err.type != AI_ERROR_NONE) {
        printf("[AI] Create failed!\r\n");
        return;
    }
    const ai_network_params params = {
        AI_NETWORK_DATA_WEIGHTS(ai_network_data_weights_get()),
        AI_NETWORK_DATA_ACTIVATIONS(ai_act_buf)
    };
    if (!ai_network_init(network, &params)) {
        printf("[AI] Init failed!\r\n");
        return;
    }
    ai_input[0] = *ai_network_inputs_get(network, NULL);
    ai_output[0] = *ai_network_outputs_get(network, NULL);
    ai_input[0].data = AI_HANDLE_PTR(in_data);
    ai_output[0].data = AI_HANDLE_PTR(out_data);
    printf("[AI] Init OK (fer_mini_xception)\r\n");
}

static void Preprocess_For_AI(uint16_t* rgb565_buf, int8_t* ai_buf)
{
    int idx = 0;
    for (int y = 56; y < 184; y += 2) {
        for (int x = 56; x < 184; x += 2) {
            uint16_t p = rgb565_buf[y * 240 + x];
            uint8_t r = (p >> 11) << 3;
            uint8_t g = ((p >> 5) & 0x3F) << 2;
            uint8_t b = (p & 0x1F) << 3;
            uint8_t gray = (uint8_t)((r * 30 + g * 59 + b * 11) / 100);
            ai_buf[idx++] = (int8_t)(gray - 128);
        }
    }
}

TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim6;

void SystemClock_Config(void);
void MPU_Config(void);
void TIM3_Init(void);
void TIM6_Init(void);

int main(void)
{
    MPU_Config();
    SCB_EnableICache();
    SCB_EnableDCache();
    HAL_Init();
    SystemClock_Config();
    LED_Init();
    USART_All_Init();
    SPI_LCD_Init();
    DCMI_OV5640_Init();

    OV5640_AF_Download_Firmware();
    OV5640_AF_Trigger_Constant();
    OV5640_DMA_Transmit_Continuous(Camera_Buffer, Display_BufferSize);

    button_init();
    sk9822_init(SK9822_NUM_LEDS);
    step_motor_init();
    k230_proto_init();
    esp32_proto_init();
    light_ctrl_init();
    sys_state_init();
    serial_cmd_init();
    AI_Init();

    TIM3_Init();
    TIM6_Init();

    printf("\r\n======== SmartLamp v2 Ready ========\r\n");
    printf("Default mode: %s\r\n", sys_mode_name(sys_state.current_mode));

    sk9822_set_color_temp(4000, 0.3f);
    sk9822_show();

    ButtonEvent ev;

    while (1)
    {
        /* ===== 摄像头+LCD ===== */
        if (OV5640_FrameState == 1)
        {
            OV5640_FrameState = 0;

            SCB_CleanInvalidateDCache_by_Addr((uint32_t*)Camera_Buffer,
                                                OV5640_Width * OV5640_Height * 2);

            LCD_CopyBuffer(0, 0, Display_Width, Display_Height,
                           (uint16_t *)Camera_Buffer);

            static uint8_t ai_frame_skip = 0;
            if (++ai_frame_skip >= 5)
            {
                ai_frame_skip = 0;
                Preprocess_For_AI((uint16_t*)Camera_Buffer, in_data);
                ai_network_run(network, &ai_input[0], &ai_output[0]);

                int8_t max_p = -128;
                int8_t emotion_id = 0;
                for (int i = 0; i < 7; i++) {
                    if (out_data[i] > max_p) {
                        max_p = out_data[i];
                        emotion_id = i;
                    }
                }
                LCD_DisplayString(10, 10, (char*)emotion_labels[emotion_id]);
                if (emotion_id != last_emotion_id) {
                    last_emotion_id = emotion_id;
                    if (sys_state.current_mode == MODE_EMOTION) {
                        light_ctrl_set_emotion((uint8_t)emotion_id);
                        light_need_refresh = 1;
                        printf("[AI] %s\r\n", emotion_labels[emotion_id]);
                    }
                }
            }

            LCD_DisplayString(84, 200, "FPS:");
            LCD_DisplayNumber(132, 200, OV5640_FPS, 2);
            LED1_Toggle;
        }

        /* ===== K230数据(非阻塞) + FD帧解析 ===== */
        {
            uint8_t ch;
            while (HAL_UART_Receive(&huart3_k230, &ch, 1, 0) == HAL_OK)
            {
                k230_parse_byte(ch);
                step_motor_feed_byte(ch);
            }
        }
        if (k230_frame_ready)
        {
            k230_frame_ready = 0;
        }

        /* ===== ESP32数据 ===== */
        esp32_poll();

        /* ===== 串口命令 ===== */
        serial_cmd_poll();

        /* ===== 坐姿检测 ===== */
        {
            static uint8_t last_p1, last_p2, last_str;
            uint8_t p1 = posture_check_1();
            uint8_t p2 = posture_check_2();
            uint8_t str = posture_check_stranger();
            if (p1 != last_p1 || p2 != last_p2 || str != last_str)
            {
                last_p1 = p1; last_p2 = p2; last_str = str;
                if (str)
                {
                    sk9822_set_all(255, 0, 0);
                    light_need_refresh = 1;
                    printf("[STRANGER] RED\r\n");
                }
                else if (p1)
                {
                    sk9822_set_all(255, 0, 0);
                    light_need_refresh = 1;
                    printf("[POSTURE] Level 1 - RED\r\n");
                }
                else if (p2)
                {
                    sk9822_set_all(128, 0, 255);
                    light_need_refresh = 1;
                    printf("[POSTURE] Level 2 - PURPLE\r\n");
                }
                else
                {
                    light_ctrl_set_static(sys_state.color_temp, sys_state.brightness_level);
                    printf("[POSTURE] Normal\r\n");
                }
            }
        }

        /* ===== 按键扫描 ===== */
        ev = button_scan();
        switch (ev)
        {
        case BTN_TOMATO:
            sys_state_switch_mode(MODE_TOMATO);
            break;
        case BTN_HAND:
            sys_state_switch_mode(MODE_HAND);
            break;
        case BTN_SENTRY:
            sys_state_switch_mode(MODE_SENTRY);
            break;
        case BTN_BRUP:
            sys_state.brightness_level += 5;
            if (sys_state.brightness_level > 100)
                sys_state.brightness_level = 100;
            sk9822_set_brightness(sys_state.brightness_level / 100.0f);
            light_need_refresh = 1;
            break;
        case BTN_BRDOWN:
            if (sys_state.brightness_level >= 5)
                sys_state.brightness_level -= 5;
            else
                sys_state.brightness_level = 5;
            sk9822_set_brightness(sys_state.brightness_level / 100.0f);
            light_need_refresh = 1;
            break;
        default:
            break;
        }

        /* ===== 模式逻辑 ===== */
        switch (sys_state.current_mode)
        {
        case MODE_EMOTION:
            light_emotion_run(k230_data.emotion);
            break;
        case MODE_TOMATO:
            light_pomodoro_run();
            break;
        case MODE_HAND:
            break;
        case MODE_SENTRY:
            if (k230_data.sentry_level > 0)
            {
                esp32_send_alarm(k230_data.sentry_level);
                k230_data.sentry_level = 0;
            }
            break;
        default:
            break;
        }

        /* ===== 灯光刷新 ===== */
        light_ctrl_tick();
        if (light_need_refresh)
        {
            sk9822_show();
            light_need_refresh = 0;
        }
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim == &htim3)
    {
        light_need_refresh = 1;
    }
    else if (htim == &htim6)
    {
        if (pomodoro_running && pomodoro_timer_sec > 0)
            pomodoro_timer_sec--;
    }
}

void TIM3_Init(void)
{
    __HAL_RCC_TIM3_CLK_ENABLE();
    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 240 - 1;
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 1000 - 1;
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_Base_Init(&htim3);
    HAL_NVIC_SetPriority(TIM3_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(TIM3_IRQn);
    HAL_TIM_Base_Start_IT(&htim3);
}

void TIM6_Init(void)
{
    __HAL_RCC_TIM6_CLK_ENABLE();
    htim6.Instance = TIM6;
    htim6.Init.Prescaler = 24000 - 1;
    htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim6.Init.Period = 10000 - 1;
    htim6.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_Base_Init(&htim6);
    HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 4, 0);
    HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
    HAL_TIM_Base_Start_IT(&htim6);
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

    HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);
    while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

    __HAL_RCC_PLL_PLLSOURCE_CONFIG(RCC_PLLSOURCE_HSE);
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 5;
    RCC_OscInitStruct.PLL.PLLN = 192;
    RCC_OscInitStruct.PLL.PLLP = 2;
    RCC_OscInitStruct.PLL.PLLQ = 4;
    RCC_OscInitStruct.PLL.PLLR = 2;
    RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
    RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
    RCC_OscInitStruct.PLL.PLLFRACN = 0;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
        Error_Handler();

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2
                                | RCC_CLOCKTYPE_D3PCLK1 | RCC_CLOCKTYPE_D1PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
    RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
        Error_Handler();

    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART1 |
                                                RCC_PERIPHCLK_SPI1 |
                                                RCC_PERIPHCLK_SPI4;
    PeriphClkInitStruct.Usart16ClockSelection = RCC_USART16CLKSOURCE_D2PCLK2;
    PeriphClkInitStruct.Spi123ClockSelection = RCC_SPI123CLKSOURCE_PLL;
    PeriphClkInitStruct.Spi45ClockSelection = RCC_SPI45CLKSOURCE_D2PCLK1;

    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
        Error_Handler();
}

void MPU_Config(void)
{
    MPU_Region_InitTypeDef MPU_InitStruct;
    HAL_MPU_Disable();
    MPU_InitStruct.Enable = MPU_REGION_ENABLE;
    MPU_InitStruct.BaseAddress = 0x24000000;
    MPU_InitStruct.Size = MPU_REGION_SIZE_512KB;
    MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
    MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
    MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
    MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
    MPU_InitStruct.Number = MPU_REGION_NUMBER0;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
    MPU_InitStruct.SubRegionDisable = 0x00;
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

void Error_Handler(void)
{
    __disable_irq();
    while (1) {}
}
