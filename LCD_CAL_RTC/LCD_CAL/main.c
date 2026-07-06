/*
 * LCD_CAL.c
 *
 * Created: 2026-07-01 오전 9:47:11
 * Author : kccistc
 */ 

#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>   // sei, ISR 등
#include <stdio.h>
#include <util/delay.h>
#include "button.h"
#include "queue.h"
#include "cal.h"
#include "lcd.h"
#include "ds1307.h"

void init_timer0();
extern void init_uart0();
extern void init_keypad();
extern void i2c_init();
extern void UART0_transmit(uint8_t data);
extern uint8_t keypad_scan(void);
extern void insert_queue(uint8_t value);
extern int queue_empty();
extern uint8_t read_queue();
extern void init_button();
extern int get_button(int button_num, int button_pin);
extern void LCD_init();
extern void lcd_goto_xy(uint8_t row, uint8_t col);
extern void lcd_write_command(uint8_t command);
extern void lcd_write_string(char *string);
extern void lcd_clear();
extern void cal_redraw(t_cal *c);
extern void cal_input(t_cal *c, uint8_t key);
extern void cal_reset(t_cal *c);
extern int ds1307_main(void);
extern void read_time_ds1307(t_ds1307* ds1307);
extern void read_date_ds1307(t_ds1307* ds1307);
extern void init_buzzer(void);
extern void buzzer_beep_for_key(uint8_t key);

typedef enum
{
	MODE_CALULATOR = 0,
	MODE_TIMER
} app_mode_t;

volatile uint32_t keypad_counter=0;
volatile uint32_t msec_count = 0;
volatile uint8_t  keypad_flag  = 0;


FILE OUTPUT = FDEV_SETUP_STREAM(UART0_transmit, NULL, _FDEV_SETUP_WRITE);


ISR(TIMER0_OVF_vect)
{
	volatile uint8_t keydata = 0;
	
	TCNT0 = 6;		// TCNT0 6~256: 250 펄스 count 하기 위해
	msec_count++;
	
	if (++keypad_counter >= 60)
	{
		keypad_counter = 0;
		if (keydata = keypad_scan())	//keypad를 체크해서 눌러진 것이 있어야 한다.
		{
			insert_queue(keydata);
			keypad_flag = 1;
		}
	};
	
	
}

int main(void)
{
	t_cal cal;
	app_mode_t app_mode = MODE_CALULATOR;
	uint32_t last_clock_update = 0;
	
	init_timer0();
	init_uart0();
	init_keypad();
	init_button();
	LCD_init();
	i2c_init();
	init_buzzer();
	
	cal_reset(&cal);
	
	stdout = &OUTPUT;		// printf가 동작할 수 있도록 stdout을 설정
	sei();

	lcd_write_string("hello !");
	
	while (1)
	{
		// BUTTON4 : CAL MODE <-> CLOCK MODE 전환 (항상 폴링)
		if (get_button(BUTTON4, BUTTON4PIN))
		{
			if (app_mode == MODE_CALULATOR)
			{
				app_mode = MODE_TIMER;
				lcd_clear();
				lcd_goto_xy(0, 0);
				lcd_write_string("CLOCK MODE");
				// TODO: DS1307(I2C) 드라이버 완성 후, 여기서 시간을 읽어와 표시하도록 교체
				_delay_ms(800);
				lcd_clear();
				last_clock_update = 0;
			}
			else
			{
				app_mode = MODE_CALULATOR;
				lcd_clear();
				lcd_goto_xy(0, 0);
				lcd_write_string("CAL MODE");
				_delay_ms(800);            // 잠깐 보여주고
				lcd_clear();
				cal_redraw(&cal);          // 이전에 입력하던 수식 / 마지막 결과를 화면에 복원 복원
			}
		}
		
		if (app_mode == MODE_CALULATOR)
		{
			if (keypad_flag)
			{
				keypad_flag = 0;
				
				uint8_t key = keypad_scan();      // 여기서 처리
				if (key) insert_queue(key);
			}
			
			// 버튼은 디바운스 로직 때문에 매 루프마다 계속 폴링해야 함
			if (get_button(BUTTON0, BUTTON0PIN)) insert_queue('H');   // 헥사 변환
			if (get_button(BUTTON1, BUTTON1PIN)) insert_queue('C');   // reset
			if (get_button(BUTTON2, BUTTON2PIN)) insert_queue('(');
			if (get_button(BUTTON3, BUTTON3PIN)) insert_queue(')');
			
			if (!queue_empty())
			{
				uint8_t key = read_queue();
				// buzzer_beep_for_key(key);
				cal_input(&cal, key);
			}
		}
		else // MODE_CLOCK
		{
			// 계산기 입력이 쌓이지 않도록 키패드 플래그만 비움 (계산 로직은 타지 않음)
			keypad_flag = 0;
			
			// TODO: DS1307에서 시간을 주기적으로 읽어와 lcd_goto_xy()/lcd_write_string()으로 표시
			if (msec_count - last_clock_update >= 1000)
			{
				last_clock_update = msec_count;
				
				t_ds1307 now;
				read_time_ds1307(&now);
				read_date_ds1307(&now);
				
				char line0[17];
				char line1[17];
				
				snprintf(line0, sizeof(line0), "20%02u-%02u-%02u", now.year, now.month, now.date);
				snprintf(line1, sizeof(line1), "%02u:%02u:%02u", now.hours, now.minutes, now.seconds);
				
				lcd_goto_xy(0, 0);
				lcd_write_string(line0);
				lcd_goto_xy(1, 0);
				lcd_write_string(line1);
			}
		}
	}
}

void init_timer0()
{
	// TCNT0 = TimerCount0
	// TCNT0 6 ~ 256 : 250개 펄스를 count 하기 위해 6부터 시작
	TCNT0 = 6;
	
	TCCR0 &= ~( 1<< CS02 | 1 << CS01 | 1 << CS00);	// 0분주 reset 먼저
	TCCR0 |= 1 << CS02 | 0 << CS01 | 0 << CS00;		// 64분주
	TIMSK |= 1 << TOIE0;	// TIMER0 Overflow INT

}

