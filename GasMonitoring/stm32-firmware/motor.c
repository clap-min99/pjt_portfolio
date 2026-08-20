#include "device_driver.h"
#include "motor.h"

void Motor_Init(void)
{
    // GPIOC 클럭 활성화 (AHB1ENR Bit 2)
    Macro_Set_Bit(RCC->AHB1ENR, 2);

    // PC0, PC1 범용 출력 모드(01b) 설정
    Macro_Write_Block(GPIOC->MODER, 0x3, 0x1, 0); // PC0 [1:0]
    Macro_Write_Block(GPIOC->MODER, 0x3, 0x1, 2); // PC1 [3:2]

    // 출력 형태: Push-Pull (0)
    Macro_Clear_Bit(GPIOC->OTYPER, 0);
    Macro_Clear_Bit(GPIOC->OTYPER, 1);

    // 부팅 시 모터 오동작(Glitch) 방지를 위한 초기 정지 상태 고정
    Motor_Off();
}

void Motor_On(void)
{
    Macro_Set_Bit(GPIOC->ODR, 0);   // IN1 = HIGH
    Macro_Clear_Bit(GPIOC->ODR, 1); // IN2 = LOW
}

void Motor_Off(void)
{
    Macro_Clear_Bit(GPIOC->ODR, 0); // IN1 = LOW
    Macro_Clear_Bit(GPIOC->ODR, 1); // IN2 = LOW
}

void Motor_Display(int on)
{
    if (on)
    {
        Motor_On();
    }
    else
    {
        Motor_Off();
    }
}