/*
 * queue.c
 *
 * Created: 2026-06-29 오후 3:19:18
 *  Author: kccistc
 */ 

#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

#define TRUE 1
#define FALSE 0
#define QUEUE_MAX 100

int front=-1;   // read index
int rear=-1;    // insert index
uint8_t queue[QUEUE_MAX];

int queue_full(void)
{
	// queue에서 rear+1 % QUEUE_MAX의 값이 front와 같으면 queue full
	int tmp=(rear+1) % QUEUE_MAX;
	if (tmp == front)  // front와 같으면 queue full
		return TRUE;
	else return FALSE;	
}

int queue_empty()
{
	if (rear == front)  // front와 같으면 queue empty
		return TRUE;
	else return FALSE;	
}

uint8_t read_queue()
{
	if (queue_empty())
		printf("Queue is empty !!!\n");
	else
	{
		front = (front+1) % QUEUE_MAX;
		return (queue[front]);
	}
}

void queue_init()  // queue가 텅 빈경우 fron와 rear가 동일한 위치를 가리틴다.
{
	front=-1;   // read index
	rear=-1;    // insert index	
}

void insert_queue(uint8_t value)
{
	if (queue_full())
	{
		printf("queue full!!!!\n");
		return;
	}
	else   // save 
	{
		rear = (rear+1) % QUEUE_MAX;
		queue[rear]=value;
	}
}