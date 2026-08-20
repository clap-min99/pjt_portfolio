#include "device_driver.h"
#include "timer.h"
#include "adc.h"
#include "led.h"
#include "motor.h"
#include "key.h"
#include "servo.h"
#include <stdio.h>

extern volatile unsigned long g_sys_tick;
extern volatile unsigned char g_rx_cmd;  // Qt 수신 명령 ('1': 차단, '0': 복구)
extern volatile unsigned char g_rx_flag; // 명령 수신 플래그

static unsigned char g_valve_state = 0; // 밸브 상태 (1: 차단/환기, 0: 정상/개방)

// 밸브 상태 변경 및 모든 액추에이터(LED + DC모터 + 서보모터) 일괄 동기화
static void Valve_Set_State(unsigned char state)
{
    if (g_valve_state == state)
    {
        return;
    }

    g_valve_state = state;
    LED_Display(state);   // 경보 LED 제어
    Motor_Display(state); // 환기 팬(DC 모터) 제어
    Servo_Display(state); // 물리 밸브(서보 모터 90°/0°) 제어
}

// 수신된 Qt 명령어 디코딩 처리
static void Process_Command(unsigned char cmd)
{
    if (cmd == '1')
    {
        Valve_Set_State(1);
    }
    else if (cmd == '0')
    {
        Valve_Set_State(0);
    }
}

static void Sys_Init(int baud)
{
    // FPU(Coprocessor CP10, CP11) Full Access 활성화
    SCB->CPACR |= (0x3 << 10 * 2) | (0x3 << 11 * 2);

    Clock_Init();
    Uart2_Init(baud);
    setvbuf(stdout, NULL, _IONBF, 0);
}

void Main(void)
{
    unsigned short adc_val;
    unsigned long last_sensor_tick = 0L;
    unsigned long last_key_tick = 0L;

    Sys_Init(115200);
    printf("\n=== Smart Monitoring System ===\n");

    Timer_Init();
    ADC1_Init();
    LED_Init();
    Motor_Init();
    Key_Init();
    Servo_Init();

    Uart2_RX_Interrupt_Enable(1);

    // 초기 상태: 정상 개방 (LED 소등, 팬 정지, 밸브 0도 개방)
    Valve_Set_State(0);

    printf("System Ready!\n");

    for (;;)
    {
        // 1. Qt 원격 제어 명령 비동기 처리
        if (g_rx_flag)
        {
            Process_Command(g_rx_cmd);
            g_rx_flag = 0;
        }

        // 2. Non-blocking 30ms 주기로 물리 스위치 입력 처리
        if ((g_sys_tick - last_key_tick) >= 30)
        {
            last_key_tick = g_sys_tick;

            KeyEvent evt = Key_Scan();
            if (evt == KEY_EVENT_VALVE_CLOSE)
            {
                Valve_Set_State(1);
            }
            else if (evt == KEY_EVENT_VALVE_OPEN)
            {
                Valve_Set_State(0);
            }
        }

        // 3. Non-blocking 200ms 주기로 가스 센서 데이터 전송
        if ((g_sys_tick - last_sensor_tick) >= 200)
        {
            last_sensor_tick = g_sys_tick;

            adc_val = ADC1_Read();
            printf("%d\n", adc_val);
        }
    }
}