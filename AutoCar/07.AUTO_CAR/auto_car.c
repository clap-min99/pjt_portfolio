/*
 * auto_car.c
 *
 * Created: 2026-06-22 오후 3:15:56
 *  Author: kccistc
 */ 
#include "auto_car.h"
#include <stdio.h>
#include <avr/interrupt.h>
#include <stdlib.h>

#define FND2_CHAR_DASH  10
#define FND2_CHAR_F     11
#define FND2_CHAR_b     12
#define FND2_CHAR_L     13
#define FND2_CHAR_r     14
#define FND2_CHAR_S     15
#define TURN_MARGIN 10

#define obstacle_dist   10
#define corner_dist     12

extern int func_state;

void init_auto_car(void);
void init_fnd1(void);
void init_fnd2(void);
int get_auto_button(void);
void set_auto_led(int state);
void fnd1_display(uint32_t ms);
void fnd1_scan(uint32_t elapsed_ms);
void fnd2_scan(uint8_t dir, uint8_t speed_pct);
void init_ultrasonic(void);
void ultrasonic_processing(void);
void make_trigger(uint8_t pin);
void auto_drive(void);
void reset_drive_stats(void);
void fnd2_result_scan(void);

extern void backward(int speed);
extern void forward(int speed);
extern void turn_left(int speed);
extern void turn_right(int speed);
extern void stop();

// 주행 통계
volatile uint16_t cnt_f = 0;
volatile uint16_t cnt_b = 0;
volatile uint16_t cnt_l = 0;
volatile uint16_t cnt_r = 0;
volatile uint32_t total_drive_ms = 0;   // 총 주행시간 (정지 시 확정)
static uint8_t last_dir = 'S';   // 직전 방향 (변화 감지용)

volatile int dist_left = 100;
volatile int dist_front = 100;
volatile int dist_right = 100;

static volatile uint8_t ultra_step = 0;

extern volatile uint32_t msec_count;
extern volatile uint8_t current_dir;
extern volatile uint8_t current_speed;
extern volatile uint32_t total_drive_ms;


int error;

uint32_t state_start_ms;


// 속도값(0~1023)을 %로 변환해서 FND2에 반영
static void update_status(uint8_t dir, int speed)
{
	current_dir = dir;
	current_speed = (uint16_t)speed * 100 / 1023;

	// 방향이 바뀌는 순간에만 카운트
	if (dir != last_dir)
	{
		switch (dir)
		{
			case 'F': case 'f': cnt_f++; break;
			case 'b': case 'B': cnt_b++; break;
			case 'L': case 'l': cnt_l++; break;
			case 'r': case 'R': cnt_r++; break;
		}
		last_dir = dir;
	}
}
void reset_drive_stats(void)
{
	cnt_f = cnt_b = cnt_l = cnt_r = 0;
	last_dir = 'S';
}

// ========== FND1 세그먼트 ==========
// Common Anode: 0이면 켜짐
// PORTD: 7=A, 6=B, 5=C, 4=D, 3=E, 2=F, 1=G, 0=DP
static const uint8_t fnd_font[] = {
	0x03, // 0
	0x9F, // 1
	0x25, // 2
	0x0D, // 3
	0x99, // 4
	0x49, // 5
	0x41, // 6
	0x1F, // 7
	0x01, // 8
	0x09, // 9
	0x02, // 0.
	0x9E, // 1.
	0x24, // 2.
	0x0C, // 3.
	0x98, // 4.
	0x48, // 5.
	0x40, // 6.
	0x1E, // 7.
	0x00, // 8.
	0x08, // 9.
};

// FND2 폰트 (PORTC0=A 기준, Common Anode)
// PORTC: 7=DP, 6=G, 5=F, 4=E, 3=D, 2=C, 1=B, 0=A
static const uint8_t fnd2_font[] = {
	0xC0, // 0  
	0xF9, // 1 
	0xA4, // 2  
	0xB0, // 3  
	0x99, // 4 
	0x92, // 5  
	0x82, // 6 
	0xD8, // 7 
	0x80, // 8  
	0x98, // 9 
	0xBF, // -  
	// 알파벳
	0x8E, // F 
	0x83, // b 
	0xC7, // L 
	0xAF, // r 
	0x92, // S 
};

