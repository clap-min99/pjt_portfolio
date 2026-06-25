/*
 * washer.c
 *
 * Created: 2026-06-18 오후 3:58:53
 *  Author: kccistc
 */ 

#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>

#include "washer.h"
#include "button.h"
#include "pwm.h"
#include "fnd.h"
#include "led.h"

uint16_t wash_time  = 2;    // 세탁 시간
uint16_t rinse_time = 1;	// 헹굼 시간
uint16_t spin_time  = 1;	// 탈수 시간


uint16_t remain_time = 0;
uint16_t remain_msec = 0;
uint32_t prev_ms =0;

extern void init_button(void);
extern int get_button(int button_num, int button_pin);
extern void fnd_display_washer(uint16_t remain_time, uint16_t remain_msec);
extern void fnd_display_setting(uint16_t value);

extern volatile uint32_t msec_count;
static uint32_t motor_dir_time = 0;
static uint8_t motor_dir = 0;

static WasherState state = WASHER_IDLE;
static WasherState prev_state = WASHER_IDLE;

void init_washer(void);
void washer_run(void);
static void washer_timer_update(void);
static void washer_button_process(void);
static void washer_state_process(void);
static void washer_motor_control(void);



char *state_string(void)
{
	switch(state)
	{
		case WASHER_IDLE:
		return "IDLE";

		case SET_WASH_TIME:
		return "SET_WASH";

		case SET_RINSE_TIME:
		return "SET_RINSE";

		case SET_SPIN_TIME:
		return "SET_SPIN";

		case STATE_WASH:
		return "WASH";

		case STATE_RINSE:
		return "RINSE";

		case STATE_SPIN:
		return "SPIN";
		
		case STATE_PAUSE:
		return "PAUSE";
	}

	return "UNKNOWN";
}

void init_washer(void) {
	
	state = WASHER_IDLE;

    wash_time = 1;
    rinse_time = 1;
    spin_time = 1;
motor_dir_time = msec_count;  // ← 이거 추가
	motor_dir = 0;
    remain_time = 0;
		
}


static void washer_timer_update(void)
{
	if(msec_count != prev_ms)
	{
		prev_ms = msec_count;

		if(state == STATE_WASH ||
		state == STATE_RINSE ||
		state == STATE_SPIN)
		{
			if(remain_time > 0 || remain_msec > 0)
			{
				if(remain_msec == 0)
				{
					if(remain_time > 0)
					{
						remain_time--;
						remain_msec = 999;
					}
				}
				else
				{
					remain_msec--;
				}
			}
		}
	}
}

static void washer_button_process(void)
{
    // BTN1
    

    if(get_button(BUTTON1, BUTTON1PIN))
    {
        switch(state)
        {
            case WASHER_IDLE:
                state = SET_WASH_TIME;
                break;

            case SET_WASH_TIME:
                state = SET_RINSE_TIME;
                break;

            case SET_RINSE_TIME:
                state = SET_SPIN_TIME;
                break;

            case SET_SPIN_TIME:
                state = WASHER_IDLE;
                break;

            default:
                break;
        }
    }

    // BTN2

    if(get_button(BUTTON2, BUTTON2PIN))
    {
        switch(state)
        {
            case SET_WASH_TIME:
                wash_time++;
				if(wash_time > 9)
				wash_time = 1;
                break;

            case SET_RINSE_TIME:
                rinse_time++;
				if(rinse_time > 9)
				rinse_time = 1;
                break;

            case SET_SPIN_TIME:
                spin_time++;
				if(spin_time > 9)
				spin_time = 1;
                break;

            default:
                break;
        }
    }

    // BTN0 시작 / 정지
    

    if(get_button(BUTTON0, BUTTON0PIN))
    {
	    switch(state)
	    {
		    case WASHER_IDLE:

		    state = STATE_WASH;
		    remain_time = wash_time * 10;
			remain_msec = 0;
			motor_dir_time = msec_count;
		    printf("START\r\n");

		    break;

		    case STATE_WASH:
		    case STATE_RINSE:
		    case STATE_SPIN:

		    prev_state = state;
		    state = STATE_PAUSE;

		    printf("PAUSE\r\n");

		    break;

		    case STATE_PAUSE:

		    state = prev_state;

		    printf("RESUME\r\n");

		    break;

		    default:
		    break;
	    }
    }
	
	if(get_button(BUTTON3, BUTTON3PIN))
	{
		if(state == STATE_PAUSE)
		{
			state = WASHER_IDLE;

			remain_time = 0;
			remain_msec = 0;

			OCR3C = 0;

			PORTF &= ~((1<<6)|(1<<5));

			printf("CANCEL\r\n");
		}
	}
}

