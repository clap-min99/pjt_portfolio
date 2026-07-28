# MotorControl — DC 모터 제어

STM32F411xE 기반 DC 모터 제어 프로젝트. 버튼 클릭 방식(단클릭/더블클릭/롱클릭)으로 모터 방향과 상태를 제어하고, UART로 속도(기어)를 변경

---

## 동작 영상

-

---

## 기술 스택

| 분류 | 내용 |
|------|------|
| MCU | STM32F411xE (Nucleo-64) |
| 언어 | C (ARM-GCC) |
| IDE | VSCode |
| 빌드 | Makefile + arm-gcc (make / make run) |
| 레지스터 접근 | CMSIS (HAL 없이 직접 접근) |
| 통신 | UART2 (속도 제어) |
| 구동 | DC 모터 + L293D 드라이버 + PWM (TIMER5, PA0/PA1) |
| 전원 | CP2102 USB-UART 모듈 (모터 전원 공급) |
| 입력 | 버튼 (C13) |
| 표시 | LED LD2 (PA5) |

---

## 하드웨어 핀맵

```
PA0 → MOTOR CW  PWM (TIMER5 CH1, AF02)
PA1 → MOTOR CCW PWM (TIMER5 CH2, AF02)
PA2 → UART2 TX  (AF07)
PA3 → UART2 RX  (AF07)
PA5 → LED LD2
PC13 → BUTTON (EXTI13, falling/rising edge, IRQ40)
```

---

## 파일 구조

```
MotorControl/
├── README.md
├── clock.c          — PLL 설정, 시스템 클럭 초기화
├── main.c           — 전역변수 정의, Sys_Init, Main 루프
├── timer.c / .h     — TIM2(버튼 시간 측정), TIM3(Delay), TIM4(SW), TIM5(PWM)
├── key.c   / .h     — 버튼 GPIO 초기화, EXTI 인터럽트 설정
├── uart.c  / .h     — UART2 초기화, 송수신, 수신 인터럽트
├── motor.c / .h     — Motor_CW, Motor_CCW, Motor_Stop
├── led.c   / .h     — LED_Init, LED_On, LED_Off
├── operation.c / .h — Op_Handler, Key_Handler, Uart_Handler
├── device_driver.h  — 전역변수 extern, 함수 프로토타입 통합 헤더
├── crt0.s           — 스타트업 코드 (벡터 테이블, 스택 초기화)
└── Makefile
```

---

## State Machine

```
reset
  │
  ▼
ST_STOP (초기 정지, motor_state=2)
  │
  ├─ 단클릭 ──► ST_FORWARD (정방향 CW)
  │                 │
  │             단클릭
  │                 ▼
  │             ST_BACKWARD (역방향 CCW)
  │                 │
  │             단클릭
  │                 ▼
  │             ST_FORWARD ...
  │
  ├─ 더블클릭 ──► ST_STOP (실행 중 정지)
  │
  └─ 롱클릭(3초) ──► ST_END (LED 3번 깜빡임 → OFF)
                          │
                      단클릭
                          ▼
                      ST_STOP (재시작)
```

| STATE | 값 | 모터 동작 | 진입 조건 |
|-------|----|-----------|-----------|
| ST_FORWARD | 0 | 정방향 CW (PA0 PWM) | 단클릭 |
| ST_BACKWARD | 1 | 역방향 CCW (PA1 PWM) | 단클릭 |
| ST_STOP | 2 | 정지 (PA0, PA1 LOW) | 더블클릭 / 초기 |
| ST_END | 3 | 정지 + LED 3번 깜빡임 후 OFF | 롱클릭 3초 |

UART로 `'1'`~`'9'` 수신 시 현재 상태 유지한 채 기어(속도)만 변경

---


### 1. 버튼 클릭 판별 (operation.c)

