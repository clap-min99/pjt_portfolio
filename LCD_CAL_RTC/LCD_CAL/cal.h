/*
 * cal.h
 *
 * Created: 2026-06-29 오후 4:08:43
 *  Author: kccistc
 */ 


#ifndef CAL_H_
#define CAL_H_

#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 32
#define STACK_SIZE 16


typedef struct _cal
{
	char    buf[BUF_SIZE];   // 입력 버퍼 ex) "10+20*3"
	uint8_t idx;
} t_cal;


#endif /* CAL_H_ */