/*
 * lcd.h
 *
 * Created: 2026-07-01 오전 10:30:04
 *  Author: kccistc
 */ 


#ifndef LCD_H_
#define LCD_H_
#define F_CPU 16000000UL 
#include <avr/io.h> 
#include <util/delay.h>  
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>


#define PORT_DATA PORTC
#define PORT_CONTROL PORTB
#define DDR_DATA DDRC
#define DDR_CONTROL DDRB

#define RS_PIN	5
#define RW_PIN	6
#define E_PIN	7

#define COMMAND_CLEAR_DISPLAY 0x01
#define COMMAND_8_BIT_MODE 0x38
#define COMMAND_4_BIT_MODE 0x28 

#define COMMAND_DISPLAY_ON_OFF_BIT	2
#define COMMAND_CURSOR_ON_OFF_BIT	1
#define COMMAND_BLINK_ON_OFF_BIT	0

extern uint8_t MODE;

#endif /* LCD_H_ */