#ifndef DEVICE_DRIVER_H
#define DEVICE_DRIVER_H

#include "stm32f4xx.h"
#include "option.h"
#include "macro.h"
#include "malloc.h"

// 시스템 및 클럭 초기화
extern void Clock_Init(void);

// UART2 드라이버 (Qt GUI 통신용)
extern void Uart2_Init(int baud);
extern void Uart2_RX_Interrupt_Enable(int en);

#endif // DEVICE_DRIVER_H