/*
 * led.c
 *
 * Created: 2026-06-15 오후 1:17:24
 *  Author: kccistc
 */ 


#include "led.h"

void init_led(void);


extern volatile uint32_t msec_count;

void init_led(void)
{
	DDRA=0xff;  // PORTA 를 출력 모드로 설정
	PORTA=0x00;  // PORTA에 물려있는 led를 all off
}


int func_state = 0;
