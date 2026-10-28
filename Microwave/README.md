# Microwave — Basys3 FPGA 전자레인지 제어

Basys3와 Verilog HDL로 시간 설정, 카운트다운, 도어 제어 및 모터·부저 출력을 구현한 전자레인지 동작 시뮬레이터입니다. 로터리 인코더로 시간을 설정하고, 문이 닫힌 상태에서 조리를 시작하면 남은 시간에 따라 DC 모터와 FND를 제어합니다.

[← 포트폴리오](../README.md)

## 기술 스택

| 분류 | 내용 |
|---|---|
| 보드 / FPGA | Basys3 / Artix-7 `xc7a35tcpg236-1` |
| 언어 / 도구 | Verilog HDL / Vivado 2020.2 |
| 클럭 | 100 MHz |
| 입력 | 로터리 인코더, 보드 버튼, 스위치 |
| 출력 | 4자리 FND, 부저, SG90 서보, L298N DC 모터 |
| 핵심 설계 | 입력 동기화·디바운싱, 쿼드러처 회전 검출, 카운트다운, PWM, FND 멀티플렉싱 |

## 주요 기능

| 기능 | 동작 |
|---|---|
| 시간 설정 | 로터리 회전으로 분 또는 초를 0~59 범위에서 증감; 로터리 버튼으로 선택 전환 |
| 설정 표시 | FND에 `MM.SS` 표시, 선택한 두 자리를 0.5초 간격으로 점멸 |
| 시작 / 취소 | 문이 닫히고 시간이 0보다 클 때 시작; 취소 시 시간 초기화 및 모터 정지 |
| 조리 동작 | 1초마다 남은 시간을 감소; FND에 시간과 원형 애니메이션을 1초씩 교대로 표시 |
| 종료 알림 | `00.00` 도달 시 모터 정지, FND 점멸과 부저 3회 후 설정 화면 복귀 |
| 문 제어 | SG90 서보로 열림·닫힘 명령. 조리 및 종료 알림 중 문 버튼 비활성화 |
| 모터 속도 | SW1·SW0에 따라 PWM 듀티 40%, 60%, 80%, 100% 선택 |

## 조작 방법

| 입력 | 동작 |
|---|---|
| SW15 | ON으로 전체 리셋, OFF로 리셋 해제 |
| 로터리 회전 | 선택된 분 또는 초 증감 |
| 로터리 버튼 | 분 ↔ 초 설정 전환; LED0 꺼짐은 분, 켜짐은 초 |
| BTNC | 문이 닫혀 있고 시간이 0보다 클 때 조리 시작 |
| BTNR | 조리 또는 종료 알림 취소, 시간 초기화 |
| BTNL | 설정 상태에서 문 열기 / 닫기 전환 |
| SW1·SW0 | `00`: 40%, `01`: 60%, `10`: 80%, `11`: 100% PWM |

## 시스템 구조

![시스템 블록다이어그램](assets/microwave_block_diagram.svg)

입력은 동기화와 디바운싱을 거쳐 시간 설정과 버튼 제어에 사용합니다. 시간 및 도어 상태에 따라 FND 표시, 부저, 서보 PWM, DC 모터 PWM을 제어합니다. 공통 클럭·리셋과 일부 신호는 그림에서 생략했습니다.

| 파일 | 역할 |
|---|---|
| [`src/top.v`](src/top.v) | 입력 동기화, 설정·카운트다운, 도어·부저·모터 PWM, 표시 선택 |
| [`src/rotary.v`](src/rotary.v) | 로터리 버튼 선택 상태와 기존 회전 상태 처리 |
| [`src/button_debounce_10ms.v`](src/button_debounce_10ms.v) | 입력 디바운싱 (`debouncer`) |
| [`src/tick_generator.v`](src/tick_generator.v) | 1ms tick 생성 |
| [`src/fnd_controller.v`](src/fnd_controller.v) | 4자리 스캔, 숫자·소수점 및 원형 애니메이션 출력 |
| [`sim/tb_microwave.v`](sim/tb_microwave.v) | 기능별 자동 검사 테스트벤치 |
| [`constraints/Basys3-Master_orginal.xdc`](constraints/Basys3-Master_orginal.xdc) | 보드와 PMOD 핀 제약 |

시간 제어의 쿼드러처 전이 검출은 `top.v`에 있으며, `rotary.v`의 버튼 토글 출력을 분·초 선택에 사용합니다. 블록도는 기능 단위의 표현이며 모든 블록이 별도 RTL 모듈은 아닙니다.

### 동작 상태

![전자레인지 동작 상태도](assets/microwave_fsm.svg)

| 상태 | 동작 및 전이 |
|---|---|
| SET | 분·초 설정 및 문 제어; 시간이 0보다 크고 문이 닫혀 있을 때 START → RUN |
| RUN | 카운트다운과 모터 구동; 0 도달 → FINISH, CANCEL → SET |
| FINISH | 모터 정지 및 3회 알림; 알림 완료 또는 CANCEL → SET |

