/*
 * button.c
 *
 * Created: 2026-06-12 오후 1:22:26
 *  Author: kccistc
 */ 

/*
 * button.c
 *
 * Created: 2026-06-12 오후 1:22:26
 *  Author: kccistc
 */ 

#include "button.h"		

extern volatile uint32_t msec_count; 

void init_button(void);
int get_button(int button_num, int button_pin);

#define DEBOUNCE_MS 20    

// 버튼 초기화 (입력 방향)
void init_button(void)
{
	BUTTON_DDR &= ~(1 << BUTTON0PIN | 1 << BUTTON1PIN | 1 << BUTTON2PIN | 1 << BUTTON3PIN);
}

int get_button(int button_num, int button_pin)
{
	// stable   : 디바운스로 확정된 상태
	// last_raw : 직전에 읽은 raw 값
	// change_t : raw 값이 마지막으로 바뀐 시각(ms)
	static unsigned char stable[BUTTON_NUMBER]   = { BUTTON_RELEASE, BUTTON_RELEASE, BUTTON_RELEASE, BUTTON_RELEASE };
	static unsigned char last_raw[BUTTON_NUMBER] = { BUTTON_RELEASE, BUTTON_RELEASE, BUTTON_RELEASE, BUTTON_RELEASE };
	static uint32_t      change_t[BUTTON_NUMBER] = { 0, 0, 0, 0 };
	
	// 1) 현재 핀 raw 값 
	unsigned char raw = (BUTTON_PIN & (1 << button_pin)) ? BUTTON_PRESS : BUTTON_RELEASE;
	
	// 2) raw 가 바뀌면 그 시각을 기록 (여기서 기다리지 않음)
	if (raw != last_raw[button_num])
	{
		last_raw[button_num] = raw;
		change_t[button_num] = msec_count;
	}
	
	// 3) raw 가 DEBOUNCE_MS 이상 그대로 유지되면 확정 상태에 반영
	if ((msec_count - change_t[button_num]) >= DEBOUNCE_MS && raw != stable[button_num])
	{
		stable[button_num] = raw;
		
		// PRESS -> RELEASE 로 확정되는 순간 = "한 번 눌렀다 뗌"
		if (stable[button_num] == BUTTON_RELEASE)
		return 1;
	}
	
	return 0;
}