void init_auto_car(void)
{
	// 버튼 PG0 입력 모드
	BTN_DDR &= ~(1 << BTN_BIT);

	// LED PG4 출력 모드
	AUTO_LED_DDR |= (1 << AUTO_LED_BIT);
	AUTO_LED_PORT &= ~(1 << AUTO_LED_BIT);  // 초기 off
}


void make_trigger(uint8_t pin)
{
	TRIG_PORT &= ~(1 << pin);
	_delay_us(2);
	TRIG_PORT |= (1 << pin);
	_delay_us(10);
	TRIG_PORT &= ~(1 << pin);
}

// INT4: 왼쪽 에코
ISR(INT4_vect)
{
	if (ECHO_PORT & (1 << ECHO_LEFT))
	TCNT3 = 0;
	else
	dist_left = (int)(TCNT3 * 1000000.0 * 1024 / F_CPU / 58);
}

// INT5: 정면 에코
ISR(INT5_vect)
{
	if (ECHO_PORT & (1 << ECHO_FRONT))
	TCNT3 = 0;
	else
	dist_front = (int)(TCNT3 * 1000000.0 * 1024 / F_CPU / 58);
}

// INT6: 오른쪽 에코
ISR(INT6_vect)
{
	if (ECHO_PORT & (1 << ECHO_RIGHT))
	TCNT3 = 0;
	else
	dist_right = (int)(TCNT3 * 1000000.0 * 1024 / F_CPU / 58);
}

void init_ultrasonic(void)
{
	// 트리거 출력
	TRIG_DDR |= (1<<TRIG_LEFT)|(1<<TRIG_FRONT)|(1<<TRIG_RIGHT);
	TRIG_PORT &= ~((1<<TRIG_LEFT)|(1<<TRIG_FRONT)|(1<<TRIG_RIGHT));

	// 에코 입력
	ECHO_DDR &= ~((1<<ECHO_LEFT)|(1<<ECHO_FRONT)|(1<<ECHO_RIGHT));

	// INT4, INT5, INT6: 상승/하강 엣지 모두
	EICRB |= (0<<ISC41)|(1<<ISC40);
	EICRB |= (0<<ISC51)|(1<<ISC50);
	EICRB |= (0<<ISC61)|(1<<ISC60);

	// Timer3 초기화 후 1024분주 
	TCCR3B = 0;
	TCCR3B |= (1<<CS32)|(1<<CS30);

	// INT4, INT5, INT6 활성화
	EIMSK |= (1<<INT4)|(1<<INT5)|(1<<INT6);
}

void ultrasonic_processing(void)
{
	static uint32_t last_time = 0;

	if (msec_count - last_time >= 100)  // 100ms마다 순차 측정
	{
		last_time = msec_count;

		switch (ultra_step)
		{
			case 0: make_trigger(TRIG_LEFT);  ultra_step = 1; break;
			case 1: make_trigger(TRIG_FRONT); ultra_step = 2; break;
			case 2: make_trigger(TRIG_RIGHT); ultra_step = 0; break;
		}

	}
}


// ========== FND1 버퍼 업데이트 ==========
// ms: 자율주행 시작 후 경과 밀리초
void init_fnd1(void)
{
	// PORTD 상위 4비트(A,B,C,D) 출력
	FND1_DATA_DDR  |= 0xF0;
	FND1_DATA_PORT |= 0xF0;   // off

	// PORTB 하위 4비트(E,F,G,DP) 출력
	DDRB  |= 0x0F;
	PORTB |= 0x0F;            // off

	// 자릿수 PF4~7 출력
	FND1_DIGIT_DDR  |= (1<<FND1_D1)|(1<<FND1_D2)|(1<<FND1_D3)|(1<<FND1_D4);
	FND1_DIGIT_PORT &= ~((1<<FND1_D1)|(1<<FND1_D2)|(1<<FND1_D3)|(1<<FND1_D4));
}

void init_fnd2(void)
{
	FND2_DATA_DDR  = 0xFF;
	FND2_DATA_PORT = 0xFF;  // 전체 off (Common Anode)

	FND2_DIGIT_DDR  |= (1<<FND2_D1)|(1<<FND2_D2)|(1<<FND2_D3)|(1<<FND2_D4);
	FND2_DIGIT_PORT &= ~((1<<FND2_D1)|(1<<FND2_D2)|(1<<FND2_D3)|(1<<FND2_D4));
}



