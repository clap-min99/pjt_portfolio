# 🗂️ Embedded Systems Portfolio

ATmega128A 기반 임베디드 프로젝트 

---

## 📁 프로젝트 목록

| 프로젝트 | 설명 | 핵심 기술 |
|----------|------|-----------|
| [AutoCar](./AutoCar/README.md) | 블루투스 수동조종 + 자율주행 RC카 | FSM, UART, 초음파, PWM, FND |
| [WasherFSM](./WasherFSM/README.md) | 버튼으로 세탁/헹굼/탈수 시간 설정하는 세탁기 시뮬레이터 | FSM, FND, PWM, Timer |
| [LCD_CAL](./LCD_CAL_RTC/README.md) | LCD화면에 계산기, 시계 출력하기 | LCD1602, DS1307 RTC,  I2C |
| [MotorControl](./MotorControl/README.md) | STM32F411xE | 모터 제어 |  |


---

## 개발 환경

- **MCU** : ATmega128A
- **IDE** : Atmel Studio 7
- **언어** : C (AVR-GCC)
- **통신** : UART (PC 디버깅 / 블루투스)

---

## 📌 프로젝트 소개


### STM32F411xE

#### MotorControl
STM32F411xE 기반 모터 제어

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
