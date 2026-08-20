#include "device_driver.h"
#include "timer.h"
#include "macro.h"

volatile unsigned long g_sys_tick = 0;

void Timer_Init(void)
{
	TIM4_Init();
	TIM2_PWM_Init();
}

// 1ms 주기 시스템 틱 타이머 초기화 (TIM4)
void TIM4_Init(void)
{
	Macro_Set_Bit(RCC->APB1ENR, 2U);	 // TIM4 클럭 인에이블
	TIM4->CR1 = (1U << 4U) | (0U << 3U); // Down-counter, Repeat 모드
	TIM4->PSC = (unsigned int)(TIMXCLK / (double)TIM_FREQ + 0.5) - 1U;
	TIM4->ARR = (unsigned int)(TIM_1MS_PLS * 1) - 1U; // 1ms 주기 설정

	Macro_Set_Bit(TIM4->EGR, 0U);
	Macro_Clear_Bit(TIM4->SR, 0U);

	NVIC_ClearPendingIRQ(TIM4_IRQn);
	Macro_Set_Bit(TIM4->DIER, 0U); // 인터럽트 허용
	NVIC_EnableIRQ(TIM4_IRQn);

	Macro_Set_Bit(TIM4->CR1, 0U); // 타이머 시작
}

// 서보 모터 구동용 50Hz PWM 타이머 초기화 (TIM2 CH1)
void TIM2_PWM_Init(void)
{
	// TIM2 클럭 활성화 (APB1ENR Bit 0)
	Macro_Set_Bit(RCC->APB1ENR, 0U);

	// 타이머 주기 설정 (96MHz 기준 1us 틱, 20ms / 50Hz 주기)
	TIM2->PSC = (unsigned int)(TIMXCLK / 1000000.0 + 0.5) - 1U; // 1 tick = 1us
	TIM2->ARR = 20000U - 1U;									// 20000us = 20ms (50Hz)

	// TIM2 CH1 PWM Mode 1(110b) 및 Preload 활성화
	Macro_Write_Block(TIM2->CCMR1, 0x7, 0x6, 4); // OC1M [6:4] = 110b
	Macro_Set_Bit(TIM2->CCMR1, 3U);				 // OC1PE = 1

	// CH1 출력 활성화
	Macro_Set_Bit(TIM2->CCER, 0U); // CC1E = 1

	// 설정 레지스터 즉시 갱신 (EGR Update Generation)
	Macro_Set_Bit(TIM2->EGR, 0U);
	Macro_Clear_Bit(TIM2->SR, 0U);

	Macro_Set_Bit(TIM2->CR1, 7U); // ARPE = 1
	Macro_Set_Bit(TIM2->CR1, 0U); // CEN = 1 (타이머 시작)
}

// TIM2 CH1 펄스 폭(us) 설정
void TIM2_PWM_Set_Pulse(unsigned short pulse_us)
{
	TIM2->CCR1 = pulse_us;
}