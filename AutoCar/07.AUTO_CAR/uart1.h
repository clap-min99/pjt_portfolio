/*
 * uart1.h
 *
 * Created: 2026-06-22 오전 10:38:22
 *  Author: kccistc
 */ 


#ifndef UART1_H_
#define UART1_H_

#define F_CPU 16000000UL  // 16MHz
#include <avr/io.h>  // PORTA PORTB PORTD... IO관련 reg가 들어 있다.
#include <util/delay.h>  // _delay_ms _delay_us 등
#include <avr/interrupt.h>
#include <string.h>

#define QUEUE_SIZE 10
#define QUEUE_LENGTH 80
//volatile int rear;  // interrupt에서 queue에 data를 저장하는 위치값
//volatile int front;  // pc_command_processing에서 가져가는 위치값



#endif /* UART1_H_ */