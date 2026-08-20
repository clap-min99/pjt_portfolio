#ifndef LED_H
#define LED_H

#include "device_driver.h"

/**
 * @brief LED GPIO 초기화 (PB0: 상단, PB1: 하단, Push-Pull)
 */
void LED_Init(void);

/**
 * @brief LED 전체 점등 (PB0=HIGH, PB1=HIGH)
 */
void LED_On(void);

/**
 * @brief LED 전체 소등 (PB0=LOW, PB1=LOW)
 */
void LED_Off(void);

/**
 * @brief LED 상태 일괄 제어
 * @param on 1: 전체 점등, 0: 전체 소등
 */
void LED_Display(int on);

void LED_Top(int on);
void LED_Bottom(int on);

#endif // LED_H