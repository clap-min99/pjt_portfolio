#include "device_driver.h"
#include <stdio.h>

// 전역변수는 main 에 정의
unsigned int prv_time = 0;
// unsigned int pwm_default = 50;

volatile int gear = 1;  
volatile int Uart_Data_In=0;
volatile int motor_state = 2;
volatile int TIM2_Expired = 0; 
volatile int TIM4_Expired = 0; 
volatile unsigned char Uart_Data=0;
volatile unsigned int motor_dir= 0;
volatile unsigned int time_cnt = 0; 
volatile unsigned int Key_Pressed = 0;


static void Sys_Init(int baud) 
{
	SCB->CPACR |= (0x3 << 10*2)|(0x3 << 11*2); 
	Clock_Init();
	Uart2_Init(baud);
	setvbuf(stdout, NULL, _IONBF, 0);
	LED_Init();
	Key_Poll_Init();
	TIM5_Init();

}



void Main(void)
{
    Sys_Init(115200);
    Macro_Set_Bit(RCC->AHB1ENR, 0);
    Macro_Set_Bit(RCC->AHB1ENR, 2); 
	Uart2_RX_Interrupt_Enable(1);
    TIM2_ISR_EN();
	Key_ISR_EN();
    
	printf("PWM Motor Start\n");
    
    for(;;)
    {
		Op_Handler();
		
		
    }
}
