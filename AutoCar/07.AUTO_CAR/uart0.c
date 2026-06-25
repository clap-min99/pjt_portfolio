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
volatile unsigned char rx_buff[QUEUE_SIZE][QUEUE_LENGTH];  

volatile int rear = 0;
volatile int front = 0;

extern int func_state;
extern void forward(int speed);
extern void backward(int speed);
extern void turn_left(int speed);
extern void turn_right(int speed);
extern void stop();

extern volatile uint8_t current_dir;
extern volatile uint8_t current_speed;


ISR(USART0_RX_vect)
{
	volatile uint8_t data;
	volatile static int i = 0;
	
	data = UDR0;
	
	if (data == '\n' || data == '\r')
	{
		if ((rear+1) % QUEUE_SIZE == front % QUEUE_SIZE)
		{
			return;
		}
		rx_buff[rear][i] = '\0';
		i = 0;
		rear = (rear + 1) % QUEUE_SIZE;
	}
	else
	{
		if ((rear+1) % QUEUE_SIZE == front % QUEUE_SIZE)
		{
			return;
		}
		rx_buff[rear][i++] = data;
	}
}

void init_uart0(void){
	UBRR0H = 0x00;
	UBRR0L = 207;
	UCSR0A |= 1 << U2X0;
	UCSR0B |= 1 << RXEN0 | 1 << TXEN0 | 1 << RXCIE0;
}

void UART0_transmit(uint8_t data)
{
	while(!(UCSR0A & (1 << UDRE0)));
	UDR0 = data;
}

void pc_command_processing()
{
	if (front != rear)
	{
		if (strncmp((const char *)rx_buff[front], "f", 1) == 0) {
			forward(500);
			current_dir = 'F'; current_speed = 48;
		}
		else if (strncmp((const char *)rx_buff[front], "b", 1) == 0) {
			backward(500);
			current_dir = 'b'; current_speed = 48;
		}
		else if (strncmp((const char *)rx_buff[front], "l", 1) == 0) {
			turn_left(700);
			current_dir = 'r'; current_speed = 68;   
		}
		else if (strncmp((const char *)rx_buff[front], "r", 1) == 0) {
			turn_right(700);
			current_dir = 'L'; current_speed = 68;
		}
		else if (strncmp((const char *)rx_buff[front], "s", 1) == 0) {
			stop();
			current_dir = 'S'; current_speed = 0;
		}
		
		front = (front+1) % QUEUE_SIZE;
	}
}