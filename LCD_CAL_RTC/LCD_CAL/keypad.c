/*
 * keypad.c
 *
 * Created: 2026-06-29 오후 2:07:57
 *  Author: kccistc
 */ 

#include "keypad.h"


void init_keypad(void);
uint8_t keypad_scan(void);
uint8_t get_keypad(int row, int col);

void init_keypad()
{
	KEYPAD_DDR = 0x0f;	// row: 입력 col: 출력
	KEYPAD_PORT = 0xff; // 전체 HIGH (풀업 + COL 비활성)
}

uint8_t keypad_scan(void)
{
	uint8_t data = 0;
	
	for(int row=0; row<4; row++)
	{
		for(int col=0;col<4;col++)
		{
			data = get_keypad(row, col);
			if(data)
			return data;
		}
	}
	 return 0; 
}

uint8_t get_keypad(int row, int col)
{
	uint8_t keypad_char[4][4] = {
		{ '0','\b', '=', '+'},
		{'1', '2', '3', '/'},
		{'4', '5', '6', '*'},
		{'7', '8', '9', '-'}
	};
	
	static int8_t prev_state[4][4] =	
	{
		{ 1, 1, 1, 1 },			// 초기버튼은 눌러지지 않은 상태로 한다.
		{ 1, 1, 1, 1 },
		{ 1, 1, 1, 1 },
		{ 1, 1, 1, 1 }
	};
	
	int8_t current_state = 1;
	KEYPAD_PORT = 0xff;
	KEYPAD_PORT &= ~(1 << 3-col); // 해당 col에 전류를 흘린다.
	
	for (int delay = 0; delay<20; delay++); // keypad 체크를 위한 delay(0.625us * 20 = 1.25us)
	
		
	current_state = ( KEYPAD_PIN & (1<<(row+4)) ) >> (row+4);
	// 예: row0: pa4에 연결
	// 76543210
	// 11101111 KEYPAD_PIN & (1<<(row+4))
	// 00001110
	
	
	if (current_state == 0 && prev_state[row][col] == 1) // 처음 눌려진 상태
	{
		prev_state[row][col] = 0;
		return 0;
	}
	else if (current_state == 1 && prev_state[row][col] == 0)
	{
		// 이전에 버튼이 눌려지고 현재는 버튼을 뗀 상태이면 버튼을 1번 눌렀다 뗀 것으로 인정
		prev_state[row][col] = 1;	// prev_state를 초기화
		// printf("key: %c\n", keypad_char[row][col]);
		return keypad_char[row][col];
	}
	
	return 0;
}



 