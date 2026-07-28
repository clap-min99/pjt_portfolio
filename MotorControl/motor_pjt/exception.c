#include "device_driver.h"
#include <stdio.h>

void _Invalid_ISR(void)
{
	unsigned int r = Macro_Extract_Area(SCB->ICSR, 0x1ff, 0);
	printf("\nInvalid_Exception: %d!\n", r);
	printf("Invalid_ISR: %d!\n", r - 16);
	for(;;);
}


void EXTI15_10_IRQHandler(void)
{
    if(!(GPIOC->IDR >> 13))
    {
        Key_Pressed = 1;
    }
    else
    {
        Key_Pressed = 2;
    }
    EXTI->PR = 0x1 << 13;
    NVIC_ClearPendingIRQ(40);
}


void TIM2_IRQHandler(void)
{
    time_cnt++;
    // TIM2 Interrupt Pending Clear
    Macro_Clear_Bit(TIM2->SR, 0);
    // NVIC Pending Clear
    //NVIC_ClearPendingIRQ(28);
}


void TIM4_IRQHandler(void)
{
    // TIM4 Interrupt Pending Clear
    Macro_Clear_Bit(TIM4->SR, 0);
    // NVIC Pending Clear
    NVIC_ClearPendingIRQ(30);
    TIM4_Expired = 1;

}

void USART2_IRQHandler(void)
{
	Uart_Data = (unsigned char)USART2->DR;
	Uart2_Send_Byte(Uart_Data);
	Uart_Data_In = 1;
	NVIC_ClearPendingIRQ(38);
}

