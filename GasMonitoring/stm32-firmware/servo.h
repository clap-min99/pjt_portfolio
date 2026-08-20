#ifndef SERVO_H
#define SERVO_H

// 밸브 각도 정의 (0도: 개방, 90도: 차단)
#define SERVO_VALVE_OPEN_ANGLE 0
#define SERVO_VALVE_CLOSE_ANGLE 90

/**
 * @brief 서보 모터 GPIO(PA0 AF1) 초기화 및 초기 각도 설정
 */
void Servo_Init(void);

/**
 * @brief 서보 모터 목표 각도 제어 (0 ~ 180도)
 * @param angle 목표 각도 (0 ~ 180)
 */
void Servo_Set_Angle(unsigned char angle);

/**
 * @brief 밸브 상태에 따른 서보 모터 제어 (1: 90도 차단, 0: 0도 복구)
 * @param on 1: 90도 차단, 0: 0도 개방
 */
void Servo_Display(int on);

#endif // SERVO_H