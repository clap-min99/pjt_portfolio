#ifndef MOTOR_H
#define MOTOR_H

#include "device_driver.h"

/**
 * @brief L298N 모터 드라이버 제어용 GPIO 초기화 (PC0: IN1, PC1: IN2)
 */
void Motor_Init(void);

/**
 * @brief 모터 구동 (PC0=HIGH, PC1=LOW: 정회전)
 */
void Motor_On(void);

/**
 * @brief 모터 정지 (PC0=LOW, PC1=LOW)
 */
void Motor_Off(void);

/**
 * @brief 모터 가동 상태 제어
 * @param on 1: 모터 회전, 0: 모터 정지
 */
void Motor_Display(int on);

#endif // MOTOR_H