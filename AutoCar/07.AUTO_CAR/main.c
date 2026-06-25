/*
 * 07.AUTO_CAR.c
 *
 * Created: 2026-06-22 오전 10:33:40
 * Author : kccistc
 */ 

#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>   // sei, ISR 등
#include <stdio.h>

#include "def.h"

extern void UART0_transmit(uint8_t data);
extern int get_button(int button_num, int button_pin);
extern void init_uart0();
extern void init_uart1();
extern void init_ultrasonic(void);
extern void init_timer1_pwm(void);
extern void init_motor_driver(void);
extern void pc_command_processing();
extern void make_trigger(void);

extern void forward(int speed);
extern void backward(int speed);
extern void turn_left(int speed);
extern void turn_right(int speed);
extern void stop();

//======== 자율주행 =========
extern void init_auto_car(void);
extern void init_fnd1(void);
extern void init_fnd2(void);
extern void auto_drive(void);
extern int get_auto_button();
extern void set_auto_led(int state);
extern void fnd1_display(uint32_t ms);
extern void fnd1_scan(uint32_t elapsed_ms);
extern void fnd2_scan(uint8_t dir, uint8_t speed_pct);
extern void fnd2_result_scan(void);
extern void ultrasonic_processing();
extern void reset_drive_stats(void);


volatile uint8_t current_dir = 'S';   
volatile uint8_t current_speed = 0;  
volatile uint32_t msec_count = 0; // volatile 의 최적화 방지
volatile int ultrasonic_check_time = 0;
volatile uint32_t auto_start_ms = 0;

extern volatile int dist_left;
extern volatile int dist_front;
extern volatile int dist_right;
extern volatile uint32_t total_drive_ms;
extern volatile uint8_t bt_data;

void init_timer0();
void manual_mode(void);
void auto_mode(void);
void auto_mode_check(void);
void distance_check(void);

// extern volatile char scm[50];
FILE OUTPUT = FDEV_SETUP_STREAM(UART0_transmit, NULL, _FDEV_SETUP_WRITE);


int func_state = MANUAL_MODE;

void (*pfunc[]) () =
{
	manual_mode,
	auto_mode
};

void manual_mode(void)
{
	switch(bt_data)
	{
		case 'F': case 'f':
		forward(500);
		current_dir = 'F';
		current_speed = 48;  // 48%
		break;
		case 'B': case 'b':
		backward(500);
		current_dir = 'b';
		current_speed = 48;
		break;
		case 'L': case 'l':
		turn_left(700);
		current_dir = 'r';
		current_speed = 68;  // 68%
		break;
		case 'R': case 'r':
		turn_right(700);
		current_dir = 'L'; 
		current_speed = 68;
		break;
		case 'S': case 's':
		stop();
		current_dir = 'S';
		current_speed = 0;
		break;
		default:
		break;
	}
	bt_data = 0;  // 중복 실행 방지
}

void auto_mode(void)
{
	auto_drive();
}


ISR(TIMER0_OVF_vect)
{
	TCNT0 = 6;		// TCNT0 6~256: 250 펄스 count 하기 위해
	msec_count++;
	ultrasonic_check_time++;
}


int main(void)
{
	init_timer0();
	init_uart0();		// COMport Master로 수동조작
	init_uart1();		// 블루투스 사용 불가
	init_motor_driver();
	init_timer1_pwm();
	init_auto_car();
	init_ultrasonic();
	init_fnd1();
	init_fnd2();
	
	
	stdout = &OUTPUT;		// printf가 동작할 수 있도록 stdout을 설정
	sei();					// 전역(대문) interrupt 허용
	

	while (1)
	{
		if (get_auto_button())
		{
			func_state = (func_state == MANUAL_MODE) ? AUTO_MODE : MANUAL_MODE;
			set_auto_led(func_state == AUTO_MODE);

			if (func_state == AUTO_MODE)
			{
				auto_start_ms = msec_count;
				reset_drive_stats();        // 통계 초기화
			}
			else
			{
				stop();
				current_dir = 'S';
				current_speed = 0;
				total_drive_ms = msec_count - auto_start_ms;  // 총 주행시간 확정
			}
		}
		
		
		if (func_state == AUTO_MODE)
		{
			fnd1_scan(msec_count - auto_start_ms);   // 주행 중: 경과시간
			fnd2_scan(current_dir, current_speed);
		}
		else
		{
			// MANUAL 모드
			if (current_dir == 'S')
			{
				fnd1_scan(total_drive_ms);   // 정지: 직전 주행시간
				fnd2_result_scan();          // 결과 순환 표시
			}
			else
			{
				fnd1_scan(0);                // 조작 중: 00.00
				fnd2_scan(current_dir, current_speed);  // 방향+속도
			}
		}
		ultrasonic_processing();
		pc_command_processing();
		pfunc[func_state]();
	
	}
	
	return 0;
}

void init_timer0()
{
	// TCNT0 = TimerCount0
	// TCNT0 6 ~ 256 : 250개 펄스를 count 하기 위해 6부터 시작
	TCNT0 = 6;
	
	TCCR0 &= ~( 1<< CS02 | 1 << CS01 | 1 << CS00);	// 0분주 reset 먼저
	TCCR0 |= 1 << CS02 | 0 << CS01 | 0 << CS00;		// 64분주
	TIMSK |= 1 << TOIE0;	// TIMER0 Overflow INT

}

