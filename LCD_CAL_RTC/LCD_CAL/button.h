/*
 * button.h
 *
 * Created: 2026-06-15 오후 4:02:35
 *  Author: kccistc
 */ 

// #ifndef BUTTON_H_		// BUTTON_H_라는 이름이 정의가 되어있지 않다면, 정의를 해라
// #define BUTTON_H_		// 그래서 정의한 것
// #endif /* BUTTON_H_ */

#define F_CPU 16000000UL	// 16MHz
#include <avr/io.h>			// PORTA, PORTB 등 I/O 관련 reg
// #include <util/delay.h>		

// 아래는 pull down 저항(버튼) 기준으로 작성

#define BUTTON_DDR DDRD		
#define BUTTON_PIN PIND		// PORTD를 읽는 register 5V:1 0V:0

#define BUTTON0PIN 3		// PORTD.3 
#define BUTTON1PIN 4		// PORTD.4
#define BUTTON2PIN 5		// PORTD.5
#define BUTTON3PIN 6		// PORTD.6

#define BUTTON4PIN 7		// PORTD.7

#define BUTTON0 0			// PORTD.3의 가상 index (software number)
#define BUTTON1 1			// PORTD.4의 가상 index (software number)
#define BUTTON2 2			// PORTD.5의 가상 index (software number)
#define BUTTON3 3			// PORTD.6의 가상 index (software number)

#define BUTTON4 4			// PORTD.2의 가상 index (software number) - CAL/CLOCK 모드 전환

#define BUTTON_NUMBER 5		// 버튼의 갯수

#define BUTTON_PRESS 1		// 버튼을 누르면 high(active-high)
#define BUTTON_RELEASE 0	// 버튼을 땐 상태(low)

