/*
 * queue.h
 *
 * Created: 2026-06-29 오후 3:19:28
 *  Author: kccistc
 */ 


#ifndef QUEUE_H_
#define QUEUE_H_

#define TRUE 1
#define FALSE 0
#define QUEUE_MAX 100

int queue_full(void);
int queue_empty();
uint8_t read_queue();
void queue_init();
void insert_queue(uint8_t value);


#endif /* QUEUE_H_ */