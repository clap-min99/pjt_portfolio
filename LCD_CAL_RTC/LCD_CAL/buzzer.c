/*
 * buzzer.c
 *
 * Author: kccistc
 *
 * Timer3 하드웨어 PWM(OC3A, PE3)으로 부저 제어
 */

#include "buzzer.h"

void init_buzzer(void);
void buzzer_beep(void);
void buzzer_tone(uint16_t ocr3a_value, uint16_t duration_ms);
void buzzer_beep_for_key(uint8_t key);
static uint16_t buzzer_note_for_key(uint8_t key);
static void delay_ms_var(uint16_t ms);   // _delay_ms(1)을 ms번 반복 (컴파일타임 상수 제약 회피)

// Timer3 위상교정이 아닌 CTC 모드로 설정, PE3(OC3A)에서 사각파 출력
void init_buzzer(void)
{
	DDRE |= 0x08;                          // OC3A(PE3) 출력 모드로 설정
	TCCR3A = (1 << COM3A0);                // 비교일치 시 OC3A 출력 반전(toggle)
	TCCR3B = (1 << WGM32) | (1 << CS31);   // CTC(TOP=OCR3A), 8분주
	TCCR3C = 0;
	OCR3A  = 0;                            // 초기값: 무음
}

// ms는 실행 중 정해지는 값이라 _delay_ms()에 바로 못 넣으므로,
// _delay_ms(1)을 ms번 반복하는 방식으로 우회 (참고 코드의 delay_ms()와 동일한 패턴)
static void delay_ms_var(uint16_t ms)
{
	while (ms-- != 0)
	{
		_delay_ms(1);
	}
}

// OCR3A 값을 설정해서 duration_ms 동안 소리 재생 후, 짧은 무음 구간을 둠(스타카토 느낌)
void buzzer_tone(uint16_t ocr3a_value, uint16_t duration_ms)
{
	OCR3A = ocr3a_value;
	delay_ms_var(duration_ms);
	OCR3A = 0;
	_delay_ms(10);
}

// 키패드가 아닌 다른 입력(버튼 등)용 단순 비프음
void buzzer_beep(void)
{
	buzzer_tone(500, 100);
}

/* -----------------------------------------------------------------
 * S1~S16 키 -> 음(도~레#, 반음씩 상승) 매핑
 * S1~S16 번호는 keypad_char[row][col] 배열의 순서(0행 왼쪽->오른쪽,
 * 그 다음 1행...)로 가정함. 실제 키패드 라벨과 다르면 이 표만 수정하면 됨.
 *
 * S번호 :  1    2    3    4    5    6    7    8    9   10   11   12   13   14   15   16
 * 키     : '0' '\b' '=' '+' '1'  '2'  '3'  '/' '4'  '5'  '6'  '*' '7'  '8'  '9'  '-'
 * 음     : 도   도#   레   레#   미   파   파#   솔   솔#   라   라#   시   도    도#   레   레#
 * ----------------------------------------------------------------- */
static const char     key_table[16]  = { '0','\b','=','+', '1','2','3','/', '4','5','6','*', '7','8','9','-' };
static const uint16_t note_table[16] = {
	DO_01, DO_01_H, RE_01, RE_01_H,
	MI_01, FA_01,   FA_01_H, SO_01,
	SO_01_H, LA_01, LA_01_H, TI_01,
	DO_02, DO_02_H, RE_02, RE_02_H
};

// key에 매핑된 음의 OCR3A 값을 반환. 매핑 안 된 키(예: 버튼용 문자)는 0 반환
static uint16_t buzzer_note_for_key(uint8_t key)
{
	uint8_t i;
	for (i = 0; i < 16; i++)
	{
		if (key_table[i] == key)
		{
			return note_table[i];
		}
	}
	return 0;
}

// 키패드 키(S1~S16)에 매핑된 음을 재생. 키패드가 아닌 입력(버튼 등)은 단순 비프음으로 대체
void buzzer_beep_for_key(uint8_t key)
{
	uint16_t tone = buzzer_note_for_key(key);

	if (tone)
	{
		buzzer_tone(tone, 150);
	}
	else
	{
		buzzer_beep();
	}
}