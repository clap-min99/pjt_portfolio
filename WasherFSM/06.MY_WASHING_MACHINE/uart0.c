/*
 * uart0.c
 *
 * Created: 2026-06-16 오전 9:57:55
 *  Author: kccistc
 */ 

#include "uart0.h"
#include <stdio.h>
void init_uart0();
void UART0_transmit(uint8_t data);
void pc_command_processing();


extern int func_state;
extern void (*fp[]) ();
extern volatile char scm[50];

// p.278 표 12-3
// pc로부터 1byte 들어오면 자동으로 이곳에 진입
// ex) led_all_on\n 이면 11번 이곳으로 진입
ISR(USART0_RX_vect)
{
	// interrupt 쓰는 변수는 volatile로 해야함
	volatile uint8_t data;
	volatile static int i=0;
	
	// UDR0를 data에 복사하는 순간 UDR0의 내용은 없어지므로, 데이터가 '이동'한다고 생각해도 무방함
	data = UDR0;
	
	if (data == '\n' || data == '\r')
	{
		if ((rear+1) % QUEUE_SIZE == front % QUEUE_SIZE)
		{
			return;	// queue full 상태
		}
		rx_buff[rear][i] = '\0';		// 문장의 끝인 NULL을 넣는다. 
		i = 0;							// 다음 string을 저장하기 위해 i를 0으로 만든다.
		rear = (rear + 1) % QUEUE_SIZE;	// 0~9
	}
	else 
	{
		if ((rear+1) % QUEUE_SIZE == front % QUEUE_SIZE)
		{
			return;	// queue full 상태
		}
		rx_buff[rear][i++] = data;	
	}
}

/*
1. 전송속도 : 9600bps
2. start/stop 설정
3. RX(수신): interrupt 로 설정

*/
void init_uart0(void){
	UBRR0H = 0x00;
	UBRR0L = 207;			// 9600bps, 표 8-9
	UCSR0A |= 1 << U2X0;	// 2배속 설정
	// UART0 송신, 수신이 모두 가능하도록 RX Interrupt가 가능하도록 설정.	
	UCSR0B |= 1 << RXEN0 | 1 << TXEN0 | 1 << RXCIE0; // RXCIE: RX complete interrupt enable 
}

// UART0로 1byte 전송하는 함수
void UART0_transmit(uint8_t data)
{
	while( !(UCSR0A & (1 << UDRE0))); // no operation -> data가 송신중이면 송신이 끝날 때까지 기다림

	UDR0 = data;			// HW 전송 register에 data를 송신한다.
	
}

void pc_command_processing()
{
	if (front != rear)		// data가 rx_buff에 존재하는지 check
	{	
		
		printf("%s", rx_buff[front]);	 // printf("%s", &rx_buff[front][0]
		if (strncmp((const char *)rx_buff[front], "led_shift_left_on", strlen("led_shift_left_on"))==0) {
			func_state = 0;
		}
		else if (strncmp((const char *)rx_buff[front], "led_shift_right_on", strlen("led_shift_right_on"))==0) {
			func_state = 1;
		}
		else if (strncmp((const char *)rx_buff[front], "led_shift_left_keepon", strlen("led_shift_left_keepon"))==0) {
			func_state = 2;
		}
		else if (strncmp((const char *)rx_buff[front], "led_shift_right_keepon", strlen("led_shift_right_keepon"))==0) {
			func_state = 3;
		}
		else if (strncmp((const char *)rx_buff[front], "led_flower_on", strlen("led_flower_on"))==0) {
			func_state = 4;
		}
		else if (strncmp((const char *)rx_buff[front], "led_flower_off", strlen("led_flower_off"))==0) {
			func_state = 5;
		}
		
		front = (front+1) % QUEUE_SIZE;
	}	
	
	fp[func_state] ();
	
};