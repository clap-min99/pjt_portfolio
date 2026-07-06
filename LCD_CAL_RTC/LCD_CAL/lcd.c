/*
 * lcd.c
 *
 * Created: 2026-07-01 오전 10:29:54
 *  Author: kccistc
 */ 

#include "lcd.h"

void LCD_init(void);
static void lcd_pulse_enable(void);
void lcd_write_data(uint8_t data);
void lcd_write_command(uint8_t command);
void lcd_write_string(char *string);
void lcd_goto_xy(uint8_t row, uint8_t col);
void lcd_clear(void);
uint8_t MODE = 4;

// E 핀에 Enable 펄스(High->Low)를 인가
static void lcd_pulse_enable(void)
{
	PORT_CONTROL |= (1 << E_PIN);
	_delay_ms(1);
	PORT_CONTROL &= ~(1 << E_PIN);
	_delay_ms(1);	
}

// 4비트(상위 니블) 데이터를 PC4~PC7에 실어서 전송
void lcd_write_data(uint8_t data)
{

	PORT_CONTROL |= (1<< RS_PIN);
	if(MODE == 8){
		PORT_DATA = data;
		lcd_pulse_enable();
	}
	else {
	PORT_DATA = data & 0xF0;
	lcd_pulse_enable();
	PORT_DATA = (data << 4) & 0xF0;
	lcd_pulse_enable();
	}
	_delay_ms(2);
}


void lcd_write_command(uint8_t command)
{
	PORT_CONTROL &= ~(1 << RS_PIN);
	
	if(MODE == 8) {
		PORT_DATA = command;
		lcd_pulse_enable();
	}
	else {
		PORT_DATA = command & 0xF0;
		lcd_pulse_enable();
		PORT_DATA = (command << 4) & 0xF0;
		lcd_pulse_enable();
	}
	
	_delay_ms(2);
}

void lcd_clear(void)
{
	lcd_write_command(COMMAND_CLEAR_DISPLAY);
	_delay_ms(2);
}

void LCD_init(void)
{

	_delay_ms(50);
	
	if(MODE==8) DDR_DATA |= 0xFF;
	else DDR_DATA |= 0xF0;
	PORT_DATA = 0x00;
	DDR_CONTROL |= (1 << RS_PIN) | (1 << RW_PIN) | (1 << E_PIN);
	
	// R/W 핀으로 low를 출력하여 쓰기 전용으로 사용
	PORT_CONTROL &= ~(1 << RW_PIN);
	
	if(MODE == 8)
		lcd_write_command(COMMAND_8_BIT_MODE);
	else
	{
		lcd_write_command(0x02);
		lcd_write_command(COMMAND_4_BIT_MODE);
	}
	
	// display on/off control
	// 화면 on, 커서 off, 커서 깜빡임 off
	uint8_t command = 0x08 | (1 << COMMAND_DISPLAY_ON_OFF_BIT);
	lcd_write_command(command);
	
	lcd_clear();

	// entry mode set
	// 출력 후 커서를 오른쪽으로 옮김. 즉 DDRAM의 주소가 증가, 화면 이동은 없음
	lcd_write_command(0x06);
}

void lcd_write_string(char *string)
{
	uint8_t i;
	for (i=0; string[i]; i++)
	{
		lcd_write_data(string[i]);
	}
}

void lcd_goto_xy(uint8_t row, uint8_t col)
{
	col %= 16;
	row %= 2;
	
	//첫째 라인 시작 주소는 0x00, 둘째 라인 시작 주소는 0x40
	uint8_t address = (0x40 * row) + col;
	uint8_t command = 0x80 + address;

	lcd_write_command(command);	// 커서 이동
}
