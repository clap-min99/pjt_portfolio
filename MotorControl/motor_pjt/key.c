#include "device_driver.h"

void Key_Poll_Init(void)
{
	Macro_Set_Bit(RCC->AHB1ENR, 2); 
	Macro_Write_Block(GPIOC->MODER, 0x3, 0x0, 26);
}

void Key_ISR_EN()
{
	
	Macro_Set_Bit(RCC->AHB1ENR, 2); 
	Macro_Write_Block(GPIOC->MODER, 0x3, 0x0, 26);

	Macro_Set_Bit(RCC->APB2ENR, 14); 
	Macro_Write_Block(SYSCFG->EXTICR[3], 0xf, 0x2, 4);

	Macro_Set_Bit(EXTI->FTSR, 13);
	Macro_Set_Bit(EXTI->RTSR, 13);
	EXTI->PR = 0x1 << 13;
	
	NVIC_ClearPendingIRQ((IRQn_Type)40);
	Macro_Set_Bit(EXTI->IMR, 13);
	NVIC_EnableIRQ((IRQn_Type)40);

}
