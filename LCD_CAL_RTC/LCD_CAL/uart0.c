/*
 * uart0.c
 *
 * Created: 2026-06-16 오전 9:57:55
 *  Author: kccistc
 */ 

#include "uart0.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void init_uart0();
void UART0_transmit(uint8_t data);
// void pc_command_processing(t_ds1302 *p);
extern void write_ds1302(uint8_t addr, uint8_t data);

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
		if ((u_rear+1) % QUEUE_SIZE == u_front % QUEUE_SIZE)
		{
			return;	// queue full 상태
		}
		rx_buff[u_rear][i] = '\0';		// 문장의 끝인 NULL을 넣는다. 
		i = 0;							// 다음 string을 저장하기 위해 i를 0으로 만든다.
		u_rear = (u_rear + 1) % QUEUE_SIZE;	// 0~9
	}
	else 
	{
		if ((u_rear+1) % QUEUE_SIZE == u_front % QUEUE_SIZE)
		{
			return;	// queue full 상태
		}
		rx_buff[u_rear][i++] = data;	
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

//void pc_command_processing(t_ds1302 *p)
//{
	//if (front == rear) return;
	//
	//char cmd[QUEUE_LENGTH];
	//strcpy(cmd, (char*)rx_buff[front]);
	//front = (front + 1) % QUEUE_SIZE;
	//
	//// "setrtc" + 12자리 숫자 확인
	//if (strncmp(cmd, "setrtc", 6) != 0 || strlen(cmd) != 18)
	//{
		//printf("ERR: setrtc YYMMDDHHMMSS\n");
		//return;
	//}
	//
	//char *s = cmd + 6;   // "260629095600" 시작 포인터
	//
	//// 2자리씩 잘라서 파싱
	//char buf[3] = {0};
	//
	//strncpy(buf, s+0,  2); p->year    = atoi(buf);
	//strncpy(buf, s+2,  2); p->month   = atoi(buf);
	//strncpy(buf, s+4,  2); p->date    = atoi(buf);
	//strncpy(buf, s+6,  2); p->hour    = atoi(buf);
	//strncpy(buf, s+8,  2); p->minutes = atoi(buf);
	//strncpy(buf, s+10, 2); p->seconds = atoi(buf);
	//
	//// RTC에 쓰기
	//write_ds1302(ADDR_WRITEPROTECTED, 0x00);
	//write_ds1302(ADDR_YEAR,    p->year);
	//write_ds1302(ADDR_MONTH,   p->month);
	//write_ds1302(ADDR_DATE,    p->date);
	//write_ds1302(ADDR_HOUR,    p->hour);
	//write_ds1302(ADDR_MINUTES, p->minutes);
	//write_ds1302(ADDR_SECONDS, p->seconds);
	//
	//printf("SET: 20%02d-%02d-%02d %02d:%02d:%02d\n",
	//p->year, p->month, p->date,
	//p->hour, p->minutes, p->seconds);
//}