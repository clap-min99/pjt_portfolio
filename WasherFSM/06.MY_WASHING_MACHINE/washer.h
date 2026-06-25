/*
 * washer.h
 *
 * Created: 2026-06-18 오후 3:59:06
 *  Author: kccistc
 */ 


#ifndef WASHER_H_
#define WASHER_H_

#include <stdint.h>

typedef enum
{
	WASHER_IDLE,

	SET_WASH_TIME,
	SET_RINSE_TIME,
	SET_SPIN_TIME,

	STATE_WASH,
	STATE_RINSE,
	STATE_SPIN,
	
	STATE_PAUSE
	
} WasherState;


#endif