#include "stm32f4xx.h"
#include "option.h"
#include "macro.h"
#include "malloc.h"

// 변수

extern volatile unsigned int motor_dir;
extern volatile int TIM2_Expired;
extern volatile int TIM4_Expired;
extern volatile int Uart_Data_In;
extern volatile unsigned char Uart_Data;
extern volatile unsigned int Key_Pressed;
extern volatile unsigned int time_cnt;
extern volatile int motor_state;
extern volatile int gear;


// Uart.c

extern void Uart2_Init(int baud);
extern void Uart2_Send_Byte(char data);
extern char Uart2_Get_Char(void);
extern char Uart2_Get_Pressed(void);
extern void Uart2_RX_Interrupt_Enable(int en);


// SysTick.c

extern void SysTick_Run(void);

// Led.c

extern void LED_Init(void);
extern void LED_On(void);
extern void LED_Off(void);

// Clock.c

extern void Clock_Init(void);

// key.c

extern void Key_Poll_Init(void);
extern int Key_Get_Pressed(void);
extern void Key_ISR_EN(void);

// motor.c
extern void Motor_CW(int gear);
extern void Motor_CCW(int gear);
extern void Motor_Stop(void);

// timer.c
extern void TIM2_Delay(int time);
extern void TIM2_SW_Start(int time);
extern unsigned int TIM2_SW_Stop(void);
extern int TIM2_SW_T_CHK(void);
extern void TIM2_ISR_EN(void);

extern void TIM3_Delay(int time);

extern void TIM4_SW_Start(int time);
extern int TIM4_SW_T_CHK(void);
extern void TIM4_SW_Stop(void);

extern void TIM5_Init(void);
extern void TIM5_CW_PWM(unsigned short freq, int duty);
extern void TIM5_CCW_PWM(unsigned short freq, int duty);


// exception.c
extern void _Invalid_ISR(void);
extern void EXTI15_10_IRQHandler(void);
extern void TIM2_IRQHandler(void);
extern void TIM4_IRQHandler(void);
extern void USART2_IRQHandler(void);

//operation.c
extern void Op_Handler(void);
extern void Uart_Handler(void);
extern void Key_Handler(unsigned int time_cnt);