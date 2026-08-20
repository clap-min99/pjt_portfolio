#ifndef KEY_H
#define KEY_H

#include "stm32f4xx.h"
#include <stdint.h>

// 스위치 입력 이벤트 정의
typedef enum
{
    KEY_EVENT_NONE = 0,
    KEY_EVENT_VALVE_CLOSE, // 긴급 차단 버튼 이벤트
    KEY_EVENT_VALVE_OPEN   // 정상 복구 버튼 이벤트
} KeyEvent;

void Key_Init(void);
KeyEvent Key_Scan(void);

#endif // KEY_H