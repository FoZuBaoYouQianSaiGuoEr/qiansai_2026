#include "main.h"
#include "stm32h7xx_it.h"

extern DCMI_HandleTypeDef hdcmi;
extern DMA_HandleTypeDef DMA_Handle_dcmi;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2_motor1;
extern UART_HandleTypeDef huart3_k230;
extern UART_HandleTypeDef huart7_esp32;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim6;

void NMI_Handler(void) { while (1) {} }
void HardFault_Handler(void) { while (1) {} }
void MemManage_Handler(void) { while (1) {} }
void BusFault_Handler(void) { while (1) {} }
void UsageFault_Handler(void) { while (1) {} }
void SVC_Handler(void) {}
void DebugMon_Handler(void) {}
void PendSV_Handler(void) {}

void SysTick_Handler(void)
{
    HAL_IncTick();
}

void DMA2_Stream7_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&DMA_Handle_dcmi);
}

void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart1);
}

void DCMI_IRQHandler(void)
{
    HAL_DCMI_IRQHandler(&hdcmi);
}

void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart2_motor1);
}

void USART3_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart3_k230);
}

void UART7_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart7_esp32);
}

void TIM3_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim3);
}

void TIM6_DAC_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim6);
}
