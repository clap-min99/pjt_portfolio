# 시스템 설치 및 운용 가이드 (Setup & Operation Guide v1.0)

본 문서는 **스마트 가스 모니터링 시스템**을 구성하는 3계층(STM32 베어메탈 펌웨어, Qt 데스크톱 관제 서버, Android 모바일 클라이언트)의 개발 환경 구축, 필수 의존성 패키지 설치, 소스 코드 빌드 및 플래싱, 네트워크 구성, 그리고 실제 운용 및 테스트 절차를 정의합니다.

---

## 📋 목차

1. 개발 및 실행 환경 요구사항 (Prerequisites)
2. 계층별 빌드 및 배포 절차 (MSYS2 패키지 설치 포함)
3. 네트워크 환경 구성 (Local Hotspot / Tailscale VPN)
4. 단계별 시스템 통합 구동 절차
5. 통합 검증 및 시나리오 테스트 가이드
6. 긴급 점검 및 트러블슈팅 체크리스트

---

## 1. 개발 및 실행 환경 요구사항 (Prerequisites)

시스템 빌드 및 구동을 위해 다음 툴체인과 환경이 준비되어야 합니다.

| 계층 | 개발 툴체인 / IDE | 필수 라이브러리 및 SDK | 대상 하드웨어 / OS |
| :--- | :--- | :--- | :--- |
| **Edge Node** | • GNU Arm Embedded Toolchain (`arm-none-eabi-gcc`)<br>• Make / ST-Link CLI (또는 STM32CubeProgrammer) | • CMSIS Core Header (ARM Cortex-M4)<br>• C99 컴파일러 | STM32F411RE Nucleo-64 |
| **Gateway** | • Qt Creator (또는 MSVC / MinGW / MSYS2 UCRT64)<br>• C++17 지원 컴파일러 | • Qt 5.15+ 또는 Qt 6.x<br>• 필수 모듈: `widgets`, `network`, `serialport`, `charts` | Windows 10 / 11, Linux |
| **Client** | • Android Studio (Ladybug / Koala 이상)<br>• OpenJDK 17 / 21 | • Android SDK (minSdk 26, compileSdk 34+)<br>• Jetpack Compose, CameraX 1.4+ | Android 8.0 (API 26) 이상 디바이스 |

---

## 2. 계층별 빌드 및 배포 절차

### 2.1 STM32 펌웨어 빌드 및 플래싱 (`stm32-firmware/`)

1. **소스 코드 빌드**:
터미널에서 `stm32-firmware` 디렉터리로 이동 후 `make` 명령어를 실행하여 바이너리를 생성합니다.
```bash
cd stm32-firmware
make clean
make
```


* 빌드 완료 시 `main.elf`, `main.bin`, `main.hex` 파일이 생성됩니다.


2. **MCU 플래싱**:
* **ST-Link 드래그 앤 드롭**: PC에 인식된 `NODE_F411RE` 가상 드라이브에 `main.bin`을 복사합니다.
* **STM32CubeProgrammer CLI 활용**:
```bash
STM32_Programmer_CLI -c port=SWD -w main.bin 0x08000000 -v -rst
```





---

### 2.2 Qt 관제 서버 빌드 및 실행 (`qt-server/`)

#### [사전 작업] MSYS2 (UCRT64) 환경 필수 모듈 설치

Windows MSYS2 UCRT64 환경에서 Qt5를 사용하는 경우, 시리얼 통신과 차트 렌더링을 위한 추가 모듈을 먼저 설치해야 합니다.

```bash
# MSYS2 UCRT64 터미널에서 실행
pacman -S mingw-w64-ucrt-x86_64-qt5-serialport
pacman -S mingw-w64-ucrt-x86_64-qt5-charts
```

#### 빌드 및 실행 절차

1. **Qt Creator를 통한 실행 (권장)**:
* Qt Creator 실행 $\rightarrow$ `qt-server/qt-server.pro` 프로젝트 열기
* Build Kit(MinGW 또는 MSVC 64-bit) 선택
* `Ctrl + R` (Run)을 눌러 빌드 및 실행


2. **qmake 커맨드라인 빌드 (MSYS2 / MinGW)**:
```bash
cd qt-server
qmake qt-server.pro
mingw32-make -j4       # Linux의 경우: make -j4
./release/SmartMonitoringServer.exe
```



