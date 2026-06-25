# 🫧 WasherFSM — 세탁기 시뮬레이터

ATmega128A 기반 세탁기 FSM. 버튼으로 세탁·헹굼·탈수 시간을 설정하고, FND에 남은 시간을 표시하며 모터(팬)로 각 동작을 표현합니다.

---

## 📹 동작 영상

![washer](./assets/washer.gif)

---

## 🔧 기술 스택

| 분류 | 내용 |
|------|------|
| MCU | ATmega128A |
| 언어 | C (AVR-GCC) |
| IDE | Atmel Studio 7 |
| 구동 | DC 모터 (팬) + L298N 드라이버 |
| 표시 | FND(7세그) × 1 |
| 입력 | 버튼 × 3 (BTN0 / BTN1 / BTN2) |
| 타이머 | Timer0 (1ms 기준 카운터) |

---

## 📐 하드웨어 핀맵

### 버튼
```
BTN0 → 실행 중 → 대기 모드 (취소/리셋)
BTN1 → 설정 모드 이동 / 다음 설정으로 진행
BTN2 → 시간 증가
```

### FND (남은 시간 표시)
```
PORTC → 세그먼트 A~DP
PORTB → 자릿수 D1~D4
```

### 모터 드라이버 (L298N)
```
PWM 핀 → 속도 제어
IN1/IN2 → 방향 제어 (세탁/헹굼 vs 탈수)
```

---

## 🗂️ 파일 구조

```
WasherFSM/
├── main.c        — 메인 루프, 초기화
├── washer.c/h    — 세탁기 FSM 상태 머신
├── fnd.c/h       — FND 멀티플렉싱 (남은 시간 표시)
├── pwm.c/h       — 모터 PWM 제어
├── button.c/h    — 버튼 입력 처리 (디바운싱)
└── timer.c/h     — Timer0 기반 1ms 카운터
```

---

## 🤖 세탁기 FSM

### 상태 다이어그램

```
전원 ON
   │
   ▼
STANDBY (대기)  ─── BTN1 ──► SET_WASH (세탁 시간 설정)
   ▲                              │ BTN2: 시간 증가
   │                              │ BTN1: 다음으로
   │                              ▼
   │                         SET_RINSE (헹굼 시간 설정)
   │                              │ BTN2: 시간 증가
   │                              │ BTN1: 다음으로
   │                              ▼
   │                         SET_SPIN (탈수 시간 설정)
   │                              │ BTN2: 시간 증가
   │                              │ BTN1: 시작!
   │                              ▼
   │               ┌────────── WASHING (세탁 중) ◄─ BTN0: 취소
   │               │               │ 시간 종료
   │               │               ▼
   │               │           RINSING (헹굼 중) ◄─ BTN0: 취소
   │               │               │ 시간 종료
   │               │               ▼
   │               │           SPINNING (탈수 중) ◄─ BTN0: 취소
   │               │               │ 시간 종료
   │               │               ▼
   └───────────────┴────────── STANDBY (완료 → 대기)
```

### 상태 정의

| 상태 | FND 표시 | 모터 | 비고 |
|------|----------|------|------|
| STANDBY | `----` | OFF | 초기 대기 |
| SET_WASH | 세탁 시간 (초) | OFF | BTN2로 시간 증가 |
| SET_RINSE | 헹굼 시간 (초) | OFF | BTN2로 시간 증가 |
| SET_SPIN | 탈수 시간 (초) | OFF | BTN2로 시간 증가 |
| WASHING | 남은 시간 카운트다운 | ON (정방향) | 세탁 동작 |
| RINSING | 남은 시간 카운트다운 | ON (정방향) | 헹굼 동작 |
| SPINNING | 남은 시간 카운트다운 | ON (고속) | 탈수 동작 |

---

## 💡 주요 코드

### 1. FSM 상태 구조

```c
typedef enum {
    STANDBY,
    SET_WASH,
    SET_RINSE,
    SET_SPIN,
    WASHING,
    RINSING,
    SPINNING
} WasherState;

WasherState current_state = STANDBY;
```

### 2. FSM 메인 루프

