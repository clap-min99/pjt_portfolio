/*
 * 06.MY_WASHING_MACHINE.c
 *
 * Created: 2026-06-18 오후 3:46:54
 * Author : kccistc
 */ 

#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>   // sei, ISR 등
#include <stdio.h>

#include "button.h"

extern void init_button(void);
extern void init_led();
extern int get_button(int button_num, int button_pin);
extern int init_fnd();
extern void init_uart0();
extern void UART0_transmit(uint8_t data);
extern void pc_command_processing();
extern void init_ultrasonic(void);
extern void make_trigger(void);
extern void ultrasonic_processing(void);
extern void init_timer3_pwm(void);
extern void init_motor_driver(void);
extern void dcmotor_pwm_control_main(void);
extern void init_washer(void);
extern void washer_run(void);
extern void init_timer2(void);

extern volatile char scm[50];

FILE OUTPUT = FDEV_SETUP_STREAM(UART0_transmit, NULL, _FDEV_SETUP_WRITE);

void init_timer0();
volatile uint32_t msec_count = 0; // volatile 의 최적화 방지
volatile int ultrasonic_check_time = 0;

ISR(TIMER0_OVF_vect)
{
	TCNT0 = 6;		// TCNT0 6~256: 250 펄스 count 하기 위해
	msec_count++;
	ultrasonic_check_time++;
}


int main(void)
{
    init_led();
    init_timer0();
    init_uart0();
    init_button();
    init_washer();
	init_fnd();
	init_timer2();
    init_timer3_pwm();
    init_motor_driver();
	
	stdout = &OUTPUT;		// printf가 동작할 수 있도록 stdout을 설정
	sei();					// 전역(대문) interrupt 허용
	
	
	// dcmotor_pwm_control_main();
	
    while (1) 
    {
		washer_run();	
		 // pc_command_processing();
		 // ultrasonic_processing();
    }
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

