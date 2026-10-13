# 하드웨어 설정과 보드 확인 항목

담당: Member 1 (센서), Member 3 (모터 / LED)

| 항목 | 확정 값 |
|---|---|
| JetPack / Kernel 버전 | Kernel 5.15.199-tegra (사용자 로그), JetPack 확인 필요 |
| 캐리어 보드 | NVIDIA 정품 Orin Nano Developer Kit (사용자 확인) |
| 초음파 센서 모델 | TODO |
| Trigger / Echo GPIO 및 Pinmux | 현재 코드: 물리11/36, global GPIO460/461; 보드 확인 필요 |
| 엔코더 S1 / S2 / KEY | 현재 코드: 물리13/18/22, global GPIO470/473/471; 보드 확인 필요 |
| Echo 레벨 변환 회로 | TODO |
| 모터 / 전압 / 별도 전원 | EZmotor R300 / Jetson 물리2번 5V를 브레드보드로 공급 / 별도 전원 없음 (사용자 확인) |
| 모터 드라이버 회로 | HW-095 (사용자 확인), 실제 전원 단자 및 점퍼 연결 확인 필요 |
| 모터 PWM 채널 / 주기 | ENA 물리33 / 32c0000.pwm / 현재 주기 1ms (1kHz) |
| 모터 방향 IN1 / IN2 | 현재 코드: IN1 물리29 / global GPIO453, IN2 GND 고정 |
| LED 연결 방식 / PWM 채널 | LED1..8: 물리7/12/19/16/21/23/37/31. Main 상태와 연동 |

핀 번호와 PWM API는 보드 및 커널 버전을 확인한 뒤 확정한다.
위 global GPIO는 현재 코드의 설정이며 JetPack/커널별 GPIO base를 확인해야 한다.
엔코더 `transitions=4`가 기본이며 보드 특성에 따라 2, 방향은 `reverse=1`로 바꿀 수 있다.
`tests/test_led_bar.py`는 기존 센서/엔코더/모터 핀을 출력으로 설정하지 않도록 변경했다.
5V Echo 센서는 GPIO 입력에 맞는 레벨 변환을 사용한다.
모터는 드라이버 회로와 별도 전원을 사용하며 공통 GND 및 보호 회로를 검토한다.

TODO: 배선도, Device Tree / Pinmux 설정, 측정 결과 기록.

## 모터 로드: ENA 33번, IN1 29번

배선은 ENA=물리33, IN1=물리29, IN2=GND를 기준으로 한다.
ENA 채널은 `32c0000.pwm`이다. `3280000.pwm`은 물리15에 해당하므로 선택하지 않는다.
`pwm_id`는 물리 핀 번호 33도, 고정 상수도 아니다. Linux 5.15의 sysfs
`pwmchipN/device`가 `32c0000.pwm`인 N을 찾아 channel 0의 global ID로 사용한다.
앱을 Ctrl+C로 종료한 뒤 다음 명령을 실행한다.

```sh
make -C kernel/fan_pwm
sudo python3 scripts/load_motor.py
sudo ./build/smart-fan
```

스크립트는 장치/모듈 파일을 확인한 뒤 기존 모터 모듈을 해제하고 정확한 ID로 재로드한다.
모듈이 사용 중이면 중단한다. 읽기만 하려면 `python3 scripts/load_motor.py --print-id`.
이 스크립트는 사용자 로그의 Linux 5.15를 대상으로 하며 Pinmux를 변경하지 않는다.
재로드 뒤 S1에서 `32c0000.pwm`의 smartfan-motor duty=600000, IN1=HIGH를 확인한다.

## LED 바 새 배선 (WCNLB8, LED1 → LED8)

11(Trig), 13/18/22(엔코더), 36(Echo), 29(모터 IN1)을 제외했다.
모터 PWM 선택을 위해 15/32/33도 사용하지 않는다.
원래 겹치지 않았던 LED1/4/7/8 배선은 유지한다.

| LED | 기존 물리 핀 | 새 물리 핀 | GPIO 이름 | chip offset | global GPIO (base=348일 때) |
|---|---|---|---|---|---|
| LED1 | 7 | 7 | PAC.06 | 144 | 492 |
| LED2 | 11 | 12 | PH.07 | 50 | 398 |
| LED3 | 13 | 19 | PZ.05 | 135 | 483 |
| LED4 | 16 | 16 | PY.04 | 126 | 474 |
| LED5 | 18 | 21 | PZ.04 | 134 | 482 |
| LED6 | 22 | 23 | PZ.03 | 133 | 481 |
| LED7 | 37 | 37 | PY.02 | 124 | 472 |
| LED8 | 31 | 31 | PQ.06 | 106 | 454 |

핀 이름/offset 근거: [NVIDIA Jetson.GPIO 핀 정의](https://github.com/NVIDIA/jetson-gpio/blob/master/lib/python/Jetson/GPIO/gpio_pin_data.py).
Orin Nano는 ORIN_NX 핀 정의를 공유한다. global 번호는 기존 프로젝트와 같은
GPIO base=348 기준으로 계산한 값이며, 보드의 base가 다르면 base+offset으로 바꾼다.
Python 테스트는 BOARD 물리 번호를 사용하므로 global 번호를 입력하지 않는다.

GPIO → LED 애노드(+), LED 캐소드(-) → 각 LED용 330Ω 저항 → GND로 연결한다.
LED1부터 차례대로 S1=2개, S2=5개, S3=8개를 켜고 종료 시 전체 LOW로 정리한다.
12번은 I2S, 16/19/21/23/37번은 SPI 대체 기능이 있는 핀이므로, 사용 중인 보드에서
해당 주변장치가 핀을 점유하지 않고 GPIO 출력으로 설정되어 있어야 한다.

```sh
sudo python3 tests/test_led_bar.py
# 위와 동일한 배선을 명시하는 경우:
sudo python3 tests/test_led_bar.py --pins 7 12 19 16 21 23 37 31
```

`kernel/fan_led/fan_led_drv.c`의 LED1..8 GPIO 배열을 base=348 기준
`{492, 398, 483, 474, 482, 481, 472, 454}`로 변경했다.
LED 드라이버 프로토콜은 int 단계 0/1/2/3이며 모터의 int 0/2/5/8과 구분한다.
Main/Output은 `/dev/fan_led`에 OFF/S1/S2/S3의 mode 0/1/2/3을 전달한다.
장치가 없거나 LED 출력이 실패하면 로그를 남기고 모터만 제어한다.
LED 드라이버를 로드한 상태에서는 GPIO를 직접 쓰는 Python 테스트를 실행하지 않는다.

```sh
make -C kernel/fan_led
sudo insmod kernel/fan_led/fan_led_drv.ko
# 드라이버를 통해 단계 0/1/2/3을 전달하고 점등 확인:
sudo python3 - <<'PY'
import os, struct, time
fd = os.open('/dev/fan_led', os.O_WRONLY)
try:
    for level in (0, 1, 2, 3, 0):
        os.write(fd, struct.pack('@i', level))
        time.sleep(1.5)
finally:
    os.close(fd)
PY
```

기존 모듈을 이미 로드했다면 재로드하기 전에 `sudo rmmod fan_led_drv`로 해제한다.
드라이버 경로의 실제 커널 빌드/점등은 Orin에서 확인한다.
