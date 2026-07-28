#include "device_driver.h"
#include <stdio.h>

#define ST_FORWARD   0
#define ST_BACKWARD  1
#define ST_STOP      2
#define ST_END       3

static int      key_hold     = 0;     // 현재 눌려있는지 여부
static unsigned press_time     = 0;     // 누른 시간
static unsigned release_time = 0;     // 뗀 시간
static int      click_cnt    = 0;     // 


void Op_Handler(void)
{
    unsigned int now = time_cnt;
    Key_Handler(now);
    Uart_Handler();

}

void Key_Handler(unsigned int time_cnt)
{
    if(Key_Pressed==1)
    {
        Key_Pressed=0;
        key_hold   = 1;
		press_time = time_cnt;
        printf("Key_Pressed==1");
    }

    else if (Key_Pressed == 2)         
	{
		Key_Pressed = 0;
		key_hold    = 0;

		if (time_cnt - press_time < 3000)  // 짧은 누름만 클릭 인정
		{
			click_cnt++;
			release_time = time_cnt;
		}
	}
    
    // 롱 판정
	if (key_hold && (time_cnt - press_time >= 3000))
	{
		key_hold  = 0;                
		click_cnt = 0;
		if (motor_state != ST_END)
        {
            Motor_Stop();
            

            // LED 3번 깜빡임
            for (int i = 0; i < 3; i++)
            {
                LED_On();  TIM3_Delay(200);
                LED_Off(); TIM3_Delay(200);
            }
            motor_state = ST_END;
        }
		return;
	}

	// 클릭 판단
    // 2번 클릭 시 정지
	if (click_cnt >= 2)
	{
		click_cnt = 0;
		if (motor_state == ST_FORWARD || motor_state == ST_BACKWARD)
        {
		    Motor_Stop();
           
            motor_state = ST_STOP;
        }
    }
    // 1번 클릭 + 한번 더 길게 누를 때 end
	else if (click_cnt == 1 && !key_hold && (time_cnt - release_time >= 500))
    {
        click_cnt = 0;
        switch (motor_state)
        {
            case ST_STOP:     Motor_CW(gear);  motor_state = ST_FORWARD;  break;
            case ST_FORWARD:  Motor_CCW(gear); motor_state = ST_BACKWARD; break;
            case ST_BACKWARD: Motor_CW(gear);  motor_state = ST_FORWARD;  break;
            case ST_END:      LED_On();        motor_state = ST_STOP;     break;  
        }
    }


}


void Uart_Handler(void)
{
    if (!Uart_Data_In) return;
	Uart_Data_In = 0;

	if (Uart_Data < '1' || Uart_Data > '9') return;

	gear = Uart_Data - '0';
	printf("gear=%d\n", gear);

	if      (motor_state == ST_FORWARD)  Motor_CW(gear);
	else if (motor_state == ST_BACKWARD) Motor_CCW(gear);
}