// 버튼 눌렀다 떼면 1 리턴
int get_auto_button(void)
{
	static unsigned char stable    = BUTTON_RELEASE;
	static unsigned char last_raw  = BUTTON_RELEASE;
	static uint32_t      change_t  = 0;

	unsigned char raw = (BTN_PIN & (1 << BTN_BIT)) ? BUTTON_PRESS : BUTTON_RELEASE;

	if (raw != last_raw)
	{
		last_raw = raw;
		change_t = msec_count;
	}

	if ((msec_count - change_t) >= 20 && raw != stable)
	{
		stable = raw;
		if (stable == BUTTON_RELEASE)
		return 1;  // 눌렀다 뗀 순간
	}

	return 0;
}

void set_auto_led(int state)
{
	if (state)
	AUTO_LED_PORT |= (1 << AUTO_LED_BIT);   // LED ON
	else
	AUTO_LED_PORT &= ~(1 << AUTO_LED_BIT);  // LED OFF
}


// 폰트 한 바이트를 두 포트로 분배해서 출력
static void fnd1_output(uint8_t seg)
{
	FND1_DATA_PORT = (FND1_DATA_PORT & 0x0F) | (seg & 0xF0);
	PORTB = (PORTB & 0xF0) | (seg & 0x0F);
}

// ========== FND1 스캔 (while문에서 계속 호출) ==========
void fnd1_scan(uint32_t elapsed_ms)
{
	static uint8_t digit = 0;
	uint8_t seg_data;

	if (elapsed_ms < 60000)
	{
		uint8_t sec  = (elapsed_ms / 1000) % 60;
		uint8_t csec = (elapsed_ms % 1000) / 10;
		uint8_t d[4] = { sec/10, sec%10 + 10, csec/10, csec%10 };
		seg_data = fnd_font[d[digit]];
	}
	else
	{
		uint8_t min = elapsed_ms / 60000;
		uint8_t sec = (elapsed_ms % 60000) / 1000;
		uint8_t d[4] = { min/10, min%10 + 10, sec/10, sec%10 };
		seg_data = fnd_font[d[digit]];
	}

	// 자릿수 off
	FND1_DIGIT_PORT &= ~((1<<FND1_D1)|(1<<FND1_D2)|(1<<FND1_D3)|(1<<FND1_D4));

	// 세그먼트 두 포트로 분배 출력
	fnd1_output(seg_data);

	switch (digit)
	{
		case 0: FND1_DIGIT_PORT |= (1<<FND1_D1); break;
		case 1: FND1_DIGIT_PORT |= (1<<FND1_D2); break;
		case 2: FND1_DIGIT_PORT |= (1<<FND1_D3); break;
		case 3: FND1_DIGIT_PORT |= (1<<FND1_D4); break;
	}

	digit = (digit + 1) % 4;
}

void fnd2_scan(uint8_t dir, uint8_t speed_pct)
{
	static uint8_t digit = 0;
	uint8_t seg_data;
	
	// 방향 문자 선택
	uint8_t dir_char;
	switch (dir)
	{
		case 'f': case 'F': dir_char = FND2_CHAR_F; break;
		case 'b': case 'B': dir_char = FND2_CHAR_b; break;
		case 'l': case 'L': dir_char = FND2_CHAR_L; break;
		case 'r': case 'R': dir_char = FND2_CHAR_r; break;
		case 's': case 'S': dir_char = FND2_CHAR_S; break;
		default:            dir_char = FND2_CHAR_S; break;
	}

	// 속도 % (0~99)
	uint8_t spd_ten = (speed_pct / 10) % 10;
	uint8_t spd_one = speed_pct % 10;

	switch (digit)
	{
		case 0: seg_data = fnd2_font[dir_char];      break;  // 방향
		case 1: seg_data = fnd2_font[FND2_CHAR_DASH]; break; // - (구분)
		case 2: seg_data = fnd2_font[spd_ten];       break;  // 속도 십
		case 3: seg_data = fnd2_font[spd_one];       break;  // 속도 일
		default: seg_data = 0xFF; break;
	}

	// off → 데이터 → on
	FND2_DIGIT_PORT &= ~((1<<FND2_D1)|(1<<FND2_D2)|(1<<FND2_D3)|(1<<FND2_D4));
	FND2_DATA_PORT = seg_data;

	switch (digit)
	{
		case 0: FND2_DIGIT_PORT |= (1<<FND2_D1); break;
		case 1: FND2_DIGIT_PORT |= (1<<FND2_D2); break;
		case 2: FND2_DIGIT_PORT |= (1<<FND2_D3); break;
		case 3: FND2_DIGIT_PORT |= (1<<FND2_D4); break;
	}

	digit = (digit + 1) % 4;
}

