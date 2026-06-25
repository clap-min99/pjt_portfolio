/*
 * uart1.c
 *
 * Created: 2026-06-22 오전 10:38:30
 *  Author: kccistc
 */ 
#include "uart1.h"
#include <stdio.h>

extern void UART0_transmit(uint8_t data);

void init_uart1();
void UART1_transmit(uint8_t data);

volatile uint8_t bt_data;
//extern int func_state;
//extern void (*fp[]) ();
//extern volatile char scm[50];



// p.278 표 12-3
// bluetooth(bt)로부터 1byte 들어오면 자동으로 이곳에 진입

ISR(USART1_RX_vect)
{
	// interrupt 쓰는 변수는 volatile로 해야함
	// UDR0를 data에 복사하는 순간 UDR0의 내용은 없어지므로, 데이터가 '이동'한다고 생각해도 무방함
	bt_data = UDR1;
	// UART0_transmit('[');
	UART1_transmit(bt_data);	// BT로부터 들어온 byte를 확인하기 위해 comport로 출력
	//UART0_transmit(']');
	
	
}

/*
1. 전송속도 : 9600bps
2. start/stop 설정
3. RX(수신): interrupt 로 설정

*/
void init_uart1(void){
	UBRR1H = 0x00;
	UBRR1L = 207;			// 9600bps, 표 8-9
	UCSR1A |= 1 << U2X1;	// 2배속 설정
	// UART1 송신, 수신이 모두 가능하도록 RX Interrupt가 가능하도록 설정.
	UCSR1B |= 1 << RXEN1 | 1 << TXEN1 | 1 << RXCIE1; // RXCIE: RX complete interrupt enable
}

// UART1로 1byte 전송하는 함수
void UART1_transmit(uint8_t data)
{
	while( !(UCSR1A & (1 << UDRE1))); // no operation -> data가 송신중이면 송신이 끝날 때까지 기다림

	UDR1 = data;			// HW 전송 register에 data를 송신한다.
	
}