#ifndef __TIMER_H__
#define __TIMER_H__

#define TIM_TICK (20U)                  // 타이머 틱 단위 (us)
#define TIM_FREQ (1000000.0 / TIM_TICK) // 카운트 주파수 (Hz)
#define TIM_1MS_PLS (TIM_FREQ / 1000.0) // 1ms당 카운트 펄스 수

void Timer_Init(void);
void TIM4_Init(void);                             // 1ms 시스템 틱 생성 타이머
void TIM2_PWM_Init(void);                         // 서보 모터용 PWM 타이머 (50Hz / 20ms)
void TIM2_PWM_Set_Pulse(unsigned short pulse_us); // PWM 펄스 폭(us) 제어

#endif // __TIMER_H__