// 정지 후: f/b/l/r 횟수를 1초마다 순환 표시
void fnd2_result_scan(void)
{
	static uint8_t  phase = 0;        // 0=f 1=b 2=l 3=r
	static uint32_t last_switch = 0;
	static uint8_t  digit = 0;

	// 1초마다 다음 방향으로
	if (msec_count - last_switch >= 1000)
	{
		last_switch = msec_count;
		phase = (phase + 1) % 4;
	}

	uint8_t dir_char;
	uint16_t count;
	switch (phase)
	{
		case 0: dir_char = FND2_CHAR_F; count = cnt_f; break;
		case 1: dir_char = FND2_CHAR_b; count = cnt_b; break;
		case 2: dir_char = FND2_CHAR_L; count = cnt_l; break;
		default:dir_char = FND2_CHAR_r; count = cnt_r; break;
	}

	if (count > 99) count = 99;   // 2자리 제한
	uint8_t ten = (count / 10) % 10;
	uint8_t one = count % 10;

	uint8_t seg_data;
	switch (digit)
	{
		case 0: seg_data = fnd2_font[dir_char];       break;  // 방향
		case 1: seg_data = fnd2_font[FND2_CHAR_DASH]; break;  // -
		case 2: seg_data = fnd2_font[ten];            break;  // 십
		case 3: seg_data = fnd2_font[one];            break;  // 일
		default: seg_data = 0xFF; break;
	}

	FND2_DIGIT_PORT &= ~((1<<FND2_D1)|(1<<FND2_D2)|(1<<FND2_D3)|(1<<FND2_D4));
	FND2_DATA_PORT = seg_data;

	switch (digit)
	{
		case 0: FND2_DIGIT_PORT |= (1<<FND2_D1); break;
		case 1: FND2_DIGIT_PORT |= (1<<FND2_D2); break;
		case 2: FND2_DIGIT_PORT |= (1<<FND2_D3); break;
		case 3: FND2_DIGIT_PORT |= (1<<FND2_D4); break;
	}

	digit = (digit + 1) % 4;
}

void auto_drive(void)
{
	static uint32_t stuck_start = 0;
	static uint8_t  stuck = 0;

	int l = dist_left;
	int m = dist_front;  
	int r = dist_right;

	// ── stuck 감지 ──
	uint8_t obstacle = (m <= obstacle_dist)
	|| (l <= obstacle_dist)
	|| (r <= obstacle_dist);

	if (obstacle)
	{
		if (!stuck)
		{
			stuck = 1;
			stuck_start = msec_count;
		}
	}
	else
	{
		stuck = 0;
	}

	// ── 2초 이상 막히면 후진 ──
	if (stuck && (msec_count - stuck_start >= 2000))
	{
		backward(600);
		update_status('b', 600);

		if (msec_count - stuck_start >= 3000)
		stuck = 0;
		return;
	}

	// ── 회피 ──
	if (l <= corner_dist && r <= corner_dist)
	{
		backward(600);
		update_status('b', 600);
	}
	else if (m <= obstacle_dist && l <= obstacle_dist)
	{
		turn_right(600);
		update_status('L', 600);
	}
	else if (m <= obstacle_dist && r <= obstacle_dist)
	{
		turn_left(600);
		update_status('r', 600);
	}
	else if (m <= obstacle_dist)
	{
		if (l > r) { turn_left(600);  update_status('r', 600); }
		else       { turn_right(600); update_status('L', 600); }
	}
	else if (l <= obstacle_dist)
	{
		turn_right(600);
		update_status('L', 600);
	}
	else if (r <= obstacle_dist)
	{
		turn_left(600);
		update_status('r', 600);
	}
	else
	{
		forward(400);
		update_status('f', 400);
	}
}
