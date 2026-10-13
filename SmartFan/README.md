# Smart Fan - Linux Device Driver Project

NVIDIA Jetson Orin Nano에서 초음파 센서와 로터리 엔코더 입력으로 팬을 제어하는 임베디드 Linux 팀 프로젝트입니다.
GPIO·IRQ 입력 드라이버, PWM·LED 출력 드라이버와 사용자 프로그램을 통합했으며, 실제 하드웨어 동작 시연과 시연영상 제작을 완료했습니다.

커널은 하드웨어 접근과 데이터 전달을, 사용자 프로그램은 이벤트 판단과 상태 전환을 담당합니다.
장치와의 통신은 Character Device의 `read()` / `write()`를 사용합니다.

## 구현한 기능

- 초음파 센서: Trigger 펄스 생성, Echo 상승·하강 IRQ로 거리 측정, Completion으로 측정 완료 동기화
- 로터리 엔코더: A/B 위상 변화로 회전량 누적, 버튼 입력 전달, 공유 데이터 spinlock 보호
- 수동 제어: 버튼 ON/OFF, 좌우 회전으로 3단계 풍량 조절
- 자동 제어: 20cm 이내 접근 시 기동, 멀어지거나 센서 측정 실패 시 정지
- 모터 출력: Linux PWM Framework로 상태별 Duty 적용
- LED 표시: 팬 상태에 따라 0/2/5/8개 점등
- 통합 실행: 상태 변경 시 출력 갱신, 시작·종료 시 OFF 요청, SIGINT/SIGTERM 종료 처리

### 팬 상태와 출력

| 상태 | 모터 명령값 (`speed`) | PWM Duty | LED 명령값 (`mode`) | 점등 개수 |
|---|---|---|---|---|
| OFF | 0 | 0% | 0 | 0개 |
| S1 | 2 | 60% | 1 | 2개 |
| S2 | 5 | 80% | 2 | 5개 |
| S3 | 8 | 100% | 3 | 8개 |

풍량은 OFF와 3개의 동작 단계로 구성됩니다. `2/5/8`은 논리 명령값이며 PWM 비율이나 측정 RPM을 의미하지 않습니다.
현재 PWM 주기는 1ms (1kHz)입니다.

## 시스템 구조

```text
USER SPACE
  Main → Event Handler → State Machine → Output Controller
           ↑ read()                         ↓ write()
KERNEL SPACE
  ultrasonic_drv                      fan_pwm_drv / fan_led_drv
  /dev/ultrasonic, /dev/fan_encoder    /dev/fan_pwm, /dev/fan_led
           ↑ GPIO / IRQ                     ↓ PWM / GPIO
HARDWARE
  초음파 센서 / 로터리 엔코더          모터 드라이버 / DC 모터 / LED 바
```

Event는 입력을 이벤트로 변환하고, State는 `mode / power / speed`를 함께 관리합니다.
Output은 모터에 `speed`, LED에 `mode`를 전달합니다. 엔코더와 초음파 장치는 하나의 `ultrasonic_drv` 모듈에서 제공합니다.

## 개발 환경과 하드웨어

| 항목 | 구성 |
|---|---|
| 보드 | NVIDIA Jetson Orin Nano Developer Kit |
| OS / 커널 | Jetson Linux / 보드 기록 기준 5.15.199-tegra |
| 언어 / 빌드 | C, GCC, Make, Kbuild; 보조 도구 Python 3 |
| 초음파 센서 / 엔코더 | HC-SR04 / EC11 (발표 자료 기준) |
| 모터 / 드라이버 | EZmotor R300 / HW-095 |
| LED | WCNLB8-SR12 8구 LED 바 |
| 통신 | Character Device (`/dev`), 바이너리 `read()` / `write()` |

### 배선 요약

아래 번호는 Jetson 40핀 헤더의 **물리 핀 번호**입니다.

| 기능 | 물리 핀 |
|---|---|
| 초음파 Trigger / Echo | 11 / 36 |
| 엔코더 S1 / S2 / KEY | 13 / 18 / 22 |
| 모터 ENA (PWM) / IN1 / IN2 | 33 / 29 / GND |
| LED1 → LED8 | 7 / 12 / 19 / 16 / 21 / 23 / 37 / 31 |

커널 코드의 global GPIO 번호는 현재 보드 설정을 기준으로 합니다. 다른 환경에서는 GPIO base와 Pinmux를 확인해야 합니다.
PWM ENA는 `32c0000.pwm` 컨트롤러를 사용하며, 로드 스크립트는 Pinmux를 변경하지 않습니다.
상세 배선과 전원 구성, 회로도에서 미확정으로 표시한 항목은 아래 문서를 참고하세요.