static void washer_state_process(void)
{
	switch(state)
	{
		case STATE_WASH:

		if(remain_time == 0 && remain_msec == 0)
		{
			state = STATE_RINSE;
			remain_time = rinse_time * 10;
			remain_msec = 0;
		}

		break;

		case STATE_RINSE:

		if(remain_time == 0 && remain_msec == 0)
		{
			state = STATE_SPIN;
			remain_time = spin_time * 10;
			remain_msec = 0;
		}

		break;

		case STATE_SPIN:

		if(remain_time == 0 && remain_msec == 0)
		{
			state = WASHER_IDLE;
		}

		break;

		default:
		break;
	}
}


static void washer_motor_control(void)
{
	switch(state)
	{
		case WASHER_IDLE:

			OCR3C = 0;

			PORTF &= ~((1<<6)|(1<<5));

			break;
	
		
		case STATE_PAUSE:

			OCR3C = 0;

			PORTF &= ~((1<<6)|(1<<5));

			break;

		case STATE_WASH:
		{
			OCR3C = 120;

			uint32_t cycle_time = (msec_count - motor_dir_time) % 8000;
			// 0~2999: 정회전, 3000~4999: 정지(2초), 5000~7999: 역회전, 8000~9999: 정지(2초)

			if(cycle_time < 3000)
			{
				// 정회전
				PORTF &= ~((1<<6)|(1<<5));
				PORTF |= (1<<6);
			}
			else if(cycle_time < 4000)
			{
				// 정지 1초
				PORTF &= ~((1<<6)|(1<<5));
			}
			else if(cycle_time < 7000)
			{
				// 역회전
				PORTF &= ~((1<<6)|(1<<5));
				PORTF |= (1<<5);
			}
			else
			{
				// 정지 1초
				PORTF &= ~((1<<6)|(1<<5));
			}
		}
		break;
		
		case STATE_RINSE:

			OCR3C = 180;

			if(msec_count - motor_dir_time >= 2000)
			{
				motor_dir_time = msec_count;

				motor_dir ^= 1;

				PORTF &= ~((1<<6)|(1<<5));

				if(motor_dir)
				PORTF |= (1<<6);
				else
				PORTF |= (1<<5);
			}

			break;
		
		case STATE_SPIN:

			OCR3C = 255;

			PORTF &= ~((1<<6)|(1<<5));
			PORTF |= (1<<6);

			break;
		
		default:
		break;
	}
}

static void washer_led_display(void)
{
	switch(state)
	{
		case STATE_WASH:
		PORTA = 0x01;   // LED0
		break;

		case STATE_RINSE:
		PORTA = 0x02;   // LED1
		break;

		case STATE_SPIN:
		PORTA = 0x04;   // LED2
		break;

		case SET_WASH_TIME:
		PORTA = 0x08;   // LED3
		break;

		case SET_RINSE_TIME:
		PORTA = 0x10;   // LED4
		break;

		case SET_SPIN_TIME:
		PORTA = 0x20;   // LED5
		break;

		case STATE_PAUSE:
		PORTA = 0x80;   // LED7
		break;

		default:
		PORTA = 0x00;
		break;
	}
}


//
//static void washer_debug(void)
//{
	//static uint32_t old_ms = 0;
//
	//if(msec_count - old_ms >= 1000)
	//{
		//old_ms = msec_count;
//
		//printf("STATE=%s ", state_string());
		//printf("TIME=%d\r\n", remain_time);
		//
	//}
//}


void washer_run(void)
{
	switch(state)
	{
			case SET_WASH_TIME:
			fnd_display_setting(wash_time);
			break;

			case SET_RINSE_TIME:
			fnd_display_setting(rinse_time);
			break;

			case SET_SPIN_TIME:
			fnd_display_setting(spin_time);
			break;

			default:
			fnd_display_washer(remain_time, remain_msec);
			break;
	}

		
	washer_button_process();

	washer_timer_update();

	washer_state_process();
	
	washer_motor_control();
	
	 washer_led_display();
	

	//washer_debug();
}