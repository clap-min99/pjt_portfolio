#include "device_driver.h"

/**
 * @brief 시스템 클럭(SYSCLK)을 내부 HSI 기반 96MHz로 초기화
 *
 * [클럭 트리 구성]
 * - HSI (16MHz) / PLLM(8) * PLLN(192) / PLLP(4) = SYSCLK 96MHz
 * - AHB Prescaler = /1  -> HCLK  = 96MHz
 * - APB1 Prescaler = /2 -> PCLK1 = 48MHz (TIMx: 96MHz)
 * - APB2 Prescaler = /1 -> PCLK2 = 96MHz
 */
void Clock_Init(void)
{
    // 1. HSI 클럭 활성화 및 안정화 대기
    RCC->CR |= (1 << 0);
    while (!Macro_Check_Bit_Set(RCC->CR, 1))
        ;

    // 2. Flash 레이턴시(3 WS @ 96MHz) 및 프리페치/I-Cache/D-Cache 동시 활성화
    FLASH->ACR = (1 << 10) | (1 << 9) | (1 << 8) | (0x3 << 0);

    // 3. PLL 설정 (PLLM=8, PLLN=192, PLLP=4, PLLSRC=HSI)
    RCC->PLLCFGR = (8 << 24) | (0 << 22) | (1 << 16) | (192 << 6) | (8 << 0);

    // 4. PLL 활성화 및 Lock 대기
    Macro_Set_Bit(RCC->CR, 24);
    while (!Macro_Check_Bit_Set(RCC->CR, 25))
        ;

    // 5. APB1/APB2 분주비 설정 (PCLK1 = 48MHz, PCLK2 = 96MHz)
    RCC->CFGR = (0 << 13) | (4 << 10) | (0 << 4);

    // 6. 시스템 클럭 소스를 PLL로 전환 및 전환 완료 대기
    Macro_Write_Block(RCC->CFGR, 0x3, 0x2, 0);
    while (Macro_Extract_Area(RCC->CFGR, 0x3, 2) != 0x2)
        ;
}