- [하드웨어 설정과 보드 기록](docs/hardware.md)
- [KiCad 회로도 설명](hardware/kicad/README.md)
- [회로도 PDF](hardware/kicad/SmartFan.pdf)
- [KiCad 회로도 원본](hardware/kicad/SmartFan.kicad_sch)

## 빌드와 실행

Jetson/Linux에서 GCC, Make, Python 3와 타깃 커널에 맞는 빌드 헤더를 준비합니다.
아래 명령은 포트폴리오 저장소의 `SmartFan/` 디렉터리에서 실행합니다.

### 1. 빌드

```sh
cd SmartFan             # 포트폴리오 저장소 루트에서 이동
make                    # build/smart-fan 사용자 프로그램
make test               # 장치 없이 실행하는 모의 테스트
make kernel             # 센서·엔코더 / PWM / LED 커널 모듈

# 타깃 커널 빌드 경로를 별도로 지정할 경우
make kernel KDIR=/path/to/target/kernel/build
```

### 2. 모듈 로드

앱이 종료되어 있고 해당 모듈이 아직 로드되지 않은 상태에서 실행합니다.

```sh
sudo insmod kernel/ultrasonic/ultrasonic_drv.ko
sudo python3 scripts/load_motor.py
sudo insmod kernel/fan_led/fan_led_drv.ko

ls -l /dev/ultrasonic /dev/fan_encoder /dev/fan_pwm /dev/fan_led
```

`load_motor.py`는 Linux 5.15의 sysfs에서 물리33번에 해당하는 PWM ID를 찾아 모터 모듈을 로드합니다.
기존 모터 모듈이 있으면 재로드하며, 사용 중이면 중단합니다. ID만 확인하려면 `python3 scripts/load_motor.py --print-id`를 사용합니다.
센서·LED 모듈을 다시 로드할 때는 앱 종료 후 기존 모듈을 먼저 해제합니다.

엔코더 방향이 반대면 센서 모듈 로드 시 `reverse=1`을 지정합니다.
한 칸당 인식은 `transitions=4`가 기본이며 보드 특성에 따라 `transitions=2`를 사용할 수 있습니다.

### 3. 앱 실행

```sh
sudo ./build/smart-fan                  # 수동 엔코더 모드
sudo ./build/smart-fan --auto           # 초음파 자동 모드
sudo ./build/smart-fan --dry-run        # 수동 입력·상태 확인, 출력 장치 접근 없음
sudo ./build/smart-fan --auto --dry-run # 자동 입력·상태 확인, 출력 장치 접근 없음
```

종료는 `Ctrl+C`입니다. `--dry-run`도 입력 장치는 필요하지만 모터·LED 출력 장치를 열지 않습니다.
기존 `fan_distance` 실행 파일 대신 새로 빌드한 `build/smart-fan`을 사용합니다.

## 조작 방법

### 수동 모드 (기본)

| 입력 | 동작 |
|---|---|
| 버튼 짧게 눌렀다 떼기 | OFF → S1, 동작 중 → OFF |
| 버튼 2초 이상 누르기 | 모든 상태에서 OFF |
| 엔코더 오른쪽 회전 | S1 → S2 → S3 |
| 엔코더 왼쪽 회전 | S3 → S2 → S1 |

양 끝에서는 단계가 유지되고, OFF에서 회전 입력은 무시합니다. 버튼 debounce는 30ms입니다.
긴 누름 후 버튼을 떼어도 다시 ON되지 않습니다.
`EVENT_SHORT_PRESS`의 순환 전환은 State API 호환용으로 남아 있으며, 실제 엔코더의 짧은 버튼 입력은 `EVENT_POWER_TOGGLE`입니다.

### 자동 모드 (`--auto`)

- 20cm 이하에 처음 접근하면 OFF에서 S1로 기동합니다.
- 가까운 상태에서는 엔코더로 풍량을 조절할 수 있습니다.
- 20cm를 초과하면 OFF가 되고, 거리 밖에서는 버튼·회전 입력을 무시합니다.
- 가까운 상태에서 버튼으로 끈 뒤에는 다시 버튼 ON 또는 멀어졌다 재접근하여 기동합니다.
- 센서 Timeout/Busy 또는 유효하지 않은 거리값은 `SENSOR_LOST`로 처리하여 OFF로 전환합니다.

기타 입력 오류나 모터 출력 오류는 OFF를 요청하고 실패 종료합니다.
LED 장치 열기·출력 실패는 로그를 남기고 모터 제어를 계속합니다.
시작·종료 시 OFF를 요청하며 출력 장치의 `release()`에서도 출력을 끕니다.