EXTI(falling/rising edge) 인터럽트로 `Key_Pressed`를 1(press) / 2(release)로 세트하고, `Key_Handler()`가 메인 루프에서 시간 차로 클릭 종류를 판별합니다.

```c
void Key_Handler(unsigned int time_cnt)
{
    if (Key_Pressed == 1) {                      // 버튼 누름
        key_hold   = 1;
        press_time = time_cnt;
        Key_Pressed = 0;
    }
    else if (Key_Pressed == 2) {                 // 버튼 뗌
        key_hold = 0;
        if (time_cnt - press_time < 3000) {      // 3초 미만 → 클릭 카운트
            click_cnt++;
            release_time = time_cnt;
        }
        Key_Pressed = 0;
    }

    // 롱클릭 판정: 누른 채로 3초 경과
    if (key_hold && (time_cnt - press_time >= 3000)) {
        Motor_Stop();
        for (int i = 0; i < 3; i++) {
            LED_On();  TIM3_Delay(200);
            LED_Off(); TIM3_Delay(200);
        }
        motor_state = ST_END;
        key_hold = 0; click_cnt = 0;
        return;
    }

    // 더블클릭: 2번 이상
    if (click_cnt >= 2) {
        Motor_Stop();
        motor_state = ST_STOP;
        click_cnt = 0;
    }
    // 단클릭: 1번 + 500ms 대기 후 확정
    else if (click_cnt == 1 && !key_hold && (time_cnt - release_time >= 500)) {
        switch (motor_state) {
            case ST_STOP:     Motor_CW(gear);  motor_state = ST_FORWARD;  break;
            case ST_FORWARD:  Motor_CCW(gear); motor_state = ST_BACKWARD; break;
            case ST_BACKWARD: Motor_CW(gear);  motor_state = ST_FORWARD;  break;
            case ST_END:      LED_On();        motor_state = ST_STOP;     break;
        }
        click_cnt = 0;
    }
}
```

### 2. PWM 속도 제어 (motor.c + timer.c)

기어 1~9단에 따라 듀티비가 결정됩니다. 방향 전환 시 역기전력 방지를 위해 500ms 정지 후 PWM을 걸어요.

```c
#define PWM_DEFAULT 60
// gear 1 → duty 60%, gear 9 → duty 100%

void Motor_CW(int gear)
{
    Motor_Stop();
    TIM3_Delay(500);                                      // 역기전력 방지 딜레이
    Macro_Write_Block(GPIOA->MODER, 0x3, 0x1, 2);        // PA1 → GPIO 출력
    Macro_Clear_Bit(GPIOA->ODR, 1);                       // PA1 LOW 고정
    TIM5_CW_PWM(10000, PWM_DEFAULT + (gear-1) * 5);      // PA0 PWM
}

void Motor_Stop(void)
{
    Macro_Write_Block(GPIOA->MODER, 0xf, 0x5, 0);        // PA0, PA1 → GPIO 출력
    Macro_Clear_Area(GPIOA->ODR, 0x3, 0);                 // PA0, PA1 LOW
}
```

TIM5 PWM 설정 (레지스터 직접 접근):
```c
void TIM5_CW_PWM(unsigned short freq, int duty)
{
    Macro_Write_Block(GPIOA->MODER, 0x3, 0x2, 0);    // PA0 → AF 모드
    Macro_Write_Block(GPIOA->AFR[0], 0xf, 0x2, 0);   // PA0 → AF02 (TIM5)
    Macro_Write_Block(TIM5->CCMR1, 0xff, 0x60, 0);   // PWM mode 1
    TIM5->CCER  = (1 << 0);                           // CH1 출력 활성화
    TIM5->PSC   = (unsigned int)(TIMXCLK / TIM5_FREQ + 0.5) - 1;
    TIM5->ARR   = (unsigned int)((double)TIM5_FREQ / freq + 0.5) - 1;
    TIM5->CCR1  = TIM5->ARR * ((double)duty / 100.);
    Macro_Set_Bit(TIM5->EGR, 0);
    Macro_Set_Bit(TIM5->CR1, 0);
}
```

