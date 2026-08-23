# 🗂️ Embedded Systems Portfolio

MCU 임베디드 프로젝트 

---

## 📁 프로젝트 목록

| 프로젝트 | 설명 | 핵심 기술 |
|----------|------|-----------|
| [GasMonitoring](./GasMonitoring/README.md) | 가스 누출 실시간 감지 및 원격 안전 차단 시스템 | Bare-metal FW, UART/TCP
| [AutoCar](./AutoCar/README.md) | 블루투스 수동조종 + 자율주행 RC카 | FSM, UART, 초음파, PWM, FND |
| [WasherFSM](./WasherFSM/README.md) | 버튼으로 세탁/헹굼/탈수 시간 설정하는 세탁기 시뮬레이터 | FSM, FND, PWM, Timer |
| [LCD_CAL](./LCD_CAL_RTC/README.md) | LCD화면에 계산기, 시계 출력하기 | LCD1602, DS1307 RTC,  I2C |
| [MotorControl](./MotorControl/README.md) | STM32F411xE 기반 DC 모터 방향/속도 제어 | 레지스터 제어, PWM, UART, EXTI |


---

## 개발 환경

- **MCU** : ATmega128A, STM32
- **IDE** : Atmel Studio 7, VSCode
- **언어** : C (AVR-GCC), C++, python
- **통신** : UART (PC 디버깅 / 블루투스)

---

## 📌 프로젝트 소개

#### SmartMonitoring
밀폐 공간의 가스 누출을 실시간으로 감지하고, 위험 시 밸브 차단·환기팬 구동·경보를 자율/원격으로 동시 수행하는 3계층 안전 관제 시스템.
STM32F411RE 베어메탈 엣지 노드(레지스터 직접 제어, PWM 서보 밸브 구동)가 UART로 Qt 데스크톱 게이트웨이와 통신하고, 게이트웨이는 다시 TCP로 Android 클라이언트와 연동해 CCTV 스트리밍 및 Tailscale VPN 기반 원격 제어를 지원.

---

### STM32F411xE

#### MotorControl
버튼 클릭 패턴(단클릭/더블클릭/롱클릭)으로 DC 모터의 정회전·역회전·정지를 제어하고, UART로 수신한 값에 따라 속도(기어)를 실시간 변경하는 프로젝트.
HAL 없이 레지스터 직접 접근으로 PWM(TIM5)과 EXTI 인터럽트를 구현

---

### Atemga128A

#### AutoCar
UART 통신을 활용(COMPortMaster, Bluetooth)하여 수동 조종, 스위치를 눌러 자율주행 모드로 전환되는 RC카. 
초음파 센서 3개(좌/정면/우)로 장애물을 감지하고 FSM으로 회피 주행

#### WasherFSM
버튼으로 세탁·헹굼·탈수 시간을 설정하고 순서대로 자동 실행되는 세탁기 시뮬레이터.  
FND로 남은 시간을 표시하고 모터(팬)로 동작을 표현

#### lcd_calculator
LCD1602와 4×4 키패드로 사칙연산을 처리하는 계산기
버튼 하나로 DS1307 RTC와 연동된 실시간 시계 화면으로 전환
