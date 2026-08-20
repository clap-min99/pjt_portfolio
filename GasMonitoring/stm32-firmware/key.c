#include "key.h"
#include "macro.h"

// 핀 번호 정의 (PB4: 밸브 차단, PB5: 밸브 복구)
#define PIN_VALVE_CLOSE 4
#define PIN_VALVE_OPEN 5

// 이전 스위치 입력 상태 (내부 풀업 기본값 HIGH = 1)
static uint8_t g_prev_close_key = 1;
static uint8_t g_prev_open_key = 1;

void Key_Init(void)
{
	// GPIOB 클럭 활성화 (AHB1ENR Bit 1)
	Macro_Set_Bit(RCC->AHB1ENR, 1);

	// 차단(PB4), 복구(PB5) 스위치 범용 입력 모드(00b) 설정
	Macro_Write_Block(GPIOB->MODER, 0x3, 0x0, PIN_VALVE_CLOSE * 2); // PB4 [9:8]
	Macro_Write_Block(GPIOB->MODER, 0x3, 0x0, PIN_VALVE_OPEN * 2);	// PB5 [11:10]

	// 차단(PB4), 복구(PB5) 스위치 내부 풀업 저항 활성화 (01b)
	Macro_Write_Block(GPIOB->PUPDR, 0x3, 0x1, PIN_VALVE_CLOSE * 2); // PB4 [9:8]
	Macro_Write_Block(GPIOB->PUPDR, 0x3, 0x1, PIN_VALVE_OPEN * 2);	// PB5 [11:10]
}

KeyEvent Key_Scan(void)
{
	KeyEvent event = KEY_EVENT_NONE;

	// 현재 핀 입력 상태 읽기 (Active-Low: 누름 = 0, 뗌 = 1)
	uint8_t curr_close_key = (GPIOB->IDR & (1U << PIN_VALVE_CLOSE)) ? 1 : 0;
	uint8_t curr_open_key = (GPIOB->IDR & (1U << PIN_VALVE_OPEN)) ? 1 : 0;

	// 밸브 차단 버튼(PB4) Falling Edge 검출 (1 -> 0)
	if (g_prev_close_key == 1 && curr_close_key == 0)
	{
		event = KEY_EVENT_VALVE_CLOSE;
	}
	// 밸브 복구 버튼(PB5) Falling Edge 검출 (1 -> 0)
	else if (g_prev_open_key == 1 && curr_open_key == 0)
	{
		event = KEY_EVENT_VALVE_OPEN;
	}

	// 직전 상태 갱신
	g_prev_close_key = curr_close_key;
	g_prev_open_key = curr_open_key;

	return event;
}