### 3. UART 수신 인터럽트 + 기어 변경 (uart.c + operation.c)

```c
// uart.c: RXNE 인터럽트 활성화
void Uart2_RX_Interrupt_Enable(int en)
{
    if (en) {
        Macro_Set_Bit(USART2->CR1, 5);   // RXNEIE ON
        NVIC_ClearPendingIRQ(38);
        NVIC_EnableIRQ(38);               // USART2 IRQ38
    }
}

// operation.c: 수신된 '1'~'9'로 기어 변경
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
```

### 4. TIM2 — 1ms 기준 타이머 (time_cnt)

```c
// TIM2 ISR이 1ms마다 time_cnt를 증가 → 버튼 누름 시간 측정에 사용
void TIM2_ISR_EN()
{
    TIM2->PSC = (unsigned int)(TIMXCLK / TIM2_FREQ + 0.5) - 1;  // 50KHz
    TIM2->ARR = TIM2_1ms_Pls;   // 1ms마다 오버플로우
    Macro_Set_Bit(TIM2->DIER, 0);
    NVIC_EnableIRQ(28);
    Macro_Set_Bit(TIM2->CR1, 0);
}
// ISR에서: time_cnt++
```

---

## 요소 기술

### 레지스터 직접 접근 

```c
// GPIO 출력 설정
Macro_Write_Block(GPIOA->MODER, 0x3, 0x1, 10);  // PA5 → Output
Macro_Set_Bit(GPIOA->ODR, 5);                    // PA5 HIGH (LED ON)

// UART 송신
while (!Macro_Check_Bit_Set(USART2->SR, 7));     // TXE 대기
USART2->DR = data;                               // 데이터 전송
```

### TIMER 활용

| 타이머 | 역할 | 방식 |
|--------|------|------|
| TIM2 | 1ms 기준 클럭 (`time_cnt++`) + 버튼 시간 측정 | 인터럽트 (IRQ28) |
| TIM3 | `TIM3_Delay(ms)` — 방향 전환 딜레이 등 | 폴링 (OPM) |
| TIM5 | 모터 PWM 출력 (CH1=CW, CH2=CCW) | PWM (AF02) |


### 버튼 클릭 이벤트 분류

EXTI가 falling/rising edge 둘 다 감지해서 누름/뗌을 구분하고, `time_cnt` 차이로 클릭 종류를 판별합니다.

```
falling edge (Key_Pressed=1) → press_time 기록
rising edge  (Key_Pressed=2) → 누름 시간 계산
    │
    ├─ 3초 이상 누름        → 롱클릭 → ST_END
    ├─ 500ms 내 2번 클릭    → 더블클릭 → ST_STOP
    └─ 500ms 대기 후 1번    → 단클릭 → 상태 전환
```

### volatile 전역변수

인터럽트(ISR)와 메인 루프가 공유하는 변수는 `volatile` 선언

```c
// main.c
volatile int motor_state = 2;     // ST_STOP으로 초기화
volatile unsigned int time_cnt = 0;
volatile unsigned int Key_Pressed = 0;
```

---

## 상위설계서

[./상위설계서/README.md](./상위설계서/README.md)

---

## 개발 메모

- 레지스터 직접 접근 (`GPIOA->ODR`, `TIM5->CCR1`, `USART2->DR` 등)
- 방향 전환 시 `Motor_Stop() + TIM3_Delay(500)` 으로 역기전력 방지
- 모터 전원 : CP2102 USB-UART 모듈로 공급
- TIMER5 CCR 채널 두 개로 H-브리지 방향 제어 — 별도 모터 드라이버 IC 없음
- 기어 1단(duty 55%) ~ 9단(duty 95%) 범위로 저속에서도 모터가 안정적으로 회전하도록 오프셋 50% 적용