```c
void washer_main(void)
{
    switch (current_state)
    {
        case STANDBY:
            fnd_display_dash();           // "----" 표시
            if (get_button(BTN1, BTN1PIN))
                current_state = SET_WASH;
            break;

        case SET_WASH:
            fnd_display(wash_time);
            if (get_button(BTN2, BTN2PIN)) wash_time += 5;
            if (get_button(BTN1, BTN1PIN)) current_state = SET_RINSE;
            if (get_button(BTN0, BTN0PIN)) current_state = STANDBY;
            break;

        // SET_RINSE, SET_SPIN 동일 구조 ...

        case WASHING:
            fnd_display(remaining);
            motor_on(FORWARD);
            if (--remaining <= 0)    current_state = RINSING;
            if (get_button(BTN0, BTN0PIN)) { motor_off(); current_state = STANDBY; }
            break;

        // RINSING, SPINNING 동일 구조 ...
    }
}
```

### 3. 1ms 기준 타이머 (Timer0)

세탁 시간 카운트다운에 사용하는 1ms 기준 클럭입니다.

```c
// 초기화: 64분주, TCNT0=6 → 250 펄스 = 1ms
void init_timer0(void)
{
    TCNT0 = 6;
    TCCR0 |= (1 << CS02);            // 64분주
    TIMSK |= (1 << TOIE0);           // Overflow 인터럽트 활성화
}

// 1ms마다 호출
ISR(TIMER0_OVF_vect)
{
    TCNT0 = 6;
    msec_count++;
}
```

### 4. FND 멀티플렉싱

```c
const uint8_t seg_table[10] = {
    0b11000000, // 0
    0b11111001, // 1
    0b10100100, // 2
    // ...
};

void fnd_display(int value)
{
    int digits[4] = { value/1000, (value/100)%10, (value/10)%10, value%10 };
    for (int i = 0; i < 4; i++) {
        FND_DIGIT_PORT = ~(1 << digit_pin[i]);
        FND_DATA_PORT  = seg_table[digits[i]];
        _delay_ms(2);
        FND_DATA_PORT  = 0x00;
    }
}
```

---

## 📚 알아야 할 개념

### FSM (Finite State Machine)
- 시스템을 **상태(State)** 와 **전이(Transition)** 로 표현하는 설계 기법
- 복잡한 동작을 `switch-case`로 명확하게 구조화할 수 있음
- 각 상태에서 **어떤 입력이 들어오면 어디로 가는지**만 정의하면 됨

### Timer0 — 1ms 기준 클럭 생성 원리
```
16MHz / 64(분주) = 250,000Hz
TCNT0 = 6 → 6~256까지 250 카운트
250,000Hz / 250 = 1,000Hz → 1ms마다 오버플로우
```

### FND 멀티플렉싱
- 4자리 FND는 세그먼트 배선이 공유됨 → 동시에 켤 수 없음
- 자릿수를 빠르게 순서대로 ON/OFF (2ms 간격)
- 눈의 잔상 효과로 4자리가 동시에 켜진 것처럼 보임

### 버튼 디바운싱
- 기계식 버튼은 눌릴 때 수 ms 동안 ON/OFF를 반복하는 **채터링** 발생
- `static` 변수로 이전 상태를 기억해서 **눌렸다 떼인 순간에만** 1을 반환

```c
// 핵심 로직: 이전 상태 = PRESS, 현재 상태 = RELEASE → "클릭 완료"
if (prev == BUTTON_PRESS && current == BUTTON_RELEASE)
    return 1;
```

### volatile
인터럽트(ISR)와 메인 루프가 공유하는 변수에 필수.  
컴파일러가 최적화로 레지스터에 캐싱하는 것을 방지합니다.

```c
volatile uint32_t msec_count = 0;
```

---

## 🗒️ 개발 메모

- 각 설정 단계(SET_WASH 등)에서 BTN0을 누르면 STANDBY로 돌아가 처음부터 재설정 가능
- 실행 중 BTN0은 즉시 정지 + 모터 OFF + STANDBY 복귀
- FND `----` 표시는 세그먼트 G만 켜는 방식으로 구현
- 모터 속도: 세탁/헹굼보다 탈수 시 듀티비를 높여 고속 회전 표현
