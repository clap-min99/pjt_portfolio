#include "device_driver.h"
#define PWM_DEFAULT 60
void Motor_CW(int gear)
{
    Motor_Stop();
    TIM3_Delay(500);
    Macro_Write_Block(GPIOA->MODER, 0x3, 0x1, 2);
	Macro_Clear_Bit(GPIOA->ODR, 1);
    TIM5_CW_PWM(10000, PWM_DEFAULT+(gear-1)*5);
   
}

void Motor_CCW(int gear)
{
  
    Motor_Stop();
    TIM3_Delay(500);
    Macro_Write_Block(GPIOA->MODER, 0x3, 0x1, 0);
    Macro_Clear_Bit(GPIOA->ODR, 0);   
	TIM5_CCW_PWM(10000, PWM_DEFAULT+(gear-1)*5);
    
}


void Motor_Stop(void)
{
    Macro_Write_Block(GPIOA->MODER,0xf,0x5,0);
    Macro_Clear_Area(GPIOA->ODR, 0x3,0);
}
