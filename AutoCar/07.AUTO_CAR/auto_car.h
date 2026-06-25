/*
 * auto_car.h
 *
 * Created: 2026-06-22
 *  Author: kccistc
 */

#ifndef AUTO_CAR_H_
#define AUTO_CAR_H_

#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

// ========== 모드 정의 ==========
#define MANUAL_MODE  0
#define AUTO_MODE    1

// ========== 버튼 (PG0) ==========
#define BTN_DDR   DDRG
#define BTN_PIN   PING
#define BUTTON0PIN   0    // PG0 자율주행 버튼
#define BUTTON_NUMBER 1
#define BUTTON_PRESS   1
#define BUTTON_RELEASE 0
#define BTN_BIT   0

// ========== 자율주행 LED (PG4) ==========
#define AUTO_LED_DDR   DDRG
#define AUTO_LED_PORT  PORTG
#define AUTO_LED_BIT   4

// ========== FND1 (스톱워치) ==========
// 세그먼트: PORTD (A~DP)
// 자릿수: PF7=D1(천), PF6=D2(백), PF5=D3(십), PF4=D4(일)
#define FND1_DATA_DDR    DDRD
#define FND1_DATA_PORT   PORTD
#define FND1_DIGIT_DDR   DDRF
#define FND1_DIGIT_PORT  PORTF
#define FND1_D1  7   // PF7
#define FND1_D2  6   // PF6
#define FND1_D3  5   // PF5
#define FND1_D4  4   // PF4

// ========== 초음파 센서 ==========
// 트리거
#define TRIG_DDR   DDRA
#define TRIG_PORT  PORTA
#define TRIG_LEFT  0   // PA0
#define TRIG_FRONT 1   // PA1
#define TRIG_RIGHT 2   // PA2

// ========== FND2 (방향+속도) ==========
#define FND2_DATA_DDR    DDRC
#define FND2_DATA_PORT   PORTC
#define FND2_DIGIT_DDR   DDRA
#define FND2_DIGIT_PORT  PORTA
#define FND2_D1  4   // PA4
#define FND2_D2  5   // PA5
#define FND2_D3  6   // PA6
#define FND2_D4  7   // PA7

// 에코
#define ECHO_DDR   DDRE
#define ECHO_PORT  PINE
#define ECHO_LEFT  4   // PE4 INT4
#define ECHO_FRONT 5   // PE5 INT5
#define ECHO_RIGHT 6   // PE6 INT6

// ========== 자율주행 파라미터 ==========

#define SPD_MIN      250
#define SPD_SLOW     350
#define SPD_MED      500
#define SPD_FAST     700
#define SPD_MAX      900

#define SPD_FULL     SPD_FAST
#define SPD_CRUISE   SPD_MED
#define SPD_PIVOT    SPD_FAST

#endif /* AUTO_CAR_H_ */