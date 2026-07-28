// #include "device_driver.h"
// #include <stdio.h>

// #define ST_FORWARD   0
// #define ST_BACKWARD  1
// #define ST_STOP      2
// #define ST_END       3

// #define LONG_MS      3000
// #define DCLK_MS      500

// static int      key_hold     = 0;     // 현재 눌려있는지 여부
// static unsigned press_time     = 0;     // 누른 시간
// static unsigned release_time = 0;     // 뗀 시간
// static int      click_cnt    = 0;     // 

// volatile int gear = 1;

// void Event_Handler(void)
// {
//     if(Uart_Data_In)
// 	{
//         printf("Motor gear box = %c\n", Uart_Data);
//         gear = Uart_Data - '0'; 
//         Uart_Data_In = 0;

//         #if 0
//         printf("Motor gear box = %c\n", Uart_Data);
//         gear = Uart_Data - '0';
// 		if(motor_state == 0)
//     	{
//         	Motor_CW(gear);
//    		}
//     	else if(motor_state == 1)
//     	{
//         	Motor_CCW(gear);
//    		}
//    		else if(motor_state == 2)
//     	{
//        		Motor_Stop();
//     	}
//         Uart_Data_In = 0;
//         #endif

// 	}
//     if(Key_Pressed)
//     {
//         Key_Pressed=0;
//     }	
//     if(TIM4_Expired)
//     {
//         TIM4_Expired = 0;
//     }
//     if(TIM2_Expired)
//     {
//         TIM2_Expired = 0;
//     }
    

// }

