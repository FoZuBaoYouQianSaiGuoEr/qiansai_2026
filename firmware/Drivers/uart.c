#include "usart.h"

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2_motor1;
UART_HandleTypeDef huart3_k230;
UART_HandleTypeDef huart4_motor2;
UART_HandleTypeDef huart7_esp32;

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef g = {0};

    if (huart->Instance == USART1)
    {
        __HAL_RCC_USART1_CLK_ENABLE();
        GPIO_USART1_CLK_ENABLE;
        g.Pin = USART1_TX_PIN | USART1_RX_PIN;
        g.Mode = GPIO_MODE_AF_PP;
        g.Pull = GPIO_PULLUP;
        g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        g.Alternate = GPIO_AF7_USART1;
        HAL_GPIO_Init(USART1_TX_PORT, &g);
    }
    else if (huart->Instance == USART2)
    {
        __HAL_RCC_USART2_CLK_ENABLE();
        GPIO_MOTOR1_CLK_ENABLE;
        g.Pin = MOTOR1_TX_PIN | MOTOR1_RX_PIN;
        g.Mode = GPIO_MODE_AF_PP;
        g.Pull = GPIO_PULLUP;
        g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        g.Alternate = GPIO_AF7_USART2;
        HAL_GPIO_Init(MOTOR1_TX_PORT, &g);
    }
    else if (huart->Instance == UART4)
    {
        __HAL_RCC_UART4_CLK_ENABLE();
        GPIO_MOTOR2_CLK_ENABLE;
        g.Pin = MOTOR2_TX_PIN | MOTOR2_RX_PIN;
        g.Mode = GPIO_MODE_AF_PP;
        g.Pull = GPIO_PULLUP;
        g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        g.Alternate = GPIO_AF8_UART4;
        HAL_GPIO_Init(MOTOR2_TX_PORT, &g);
    }
    else if (huart->Instance == USART3)
    {
        __HAL_RCC_USART3_CLK_ENABLE();
        GPIO_K230_CLK_ENABLE;
        g.Pin = K230_TX_PIN | K230_RX_PIN;
        g.Mode = GPIO_MODE_AF_PP;
        g.Pull = GPIO_PULLUP;
        g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        g.Alternate = GPIO_AF7_USART3;
        HAL_GPIO_Init(K230_TX_PORT, &g);
    }
    else if (huart->Instance == UART7)
    {
        __HAL_RCC_UART7_CLK_ENABLE();
        GPIO_ESP32_CLK_ENABLE;
        g.Pin = ESP32_TX_PIN | ESP32_RX_PIN;
        g.Mode = GPIO_MODE_AF_PP;
        g.Pull = GPIO_PULLUP;
        g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        g.Alternate = GPIO_AF7_UART7;
        HAL_GPIO_Init(ESP32_TX_PORT, &g);
    }
}

static void UART_ConfigBase(UART_HandleTypeDef *h, USART_TypeDef *inst, uint32_t baud)
{
    h->Instance = inst;
    h->Init.BaudRate = baud;
    h->Init.WordLength = UART_WORDLENGTH_8B;
    h->Init.StopBits = UART_STOPBITS_1;
    h->Init.Parity = UART_PARITY_NONE;
    h->Init.Mode = UART_MODE_TX_RX;
    h->Init.HwFlowCtl = UART_HWCONTROL_NONE;
    h->Init.OverSampling = UART_OVERSAMPLING_16;
    h->Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    h->Init.ClockPrescaler = UART_PRESCALER_DIV1;
    h->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    HAL_UART_Init(h);
    HAL_UARTEx_SetTxFifoThreshold(h, UART_TXFIFO_THRESHOLD_1_8);
    HAL_UARTEx_SetRxFifoThreshold(h, UART_RXFIFO_THRESHOLD_1_8);
    HAL_UARTEx_DisableFifoMode(h);
}

void USART_All_Init(void)
{
    UART_ConfigBase(&huart1, USART1, 115200);
    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);

    UART_ConfigBase(&huart2_motor1, USART2, 115200);
    UART_ConfigBase(&huart4_motor2, UART4, 115200);
    UART_ConfigBase(&huart3_k230, USART3, 115200);
    UART_ConfigBase(&huart7_esp32, UART7, 115200);
}

void USART1_SendByte(uint8_t ch)
{
    HAL_UART_Transmit(&huart1, &ch, 1, 10);
}

void USART1_SendStr(char *str)
{
    while (*str) USART1_SendByte(*str++);
}

int fputc(int ch, FILE *f)
{
    USART1_SendByte((uint8_t)ch);
    return ch;
}
