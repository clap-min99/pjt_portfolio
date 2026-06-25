# 🗂️ Embedded Systems Portfolio

ATmega128A 기반 임베디드 프로젝트 

---

## 📁 프로젝트 목록

| 프로젝트 | 설명 | 핵심 기술 |
|----------|------|-----------|
| [AutoCar](./AutoCar/README.md) | 블루투스 수동조종 + 자율주행 RC카 | FSM, UART, 초음파, PWM, FND |
| [WasherFSM](./WasherFSM/README.md) | 버튼으로 세탁/헹굼/탈수 시간 설정하는 세탁기 시뮬레이터 | FSM, FND, PWM, Timer |

---

## 🔧 공통 개발 환경

- **MCU** : ATmega128A
- **IDE** : Atmel Studio 7
- **언어** : C (AVR-GCC)
- **통신** : UART (PC 디버깅 / 블루투스)
- **인터럽트 방식** : 타이머 오버플로우 + 외부 인터럽트(INT)

---

## 📌 프로젝트 소개

### 🚗 AutoCar
UART 통신을 활용(COMPortMaster, Bluetooth)하여 수동 조종, 버튼 하나로 자율주행 모드로 전환되는 RC카. 
초음파 센서 3개(좌/정면/우)로 장애물을 감지하고 FSM으로 회피 주행
→ [자세히 보기](./AutoCar/README.md)

### 🫧 WasherFSM
버튼으로 세탁·헹굼·탈수 시간을 설정하고 순서대로 자동 실행되는 세탁기 시뮬레이터.  
FND로 남은 시간을 표시하고 모터(팬)로 동작을 표현합니다.  
→ [자세히 보기](./WasherFSM/README.md)
