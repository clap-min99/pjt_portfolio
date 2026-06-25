/*
 * pwm.c
 *
 * Created: 2026-06-18 오후 2:22:50
 *  Author: kccistc
 */ 
#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>   
#include <stdio.h>
#include "pwm.h"
extern void init_button(void);
extern int get_button(int button_num, int button_pin);

int motor_state = 0;   // 0: stop, 1: run
int motor_dir = 1;     // 1: 정회전, -1: 역회전
int motor_speed = 0;   // OCR3C 값

void init_timer1_pwm(void);
void init_motor_driver(void);
void forward(int speed);
void backward(int speed);
void turn_left(int speed);
void turn_right(int speed);
void stop();

#define MOTOR_PWM_DDR DDRB
#define MOTOR_LEFT_PORT_DDR 5	// OC1A
#define MOTOR_RIGHT_PORT_DDR 6	// OC1B

#define MOTOR_DIRECTION_PORT PORTF
#define MOTOR_DIRECTION_PORT_DDR DDRF

// 16bit 1번 timer/counter를 사용
/*
	PWM 출력 신호
	============
	PB5: OC1A -> 왼쪽 바퀴
	PB6: OC1B -> 오른쪽 바퀴
	BTN0: auto/manual mode
	
	**방향설정**
	============
	1. 왼쪽 바퀴
		PORTF0 -- IN1(DC motor driver)
		PORTF1 -- IN2
	2. 오른쪽 바퀴
		PORTF2 -- IN3(DC motor driver)
		PORTF3 -- IN4
	
	IN1/IN3			IN2/IN4
	=======			=======
	   0				1	: 역회전
	   1				0	: 정회전
	   1				1	: stop
	   
*/


void init_timer1_pwm(void) 
{
	// ------------ 분주비 설정 -------------
	// 분주비 64
	// 16000000Hz / 64 ---> 250000Hz(250KHz)
	// T = 1/f = 1/250000Hz ---> 0.000004sec ---> 4µs
	// 250000Hz에서 256개 펄스를 count 하면 소요시간 1.02ms
	//				127개 펄스					   0.5ms
	//				0x3ff(1023) -----> 4ms
	// TCNT3: 0~255(0x00ff) 까지 count한 후 0으로 다시 돌아간다.
	TCCR1B |= 1 << CS11 | 1 << CS10;	// 분주비 64
	// OCR3C = 0;							// OCR(output compare register): pwm 값
	// OCR3C: 50인 경우 duty(high)가 몇 %인가?
	// Duty Cycle  = (OCR3C / TOP) x 100 = 50 / 255 x 100 = 19.61%
	
	// 모드 14: 고속 PWM 모드 사용하겠다. timer1 (p327 표 14-5)
	TCCR1A |= 1 << WGM11;	// TOP --> ICR1으로 설정
	TCCR1B |= 1 << WGM13 | 1 << WGM12;
	// 비반전모드 top: ICR1 비교일치값 (PWM) 지정 OCR1A OCR1B p.350 표 15-7
	// 비교 일치 발생시 OCR1A, OCR1B의 출력핀은 LOW로 바뀌고 BOTTOM에서 HIGH로 바뀐다.(비반전 모드)
	TCCR1A |= 1 << COM1A1;
	TCCR1A |= 1 << COM1B1;
	
	ICR1 = 0x3ff;	// 1023 * 4µs ==> 4ms TOP 값

}

/*
PE5: PWM control 
PF6: IN1 방향 설정
PF7: IN2
*/

void init_motor_driver(void) {
	
	// 0. 초기화
	MOTOR_PWM_DDR &= ~(1 << 5 | 1 << 6);
	MOTOR_DIRECTION_PORT_DDR &= ~(1 << 0 | 1 << 1 | 1 << 2 | 1 << 3);
	
	// 1. 출력모드로 설정
	MOTOR_PWM_DDR |= (1 << 5) | (1 << 6);
	MOTOR_DIRECTION_PORT_DDR |= 1 << 0 | 1 << 1 | 1 << 2 | 1 << 3;	// 출력모드
	
	// 2. 모터를 전진 모드로 
	MOTOR_DIRECTION_PORT &= ~(1 << 0 | 1 << 1 | 1 << 2 | 1 << 3);	// 0으로 초기화 후 시작
	MOTOR_DIRECTION_PORT |=  1 << 2 | 1 << 0;						// 모터 전진 mode IN4IN3IN2IN1
																	//				  0  1  0  1
																	

	//// Timer3 Fast PWM (OC3C) 설정
	//TCCR3A |= (1 << COM3C1) | (1 << WGM31);
	//TCCR3B |= (1 << WGM33) | (1 << WGM32) | (1 << CS31);  // 8분주
	//ICR3 = 255;
	//OCR3C = 0;
}

void forward(int speed)
{
	// 모터를 전진 모드로
	MOTOR_DIRECTION_PORT &= ~(1 << 0 | 1 << 1 | 1 << 2 | 1 << 3);	// 0으로 초기화 후 시작
	MOTOR_DIRECTION_PORT |=  1 << 2 | 1 << 0;
	
	OCR1A = OCR1B = speed;	// OCR1A : pwm left / OCR1B: pwm right 

}

void backward(int speed)
{
	MOTOR_DIRECTION_PORT &= ~(1 << 0 | 1 << 1 | 1 << 2 | 1 << 3);
	MOTOR_DIRECTION_PORT |= 1 << 1 | 1 << 3;	// 모터를 후진 모드로 IN4IN3IN2IN1
												//					  1  0  1  0
	// PF0=0, PF1=1, PF2=0, PF3=1
	// IN1=0, IN2=1 → 왼쪽 역회전
	// IN3=0, IN4=1 → 오른쪽 역회전
	OCR1A = OCR1B = speed;
}
void turn_left(int speed)
{
	MOTOR_DIRECTION_PORT &= ~(1 << 0 | 1 << 1 | 1 << 2 | 1 << 3);
	MOTOR_DIRECTION_PORT |= 1 << 0 | 1 << 3;
	// PF0=0, PF1=1 → 왼쪽 역회전
	// PF2=1, PF3=0 → 오른쪽 정회전

	OCR1A = 0;
	OCR1B = speed;
}
void turn_right(int speed)
{
	 MOTOR_DIRECTION_PORT &= ~(1 << 0 | 1 << 1 | 1 << 2 | 1 << 3);
	 MOTOR_DIRECTION_PORT |= 1 << 1 | 1 << 2;
	 // PF0=1, PF1=0 → 왼쪽 정회전
	 // PF2=0, PF3=1 → 오른쪽 역회전

	 OCR1A = speed;
	 OCR1B = 0;
}

void stop(void)
{
	MOTOR_DIRECTION_PORT &= ~(1 << 0 | 1 << 1 | 1 << 2 | 1 << 3);
	MOTOR_DIRECTION_PORT |= 1 << 0 | 1 << 1 | 1 << 2 | 1 << 3;
	// PF0=1, PF1=0 → 왼쪽 정회전
	// PF2=0, PF3=1 → 오른쪽 역회전

	OCR1A = 0;
	OCR1B = 0;
}