도어 상태는 별도 저장값이며, 그림은 `running`과 `finished` 플래그의 동작을 상태로 정리한 것입니다. 물리 도어 센서는 없으므로 실제 문 위치가 아닌 서보에 전달한 명령 상태를 기준으로 시작을 제한합니다.

### PWM 및 시간 제어

- 서보: 20ms 주기(50Hz), 닫힘 1ms / 열림 2ms 펄스.
- DC 모터: 100MHz 기준 5,000클럭 주기(20kHz), 듀티 4단계. `running`이 0이면 ENA PWM을 끕니다.
- 부저: 약 2kHz 신호. 버튼 입력 시 100ms, 종료 시 FND 알림과 함께 3회 출력.
- FND: 1ms tick으로 자리 스캔. 설정 중 선택 자리는 0.5초 간격으로 점멸하고 RUN에서는 시간과 원형 표시가 1초마다 전환됩니다.

## 하드웨어 핀맵

| 신호 | Basys3 / 연결 대상 | 역할 |
|---|---|---|
| `clk`, `reset` | W5, R2 (SW15) | 100MHz 클럭, active-high 리셋 |
| `rotary_s1`, `rotary_s2`, `rotary_key` | JC2 M18, JC3 N17, JC4 P18 | 로터리 입력 |
| `btn_start`, `btn_cancel`, `btn_door` | U18 BTNC, T17 BTNR, W19 BTNL | 시작, 취소, 문 제어 |
| `motor_speed[0:1]` | V17 SW0, V16 SW1 | DC 모터 속도 선택 |
| `key_led` | U16 LED0 | 분·초 선택 표시 |
| `buzzer` | JC1 K17 | 부저 신호 |
| `servo_pwm` | JC7 L17 | SG90 신호 |
| `motor_pwm`, `motor_in1`, `motor_in2` | JA1 J1, JA2 L2, JA3 J2 | L298N ENA, IN1, IN2 |
| `seg[0:7]` | W7, W6, U8, V8, U5, V5, U7, V7 | FND a~g, dp |
| `an[0:3]` | U2, U4, V4, W4 | FND 자리 선택 |

L298N의 OUT1/OUT2에 DC 모터를 연결하고, ENA 점퍼를 제거한 뒤 PWM을 연결합니다. 드라이버와 Basys3의 GND는 공통으로 연결합니다. SG90의 빨간 선은 별도 전원, 갈색 선은 공통 GND, 노란 선은 JC7에 연결합니다. 전원 및 드라이버 점퍼 설정은 실제 부품의 사양에 맞게 확인해야 합니다.

## 검증 결과

Vivado XSim에서 [`tb_microwave.v`](sim/tb_microwave.v)를 실행해 **18개 검사 항목 통과**와 `ALL TESTS PASSED`를 확인했습니다. 초기화, 0초 시작 차단, 로터리 분·초 설정, 도어 시작 제한, 카운트다운·분 자리 내림, 취소 시 모터 정지, 종료 상태 및 알림 구간을 검사합니다.

시뮬레이션 시간을 줄이기 위해 테스트벤치는 디바운스 출력과 1초 카운터 값을 직접 제어합니다. 따라서 실제 디바운스 시간, 1초 주기, PWM 주파수·듀티의 시간 정확도와 종료 알림 전체 3회는 이 테스트로 자동 검증하지 않습니다. 대화 중 보드에서 주요 동작과 종료·취소 시 모터 정지를 확인했습니다.

## 트러블슈팅

로터리의 초기 연결에서 왼쪽 회전 시 값이 증가하고 오른쪽 회전 시 감소했습니다. `top.v`에서 회전 신호 순서를 바꿔 오른쪽 회전 시 증가하도록 수정한 뒤 보드에서 동작을 확인했습니다.

## Vivado 실행 방법

1. Basys3 (`xc7a35tcpg236-1`)를 대상으로 새 RTL 프로젝트를 생성합니다.
2. `src/`의 Verilog 파일 5개를 Design Sources로, `constraints/`의 XDC를 Constraints로 추가하고 `top`을 Top Module로 지정합니다.
3. 시뮬레이션 시 `sim/tb_microwave.v`를 Simulation Sources에 추가하고 `tb_microwave`를 Simulation Top으로 지정합니다.
4. Run Behavioral Simulation을 실행하고 콘솔의 `ALL TESTS PASSED`를 확인합니다.
5. 하드웨어 실행 시 합성·구현·비트스트림 생성을 진행하고 Basys3를 프로그래밍합니다.

## 개선 방향

- 실제 문 상태를 확인할 수 있는 센서 추가 및 인터록 보강.
- 시간, 도어, 부저, 모터 제어를 독립 RTL 모듈로 분리.
- 입력부터 출력까지 실제 타이밍을 검증하는 테스트와 PWM 주기·듀티 검증 추가.
- 실제 보드 동작 사진 또는 시연 영상 추가.
