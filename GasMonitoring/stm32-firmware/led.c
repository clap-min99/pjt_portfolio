#include "led.h"
#include "macro.h"

void LED_Init(void)
{
	// GPIOB 클럭 활성화 (AHB1ENR Bit 1)
	Macro_Set_Bit(RCC->AHB1ENR, 1);

	// PB0, PB1 범용 출력 모드(01b) 설정
	Macro_Write_Block(GPIOB->MODER, 0x3, 0x1, 0); // PB0 [1:0]
	Macro_Write_Block(GPIOB->MODER, 0x3, 0x1, 2); // PB1 [3:2]

	// 출력 형태: Push-Pull (0)
	Macro_Clear_Bit(GPIOB->OTYPER, 0);
	Macro_Clear_Bit(GPIOB->OTYPER, 1);

	// 초기 상태: 소등 (0V 출력)
	Macro_Clear_Bit(GPIOB->ODR, 0);
	Macro_Clear_Bit(GPIOB->ODR, 1);
}

void LED_On(void)
{
	Macro_Set_Bit(GPIOB->ODR, 0);
	Macro_Set_Bit(GPIOB->ODR, 1);
}

void LED_Off(void)
{
	Macro_Clear_Bit(GPIOB->ODR, 0);
	Macro_Clear_Bit(GPIOB->ODR, 1);
}

void LED_Display(int on)
{
	// PB0, PB1 비트 블록 동시 제어 (1: 11b 점등, 0: 00b 소등)
	Macro_Write_Block(GPIOB->ODR, 0x3, on ? 0x3 : 0x0, 0);
}

void LED_Top(int on)
{
	Macro_Write_Block(GPIOB->ODR, 0x1, on & 0x1, 0);
}

void LED_Bottom(int on)
{
	Macro_Write_Block(GPIOB->ODR, 0x1, on & 0x1, 1);
}