---

### 2.3 Android 모바일 클라이언트 빌드 (`android/`)

1. **Android Studio 실행 및 프로젝트 오픈**:
* `android/` 디렉터리를 Android Studio에서 열기
* Gradle Sync 완료 대기


2. **APK 빌드 및 설치**:
* 디버깅 모드가 활성화된 안드로이드 스마트폰을 USB로 연결
* `Run 'app'` (`Shift + F10`) 실행하여 앱 설치
* 커맨드라인 빌드:
```bash
cd android
./gradlew assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk
```





---

## 3. 네트워크 환경 구성 (Local Hotspot / Tailscale VPN)

운용 목적에 맞춰 **로컬 핫스팟 모드** 또는 **Tailscale 원격 VPN 모드** 중 하나를 구성합니다.

### 3.1 모드 A: Windows 모바일 핫스팟 (로컬 독립망)

인터넷 연결이 불가능한 독립 현장에서 PC를 Wi-Fi AP로 설정합니다.

1. **Windows 설정**: `설정` $\rightarrow$ `네트워크 및 인터넷` $\rightarrow$ `모바일 핫스팟` 켜기 (대역: 2.4GHz / 5GHz)
2. **IP 확인**: `ipconfig` 실행 후 모바일 핫스팟 어댑터 IP 확인 (통상 `192.168.137.1`)
3. **스마트폰 접속**: PC 핫스팟 Wi-Fi에 스마트폰을 연결

---

### 3.2 모드 B: Tailscale 오버레이 VPN (원격 LTE/5G망 - 권장 ⭐)

스마트폰이 외부 이동통신망(LTE/5G)에 위치한 상태에서 원격 관제할 때 사용합니다.

