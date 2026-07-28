# MotorControl — DC 모터 제어

STM32F411xE 기반 DC 모터 제어 프로젝트. 버튼 클릭 방식(단클릭/더블클릭/롱클릭)으로 모터 방향과 상태를 제어하고, UART로 속도(기어)를 변경합니다.

---

## 동작 영상

-

---

## 기술 스택

| 분류 | 내용 |
|------|------|
| MCU | STM32F411xE (Nucleo-64) |
| 언어 | C (ARM-GCC) |
| IDE | STM32CubeIDE |
| 라이브러리 | STM32 HAL |
| 통신 | UART2 (속도 제어) |
| 구동 | DC 모터 + PWM (TIMER5) |
| 입력 | 버튼 (C13) |
| 표시 | LED (A5) |

---

## 하드웨어 핀맵

```
A0  → MOTOR CCR Ch1 (TIMER5, PWM)
A1  → MOTOR CCR Ch2 (TIMER5, PWM)
A2  → UART2 TX
A3  → UART2 RX
A5  → LED (LD2)
C13 → BUTTON
```

---

## 파일 구조

```
MotorControl/
├── README.md
├── HighLevelDesign/
│   └── README.md          — 상위설계서 (State / Peripheral / Function / Variable)
└── Core/
    ├── main.c
    ├── timer.c / timer.h
    ├── key.c   / key.h
    ├── uart.c  / uart.h
    ├── motor.c / motor.h
    ├── led.c   / led.h
    └── operation.c / operation.h
```

---

## State Machine

```
reset
  │
  ▼
INIT (주변장치 초기화, LED ON)
  │
  ▼
STOP ◄─────────────────────────── double_clk
  │
  ├─ one_clk ──► FORWARD (정방향 CW)
  │                  │
  │              one_clk
  │                  ▼
  │              BACKWARD (역방향 CCW)
  │                  │
  │              one_clk
  │                  ▼
  └──────────────── STOP
  │
  └─ long_clk ──► END (LED 3번 깜빡임 후 OFF)
```

| STATE | 모터 동작 | 진입 조건 |
|-------|-----------|-----------|
| INIT | - | reset |
| STOP | 정지 | double_clk / 초기 |
| FORWARD | 정방향 (A1:0, A0:1) | one_clk |
| BACKWARD | 역방향 (A1:1, A0:0) | one_clk |
| END | 정지 + LED OFF | long_clk |
| SPEED_CTRL | 속도 변경 (기어 1~9단) | UART 수신 |

---

## 주요 코드

### 1. 버튼 클릭 판별 (TIMER2 + 인터럽트)

버튼 falling edge에서 TIMER2 시작, 경과 시간으로 클릭 종류를 판별

```c
void Key_Handler(unsigned int time_cnt)
{
    if (time_cnt >= LONG_CLK_THRESHOLD)       // 롱클릭: 3초 이상
        motor_state = END;
    else if (Key_Pressed == DOUBLE_CLK)       // 더블클릭
        motor_state = STOP;
    else                                       // 단클릭: CW ↔ CCW 토글
        motor_dir ^= 1;
}
```

### 2. PWM 속도 제어 (TIMER5)

기어 단수(1~9)에 따라 듀티비를 결정

```c
// gear default: 50 + 5 × n (n = 1~9)
// gear 1 → duty 55%, gear 9 → duty 95%

void Motor_CW(int gear)
{
    int duty = 50 + 5 * gear;
    TIM5_CW_PWM(freq, duty);     // A0 채널 PWM ON, A1 OFF
}

void Motor_CCW(int gear)
{
    int duty = 50 + 5 * gear;
    TIM5_CCW_PWM(freq, duty);    // A1 채널 PWM ON, A0 OFF
}
```

### 3. UART 속도 변경

```c
void Uart_Handler(void)
{
    if (Uart_data_in) {
        // 수신 문자 '1'~'9' → gear 변경
        gear = (int)(rx_data - '0');
        Uart_data_in = 0;
    }
}
```

### 4. Op_Handler — 메인 루프 상태 처리

```c
void Op_Handler(void)
{
    switch (motor_state)
    {
        case INIT:     LED_On();                           break;
        case STOP:     Motor_Stop();                       break;
        case FORWARD:  Motor_CW(gear);                     break;
        case BACKWARD: Motor_CCW(gear);                    break;
        case END:      LED_Blink(3); LED_Off(); break;
    }
    Uart_Handler();
}
```

---

## 요소 기술

### TIMER 활용 3종

| 타이머 | 역할 | 방식 |
|--------|------|------|
| TIMER2 | 버튼 누름 시간 측정 (롱클릭 판별) | 인터럽트 (IRQ28) |
| TIMER3 | delay(ms) 구현 | 폴링 |
| TIMER5 | 모터 PWM 속도 제어 | PWM 출력 (CCR Ch1/Ch2) |

### PWM으로 모터 방향 제어

L298N 계열 드라이버 없이 TIMER5의 두 채널(Ch1=A0, Ch2=A1)로 직접 방향 제어합니다.

```
정방향(CW)  : Ch1 PWM ON  / Ch2 LOW
역방향(CCW) : Ch1 LOW     / Ch2 PWM ON
정지        : Ch1 LOW     / Ch2 LOW
```

### 버튼 클릭 이벤트 분류

```
falling edge 감지 (C13, IRQ40)
    │
    └─ TIMER2 시작
           │
    rising edge or timeout
           │
    ├─ < 200ms 후 또 클릭 → double_clk
    ├─ 200ms ~ 3s          → one_clk
    └─ 3s 이상             → long_clk
```

### volatile 전역변수

인터럽트(ISR)와 메인 루프가 공유하는 변수는 반드시 `volatile` 선언합니다.

```c
volatile unsigned int motor_state;
volatile unsigned int gear;
volatile unsigned int Key_Pressed;
```

---

## 상위설계서

[HighLevelDesign/README.md](./상위설계서/README.md)

---

## 개발 메모

- ATmega128A 프로젝트와 달리 STM32는 HAL 라이브러리로 레지스터 직접 접근 없이 타이머/UART 초기화
- TIMER5 CCR 채널 두 개로 H-브리지 방향 제어 — 별도 모터 드라이버 IC 없음
- 기어 1단(duty 55%) ~ 9단(duty 95%) 범위로 저속에서도 모터가 안정적으로 회전하도록 오프셋 50% 적용
