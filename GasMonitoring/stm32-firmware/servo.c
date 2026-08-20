#include "device_driver.h"
#include "servo.h"
#include "timer.h"

static unsigned char g_servo_current_angle = 0;

void Servo_Init(void)
{
    // GPIOA 클럭 활성화 (AHB1ENR Bit 0)
    Macro_Set_Bit(RCC->AHB1ENR, 0U);

    // PA0 Alternate Function(10b) 모드 설정
    Macro_Write_Block(GPIOA->MODER, 0x3, 0x2, 0);

    // PA0 -> TIM2_CH1(AF1, 0001b) 매핑
    Macro_Write_Block(GPIOA->AFR[0], 0xF, 0x1, 0);

    // 초기 상태: 밸브 개방 (0도)
    Servo_Set_Angle(SERVO_VALVE_OPEN_ANGLE);
}

void Servo_Set_Angle(unsigned char angle)
{
    if (angle > 180)
    {
        angle = 180;
    }

    g_servo_current_angle = angle;

    // 정석 변환식: 0도 = 500us, 180도 = 2500us (90도 = 1500us)
    unsigned short pulse_us = 500 + ((unsigned short)angle * 2000 / 180);
    TIM2_PWM_Set_Pulse(pulse_us);
}

void Servo_Display(int on)
{
    if (on)
    {
        Servo_Set_Angle(SERVO_VALVE_CLOSE_ANGLE); // 90도 (밸브 차단)
    }
    else
    {
        Servo_Set_Angle(SERVO_VALVE_OPEN_ANGLE); // 0도 (밸브 개방)
    }
}