1. **PC 및 스마트폰에 Tailscale 설치 및 로그인** ([tailscale.com](https://tailscale.com))
2. **PC Tailscale 가상 IP 확인**: PC 트레이 아이콘에서 `100.72.78.11` (예시) 확인
3. **스마트폰 Tailscale 활성화**: VPN 연결 스위치 ON (포트포워딩 불필요)

---

## 4. 단계별 시스템 통합 구동 절차

```text
[Step 1: 하드웨어 결선] ──> [Step 2: Qt 서버 시작] ──> [Step 3: 시리얼 포트 연결]
                                                              │
[Step 5: 실시간 관제]   <── [Step 4: Android 연결 및 스트리밍 시작] ◄┘
```

1. **하드웨어 전원 및 결선 연결**:
* STM32 Nucleo 보드를 USB 케이블로 PC에 연결 (가상 COM 포트 및 5V 공급).
* 서보 모터(PA0), 모터 드라이버(PC0, PC1), 가변저항(PA6), LED(PB0, PB1), 키(PB4, PB5) 결선 상태 확인.


2. **Qt 관제 서버 실행 및 TCP 리슨**:
* `SmartMonitoringServer` 실행
* 포트 번호 확인 (`8080`) $\rightarrow$ **[서버 시작]** 버튼 클릭 (서버 로그에 포트 개방 메시지 확인)


3. **STM32 시리얼 포트 연결**:
* 포트 목록 콤보박스에서 ST-Link 포트(예: `COM4`) 선택
* 보드레이트 `115200` 확인 $\rightarrow$ **[시리얼 연결]** 클릭
* 200ms마다 수신되는 가스 수치가 실시간 차트에 그려지는지 확인


4. **Android 앱 실행 및 스트리밍 개시**:
* 스마트폰에서 앱 실행 (카메라 권한 허용)
* Server IP에 PC IP(`192.168.137.1` 또는 Tailscale `100.72.78.11`), Port `8080` 입력
* **[Connect]** 클릭 (우측 상단 배지 `Connected` 전환)
* **[전송]** 클릭 $\rightarrow$ Qt 관제 화면에 카메라 영상(CCTV) 실시간 렌더링 확인



---

## 5. 통합 검증 및 시나리오 테스트 가이드

### 5.1 시나리오 1: 정상 상태 모니터링 검증

* **동작**: 가변저항을 낮은 수치(~1200)로 유지
* **검증 항목**:
* [ ] Qt 실시간 차트 곡선이 파란색으로 안정적으로 갱신되는가?
* [ ] 상태 배지가 [정상 (초록색)]으로 유지되는가?
* [ ] Android 앱에 동일한 가스 수치(`GAS:1200:3000`)와 NORMAL 배지가 표시되는가?
* [ ] 밸브 서보 모터가 0°(개방), DC 팬이 정지 상태를 유지하는가?



---

### 5.2 시나리오 2: 가스 누출 감지 및 자동 차단 연동

* **동작**: 가변저항을 시계 방향으로 돌려 ADC 수치를 **3000 이상**으로 상승시킴
* **검증 항목**:
* [ ] Qt 상태 배지가 [위 험 (빨간색)]으로 즉시 전환되는가?
* [ ] Qt 서버가 STM32로 `'1'`(차단) 명령을 전송하고 로그에 기록되는가?
* [ ] STM32의 SG90 서보 모터가 90°(차단)로 회전하는가?
* [ ] **DC 환기 팬이 고속 회전**하고, **차단벽 LED(PB0, PB1)가 점등**하는가?
* [ ] Android 화면에 DANGER 배지가 점등되고 카메라 영상이 끊김 없이 유지되는가?
* [ ] `logs/gas_log_YYYY-MM-DD.csv` 파일에 `DANGER` 상태가 Append 기록되는가?



---

### 5.3 시나리오 3: 모바일 원격 수동 복구 제어

* **동작**: 가변저항을 다시 3000 미만으로 낮춘 후, 스마트폰 앱에서 **[밸브 복구]** 버튼 클릭
* **검증 항목**:
* [ ] Qt 관제 서버에 `[TCP] 안드로이드 원격 제어: 밸브 복구 요청` 로그가 출력되는가?
* [ ] STM32로 UART `'0'` 명령이 전송되는가?
* [ ] 서보 모터가 **0°(개방)** 위치로 원복되고, DC 팬이 정지하며 LED가 소등되는가?



---

### 5.4 시나리오 4: 현장 물리 키(Tact Switch) 인터록 테스트

* **동작**: STM32 보드의 `PB4` 스위치(수동 차단) 및 `PB5` 스위치(수동 복구)를 물리적으로 클릭
* **검증 항목**:
* [ ] PB4를 누르면 서버 명령 없이도 현장에서 즉시 밸브 차단, 팬 가동, LED 점등이 실행되는가?
* [ ] PB5를 누르면 즉시 정상 상태로 복구되는가?
* [ ] 동일 스위치를 연속으로 눌러도 소프트웨어 인터록에 의해 모터 튐이 발생하지 않는가?



---

## 6. 긴급 점검 및 트러블슈팅 체크리스트

| 이상 현상 | 점검 위치 | 조치 방법 |
| :--- | :--- | :--- |
| **`Unknown module(s) in QT: serialport charts`** | MSYS2 환경 / Qt 설치 | • `pacman -S mingw-w64-ucrt-x86_64-qt5-serialport`<br>• `pacman -S mingw-w64-ucrt-x86_64-qt5-charts` 실행 후 qmake 재수행 |
| **시리얼 연결 실패 / 포트 미인식** | PC 장치 관리자 / USB 케이블 | • ST-Link VCP 드라이버 재설치<br>• 데이터 통신 지원 USB 케이블인지 확인<br>• Qt 콤보박스를 클릭하여 포트 목록 동적 재스캔 |
| **Android 서버 연결 타임아웃** | 방화벽 / IP 설정 | • Windows 방화벽에서 포트 8080(TCP) 인바운드 허용 규칙 추가<br>• 스마트폰과 PC가 동일 핫스팟에 연결되어 있는지 확인 (또는 Tailscale 활성화 여부 점검) |
| **서보 모터 각도가 45도만 회전함** | STM32 클럭 설정 | • APB1 타이머 클럭(96MHz) 기준 프리스케일러가 `PSC = 95`로 설정되었는지 확인 (`timer.c`) |
| **모터 기동 시 가스 수치 급상승** | 전원 및 접지 | • 서보 모터 및 모터 드라이버 전원단에 100uF ~ 470uF 캐패시터 보강<br>• Star Ground 결선 상태 점검 |