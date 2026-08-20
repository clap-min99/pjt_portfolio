#ifndef OPTION_H
#define OPTION_H

// ----------------------------------------------------
// 시스템 클럭 주파수 설정 (Hz)
// ----------------------------------------------------
#define SYSCLK (96000000U)
#define HCLK (SYSCLK)
#define PCLK2 (HCLK)
#define PCLK1 (HCLK / 2U)
#define TIMXCLK ((HCLK == PCLK1) ? (PCLK1) : (PCLK1 * 2U))

// ----------------------------------------------------
// STM32F411 SRAM 메모리 맵 (128KB: 0x20000000 ~ 0x20020000)
// ----------------------------------------------------
#define RAM_START (0x20000000U)
#define RAM_END (0x20020000U)

// 힙 및 스택 영역 정의 (8-byte 정렬)
#define HEAP_BASE (((unsigned int)&__ZI_LIMIT__ + 0x7) & ~0x7)
#define HEAP_SIZE (4 * 1024U) // 4KB 힙 할당
#define HEAP_LIMIT (HEAP_BASE + HEAP_SIZE)

#define STACK_LIMIT (HEAP_LIMIT + 8U)
#define STACK_BASE (RAM_END + 1U)
#define STACK_SIZE (STACK_BASE - STACK_LIMIT)

#endif // OPTION_H