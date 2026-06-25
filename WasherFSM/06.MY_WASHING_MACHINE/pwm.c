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

void init_timer3_pwm(void);
void init_motor_driver(void);
void dcmotor_pwm_control_main(void);

// 16bit 3번 timer/counter를 사용
/*
	PWM 출력 신호
	============
	PE3: OC3A
	PE4: OC3B : INT4 사용(초음파 센서)
	PE5: OC3C ==> DC MOTOR (PWM 출력)
	BTN1: speed up
	BTN2: speed down
*/
void init_timer3_pwm(void) 
{
	// PE4 초음파 연결
	DDRE |= 1 << 3 | 1 << 5;
	
	// 모드 5 : 8bit 고속 PWM 모드 사용 timer3(p348 표 15-6)
	TCCR3A |= 1 << WGM30;
	TCCR3B |= 1 << WGM32;

	// 비반전 모드 top: 0x00ff 비교일치 값 (PWM) 지정 OCR3C p350 표 15-7
	TCCR3A |= 1 << COM3C1;
	
	// ------------ 분주비 설정 -------------
	// 분주비 64
	// 16000000Hz / 64 ---> 250000Hz(250KHz)
	// T = 1/f = 1/250000Hz ---> 0.000004sec ---> 4µs
	// 250000Hz에서 256개 펄스를 count 하면 소요시간 1.02ms
	//				127개 펄스					   0.5ms
	// TCNT3: 0~255(0x00ff) 까지 count한 후 0으로 다시 돌아간다.
	TCCR3B |= 1 << CS31 | 1 << CS30;		// 분주비 64
	OCR3C = 0;							// OCR(output compare register): pwm 값
	// OCR3C: 50인 경우 duty(high)가 몇 %인가?
	// Duty Cycle  = (OCR3C / TOP) x 100 = 50 / 255 x 100 = 19.61%
	
}

/*
PE5: PWM control 
PF6: IN1 방향 설정
PF7: IN2
*/

void init_motor_driver(void) {
	// ENA (PE5) 출력모드
	DDRE |= (1 << 5);
	// IN2(PF7), IN3(PF6) 출력모드
	DDRF |= (1 << 6) | (1 << 5);		// 출력모드로 설정
	PORTF &= ~(1<< 6 |  1<< 5);			// 6 7 reset	
	// PORTF |= 1 << 6;					// 정회전
	
	
	//// Timer3 Fast PWM (OC3C) 설정
	//TCCR3A |= (1 << COM3C1) | (1 << WGM31);
	//TCCR3B |= (1 << WGM33) | (1 << WGM32) | (1 << CS31);  // 8분주
	//ICR3 = 255;
	//OCR3C = 0;
}

void dcmotor_pwm_control_main(void)
{
	int static start_button = 0;  
	int static forward = 0;
	
	if (get_button(BUTTON0, BUTTON0PIN))	// 누르면 세탁 -> 헹굼 -> 탈수
	{
		start_button = !start_button;
		if (start_button) OCR3C = 250;
		else OCR3C = 0;
	}
	else if (get_button(BUTTON1, BUTTON1PIN))  // speed-up
	{
		if (OCR3C >= 250) OCR3C = 250;
		else OCR3C += 20;
	}
	else if (get_button(BUTTON2, BUTTON2PIN))  // speed-down
	{
		if (OCR3C <= 70) OCR3C = 70;
		else OCR3C -= 20;
	}
	else if (get_button(BUTTON3, BUTTON3PIN))  // 방향설정
	{
		forward = !forward;
		PORTF &= ~(1 << 6 | 1 << 7);
		if (forward) PORTF |= 1 << 6;
		else PORTF |= 1 << 7;
	}
	
	//while (1)
	//{
		//if (get_button(BUTTON0, BUTTON0PIN))	// start stop
		//{
			//start_button = !start_button;
			//if (start_button) OCR3C = 250;	
			//else OCR3C = 0;
		//}
		//else if (get_button(BUTTON1, BUTTON1PIN))	// speed-up
		//{
			//if (OCR3C >= 250) OCR3C = 250;
			//else OCR3C += 20;
		//}
		//else if (get_button(BUTTON1, BUTTON1PIN))	// speed-down
		//{
			//if (OCR3C <= 70) OCR3C = 70;
			//else OCR3C -= 20;
		//}
		//else if (get_button(BUTTON2, BUTTON2PIN))	// 방향설정
		//{
			//forward = !forward;
			//PORTF &= ~( 1 << 6 | 1 << 7 );
			//if (forward) PORTF |= 1 << 6;			// 정회전
			//else PORTF |= 1 << 7;					// 역회전
		//}
	//}
}