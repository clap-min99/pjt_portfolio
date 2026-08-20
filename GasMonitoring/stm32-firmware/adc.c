#include "device_driver.h"
#include "adc.h"

void ADC1_Init(void)
{
	// GPIOA 클럭 활성화 (AHB1ENR Bit 0)
	Macro_Set_Bit(RCC->AHB1ENR, 0U);

	// PA6 핀 아날로그 모드(11b) 설정 (MODER [13:12])
	Macro_Write_Block(GPIOA->MODER, 0x3, 0x3, 12U);

	// ADC1 클럭 활성화 (APB2ENR Bit 8)
	Macro_Set_Bit(RCC->APB2ENR, 8U);

	// CH6 샘플링 타임 설정: 480 Cycles (SMPR2 [20:18])
	Macro_Write_Block(ADC1->SMPR2, 0x7, 0x7, 18U);

	// 정규 시퀀스 변환 길이: 1개 (SQR1 [23:20] = 0x0)
	Macro_Write_Block(ADC1->SQR1, 0xF, 0x0, 20U);

	// 1번 변환 채널로 CH6 지정 (SQR3 [4:0] = 6)
	Macro_Write_Block(ADC1->SQR3, 0x1F, 6U, 0U);

	// ADC 클럭 프리스케일러: PCLK2 / 6 = 16MHz (ADC_CCR [17:16] = 0x2)
	Macro_Write_Block(ADC->CCR, 0x3, 0x2, 16U);

	// ADC1 활성화 (CR2 Bit 0: ADON)
	Macro_Set_Bit(ADC1->CR2, 0U);
}

unsigned short ADC1_Read(void)
{
	// 소프트웨어 변환 시작 (CR2 Bit 30: SWSTART)
	Macro_Set_Bit(ADC1->CR2, 30U);

	// 변환 완료(EOC) 대기
	while (!Macro_Check_Bit_Set(ADC1->SR, 1U))
		;

	// EOC 플래그 클리어
	Macro_Clear_Bit(ADC1->SR, 1U);

	// 12비트 변환 데이터 반환 (0 ~ 4095)
	return (unsigned short)(ADC1->DR & 0xFFFU);
}