/*
 * fnd.h
 *
 * Created: 2026-06-15 오후 4:02:10
 *  Author: kccistc
 */ 


#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

#define FND_DATA_DDR DDRC
#define FND_DATA_PORT PORTC

#define FND_DIGIT_DDR DDRB // Data Direction Register B 포트 B 핀의 입출력 방향 설정
#define FND_DIGIT_PORT PORTB  // 자릿수
#define FND_DIGIT_D1 4
#define FND_DIGIT_D2 5
#define FND_DIGIT_D3 6
#define FND_DIGIT_D4 7