## 드라이버 인터페이스

| 장치 | 호출 | 데이터 |
|---|---|---|
| `/dev/ultrasonic` | `read()` | native `int`, 거리 mm |
| `/dev/fan_encoder` | `read()` | `int steps` + `unsigned int pressed`, 총 8 bytes |
| `/dev/fan_pwm` | `write()` | native `int`, 0/2/5/8 |
| `/dev/fan_led` | `write()` | native `int`, 0/1/2/3 |

전송값은 문자열이 아닌 바이너리이며 같은 타깃 ABI로 빌드합니다.
엔코더의 `steps`는 read 사이의 누적 회전량, `pressed`는 버튼이 눌리면 1입니다.
`fan_ioctl.h`에 선언된 매크로는 현재 드라이버에서 처리하지 않으며, 통합 앱은 `read()` / `write()`를 사용합니다.

- [상세 드라이버 인터페이스](docs/driver_interface.md)
- [상태 전이표와 상태 다이어그램](docs/state_diagram.md)
- [초음파 IRQ 처리 흐름 그림](docs/presentation/ultrasonic-irq-flow.png)

## 시연과 테스트

실제 보드에서 통합 동작 시연과 시연영상 제작을 완료했습니다.
발표 시연은 초음파 접근에 따른 팬 자동 기동 등 실제 하드웨어 동작을 보여줍니다.
시연 GIF는 아래에서 확인할 수 있습니다.

<img src="./assets/smart%20fan.gif" width="400" alt="SmartFan 하드웨어 동작 시연" />

저장소에는 다음 모의 테스트가 포함되어 있습니다. `make test`로 실행합니다.

| 테스트 | 확인 범위 |
|---|---|
| `test_event` | 입력 이벤트, 버튼 debounce·긴 누름, 자동 모드 |
| `test_state` | 상태 전환과 상태 값 일관성 |
| `test_pwm` | 사용자 Output의 모터·LED 명령과 오류 처리 |
| `test_led_driver` | 모의 커널 API로 LED 점등·입력 검증·자원 해제 |
| `test_main` | 정상 실행·자동 모드·종료·오류 등 11개 시나리오 |
| `test_load_motor.py` | PWM ID 선택과 모터 모듈 로드 로직 |

모의 테스트는 실제 GPIO/IRQ/PWM 동작이나 커널 모듈 호환성 검증을 대신하지 않습니다.
LED 개별 점등은 `tests/test_led_bar.py`로 확인할 수 있으며, LED 커널 모듈이 GPIO를 점유한 상태에서는 직접 GPIO 테스트를 실행하지 않습니다.

## 트러블슈팅

| 문제 | 원인 | 해결 |
|---|---|---|
| 모터가 회전하지 않음 | PWM 출력 핀이 일반 GPIO 모드로 설정됨 | Jetson 핀을 PWM 모드로 설정하고 물리33번 컨트롤러 선택 |
| 엔코더 회전 입력이 불안정하거나 감지되지 않음 | 풀업 저항 누락으로 입력이 Floating 상태 | 엔코더 입력 풀업 연결 후 인터럽트 동작 확인 |

코드와 함께 배선, Pinmux, PWM 채널을 확인하여 통합 문제를 해결했습니다.

## 저장소 구성

```text
include/               공통 상태·이벤트 정의와 장치 프로토콜 상수
kernel/ultrasonic/     초음파 센서·엔코더 GPIO/IRQ 드라이버
kernel/fan_pwm/        모터 PWM Character Device 드라이버
kernel/fan_led/        LED GPIO Character Device 드라이버
user/main.c           실행 루프와 종료 처리
user/event/           입력 이벤트 처리
user/state/           OFF/S1/S2/S3 상태 관리
user/output/          모터·LED 출력
scripts/              보드용 모터 로드 도구
tests/                모의 테스트와 보드용 LED 테스트
docs/                하드웨어·인터페이스·상태 전이 문서와 발표 그림
hardware/kicad/       회로도 원본·PDF·배선 자료
```

## 팀

김선호 · 문형철 · 박수민 · 한규리

입력 드라이버·이벤트 처리, 상태 머신·Main·통합, PWM·LED 출력을 나누어 구현하고 공통 헤더와 `/dev` 인터페이스를 기준으로 연결했습니다.

## 향후 개선

- 실측 PWM 파형과 입력 반응 시간 기록
- 센서 측정 오차와 오류 재현 결과 정리
- JetPack 버전, 전원·레벨 변환 등 상세 하드웨어 기록 보완
