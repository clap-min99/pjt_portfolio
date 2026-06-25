/*
 * button.h
 *
 * Created: 2026-06-12 오후 1:23:02
 *  Author: kccistc
 */ 

// #ifndef BUTTON_H_		// BUTTON_H_라는 이름이 정의가 되어있지 않다면, 정의를 해라
// #define BUTTON_H_		// 그래서 정의한 것
// #endif /* BUTTON_H_ */

#define F_CPU 16000000UL	// 16MHz
#include <avr/io.h>			// PORTA, PORTB 등 I/O 관련 reg
#include <util/delay.h>		

// 아래는 pull down 저항(버튼) 기준으로 작성

#define BUTTON_DDR DDRD		
#define BUTTON_PIN PIND		// PORTD를 읽는 register 5V:1 0V:0

#define BUTTON0PIN 3		// PORTD.3 
#define BUTTON1PIN 4		// PORTD.4
#define BUTTON2PIN 5		// PORTD.5
#define BUTTON3PIN 6		// PORTD.6

#define BUTTON0 0			// PORTD.3의 가상 index (software number)
#define BUTTON1 1			// PORTD.4의 가상 index (software number)
#define BUTTON2 2			// PORTD.5의 가상 index (software number)
#define BUTTON3 3			// PORTD.6의 가상 index (software number)

#define BUTTON_NUMBER 4		// 버튼의 갯수

#define BUTTON_PRESS 1		// 버튼을 누르면 high(active-high)
#define BUTTON_RELEASE 0	// 버튼을 땐 상태(low)

// power ON & reset , 초기화(버튼 초기화 LED 초기화 DDR 초기화)
// Finite State Machine(FSM, 유한한 상태 내에서 작동) ≒ State transition Diagram(상태전이도)
// FSM을 잘 그리는게 중요하다 1. 회로도 작성 2. 펌웨어 작성(입.출력 정의) 3. 기본설계.상세설계  4. 코딩  5. 검증
