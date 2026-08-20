# 트러블슈팅 및 기술적 의사결정 기록 (Troubleshooting Log v1.0)

본 문서는 **스마트 가스 모니터링 시스템** 개발 과정에서 직면한 하드웨어, 임베디드 펌웨어, 통신 프로토콜, GUI 서버 계층의 기술적 문제와 그에 대한 근본 원인 분석, 해결 과정 및 엔지니어링 의사결정을 기록한 기술 문서입니다.

---

## 📋 목차
1. STM32 APB1 타이머 클럭 배율기(x2) 특성으로 인한 PWM 주파수/각도 왜곡
2. 액추에이터 구동 시 전원 강하(Voltage Sag)로 인한 가스 ADC 수치 왜곡
3. 아날로그 서보 모터(SG90)의 데드밴드 특성과 떨림/소음 분석
4. 상태 기반 소프트웨어 인터록(Interlock) 설계를 통한 액추에이터 보호
5. Qt ComboBox 동적 갱신 시 시그널 폭풍(Signal Storm) 방지
6. 단일 TCP 소켓 세션 내 바이너리(영상)/텍스트(명령) 디멀티플렉싱
7. std::make_unique 템플릿 인자 모호성 해결과 위임 생성자 설계

---

## 1. STM32 APB1 타이머 클럭 배율기(x2) 특성으로 인한 PWM 주파수/각도 왜곡

### 1.1 현상

* 서보 모터 각도를 90°(`SERVO_VALVE_CLOSE_ANGLE = 90`)로 지시했으나 물리적으로 약 45°만 회전함.
* 각도 인자를 강제로 180°로 설정했을 때만 90° 직각 위치로 동작하는 이상 동작 발생.

### 1.2 근본 원인 분석 (Root Cause)

STM32F411RE의 클럭 트리 구조상 `SYSCLK = 96MHz` 환경에서 APB1 버스는 최대 동작 주파수(50MHz) 제한으로 인해 2분주(`/2`)되어 **PCLK1 = 48MHz**로 동작함.

STM32 내부 하드웨어 규칙에 따르면, **APB1 분주비가 1이 아닐 경우 APB1에 연결된 타이머(TIM2~5) 클럭 입력은 PCLK1의 2배($48\text{MHz} \times 2 = 96\text{MHz}$)로 강제 공급**됨.

$$\text{Timer Clock} = 2 \times \text{PCLK1} = 96\text{ MHz}$$

기존 드라이버 코드가 APB1 버스 클럭(48MHz)을 기준으로 $1\mu\text{s}$ 분주비를 계산하여 타이머가 실제로는 2배 빠른 속도(1 tick = $0.5\mu\text{s}$)로 카운트되고 있었음.

* 계산된 주기: 20ms(50Hz) $\rightarrow$ 실제 출력 주기: **10ms (100Hz)**
* 계산된 90° 펄스: 1500µs $\rightarrow$ 실제 출력 펄스: **750µs (서보 모터는 절반 각도인 45°로 인식)**

```text
[정상 기대치] 1 tick = 1.0us  --> 1500 tick = 1500us (90도 회전)
[실제 출력치] 1 tick = 0.5us  --> 1500 tick =  750us (45도 회전)
```

### 1.3 해결 및 엔지니어링 의사결정

* **Bad Practice (기각)**: 각도 변환 수식의 분모를 임의로 90으로 조작하는 방식은 100Hz의 과도한 PWM 주파수를 모터에 지속 인가하여 모터 과열 및 수명 단축을 초래하므로 기각.
* **Best Practice (적용)**: 하드웨어 타이머 드라이버(`timer.c`)의 클럭 분주비를 실제 타이머 공급 클럭인 **96MHz 기준 96분주**로 정석 보정하여 물리 계층의 단위 무결성($1\text{ tick} = 1\mu\text{s}$, $\text{Period} = 20\text{ms}$)을 확립함.

```c
// timer.c: 96MHz 타이머 입력 클럭 기준 1us 틱 및 20ms(50Hz) 정밀 생성
void TIM2_PWM_Init(void)
{
    Macro_Set_Bit(RCC->APB1ENR, 0U); // TIM2 클럭 인에이블

    TIM2->PSC = 96U - 1U;            // 96MHz / 96 = 1MHz (1 tick = 1us)
    TIM2->ARR = 20000U - 1U;         // 20000us = 20ms (정확한 50Hz)

    Macro_Write_Block(TIM2->CCMR1, 0x7, 0x6, 4); // PWM Mode 1
    Macro_Set_Bit(TIM2->CCMR1, 3U);               // OC1PE = 1
    Macro_Set_Bit(TIM2->CCER, 0U);                // CC1E = 1

    Macro_Set_Bit(TIM2->EGR, 0U);                 // 섀도우 레지스터 즉시 갱신
    Macro_Clear_Bit(TIM2->SR, 0U);
    Macro_Set_Bit(TIM2->CR1, 0U);                 // CEN = 1
}
```

---

## 2. 액추에이터 구동 시 전원 강하(Voltage Sag)로 인한 가스 ADC 수치 왜곡

### 2.1 현상

* 실제 가스 농도 변화가 없음에도 밸브 정상 상태(모터 OFF, LED OFF)에서는 ADC 값이 **~1270**으로 측정되다가, 밸브 차단 상태(DC 환기 모터 ON, 경보 LED ON)로 진입하면 수치가 **~1530**으로 급상승하는 현상 발생.

### 2.2 근본 원인 분석 (Root Cause)

STM32 12비트 SAR ADC의 디지털 변환 수식은 기준 전압($V_{REF}$, 통상 3.3V)에 반비례함.

$$ADC\_Value = \frac{V_{sensor}}{V_{REF}} \times 4095$$

1. **정상 상태**: 액추에이터 무부하 상태로 $V_{REF} \approx 3.30\text{V}$ 유지.

$$ADC = \frac{1.02\text{V}}{3.30\text{V}} \times 4095 \approx \mathbf{1266}$$


2. **차단 상태**: DC 모터 기동 전류 및 LED 전력 소모로 인해 MCU 전원선에 **전압 강하(Voltage Sag)** 및 도선 저항에 의한 접지 전위 상승(Ground Bounce)이 발생하여 실제 $V_{REF}$가 **약 2.75V**로 하강.

$$ADC = \frac{1.02\text{V}}{\mathbf{2.75\text{V}}} \times 4095 \approx \mathbf{1518}$$



센서 출력 전압($V_{sensor}$)은 동일하지만, 기준이 되는 분모($V_{REF}$)가 작아져 디지털 결과값이 약 250 카운트 이상 왜곡된 것임.

```text
[전원 안정] V_ref: 3.3V ───> ADC: 1270 (정상)
[부하 발생] V_ref: 2.75V ──> ADC: 1520 (기준 전압 강하로 수치 왜곡 발생)
```

### 2.3 해결 및 대응 전략

1. **하드웨어 대책**: DC 모터 전원단과 MCU 아날로그 전원단의 물리적 바이패스 분리 및 대용량 디커플링 캐패시터($100\mu\text{F} \sim 470\mu\text{F}$) 병렬 배치.
2. **소프트웨어 대책**: 상위 관제 서버(`MainWindow`)에서 위험 감지 후 자동 복구 판정 시 전압 강하분을 감안한 **히스테리시스(Hysteresis) 마진** 및 단일 트리거 래치 플래그(`m_isValveClosed`)를 적용하여 수치 흔들림으로 인한 채터링(Chattering) 방지.

---

## 3. 아날로그 서보 모터(SG90)의 데드밴드 특성과 떨림/소음 분석

### 3.1 현상

* 서보 모터가 목표 각도에 도달한 후에도 지속적으로 "지이잉"하는 소음과 미세한 기계적 떨림(Jitter/Hunting)이 발생함.

### 3.2 원인 분석

* **아날로그 서보 모터 내부 제어 루프의 한계**:
* SG90 내부의 아날로그 비교 회로와 포텐셔미터(가변저항)는 데드밴드(Deadband, 오차 불감대)가 매우 좁음.
* 목표 PWM 펄스와 현재 피드백 위치 간에 $\pm 0.1^\circ$ 수준의 미세 오차만 감지되어도 내부 모터에 정/역방향 보정 전류를 초당 수백 회 가하면서 기어가 맞부딪히는 소음 발생.


* **전원선 노이즈 및 타이머 지터**:
* 단일 전원선에서 유입되는 미세 전압 리플이 타이밍 펄스를 흔들어 서보 내부 IC가 계속 위치 보정을 시도함.



### 3.3 기술적 결론 및 설계 반영

* 고가의 디지털 산업용 서보와 달리, 보급형 아날로그 서보 모터에서는 **소프트웨어적 신호 차단 없이는 구조적으로 불가피한 물리적 현상**임을 규명함.
* 밸브 유지력이 크게 요구되지 않는 환경을 위해 **동작 완료(400ms) 후 PWM 출력을 차단(`TIM2_PWM_Stop()`)하는 슬립 기법**을 기술적으로 검토 및 검증함.

---

## 4. 상태 기반 소프트웨어 인터록(Interlock) 설계를 통한 액추에이터 보호

### 4.1 문제 인식

* GUI에서 중복 클릭이 발생하거나 가스 수치가 임계값 이상인 상태가 지속될 경우, 100ms마다 STM32로 차단 명령(`'1'`)이 반복 송신됨.
* 이미 구동 중인 하드웨어 레지스터를 불필요하게 덮어쓰고 서보 모터 PWM 펄스를 재설정하여 모터 튐 및 불필요한 연산 낭비 발생.

### 4.2 개선 설계: 상태 기반 소프트웨어 인터록 가드

```c
// main.c
static unsigned char g_valve_state = 0; // 0: 개방/정상, 1: 차단/환기

static void Valve_Set_State(unsigned char state)
{
    // [소프트웨어 인터록 가드] 동일 상태 명령 중복 진입 차단
    if (g_valve_state == state)
    {
        return;
    }

    g_valve_state = state;
    LED_Display(state);   // LED 점등/소등
    Motor_Display(state); // DC 팬 가동/정지
    Servo_Display(state); // 서보 모터 90도/0도 이동
}
```

* **효과**: 불필요한 하드웨어 I/O 및 타이머 레지스터 쓰기를 $100\%$ 차단하고, 다중 입력 환경(물리 스위치, UART 명령, 자동 차단)에서의 상태 무결성을 확보함.

---

## 5. Qt ComboBox 동적 갱신 시 시그널 폭풍(Signal Storm) 방지

### 5.1 현상

* 시리얼 포트 콤보박스를 클릭했을 때 사용 가능한 COM 포트를 동적으로 재스캔하도록 `eventFilter`를 구현함.
* `clear()` 및 `addItems()` 실행 과정에서 `currentIndexChanged` 시그널이 내부적으로 수 차례 방출되어 불필요한 슬롯 트리거 및 설정값 오동작 유발.

### 5.2 해결: RAII 기반 `QSignalBlocker` 적용

```cpp
// mainwindow.cpp
void MainWindow::updatePortList()
{
    if (!ui->cbPortList || !m_serialManager)
        return;

    // UI 갱신 중 발생하는 모든 연쇄 시그널 일시 차단
    const QSignalBlocker blocker(ui->cbPortList);

    const QString currentText = ui->cbPortList->currentText();
    ui->cbPortList->clear();

    QStringList ports = m_serialManager->availablePorts();
    if (ports.isEmpty()) {
        for (int i = 1; i <= 10; ++i) {
            ports.append(QString("COM%1").arg(i));
        }
    }
    ui->cbPortList->addItems(ports);

    // 이전 선택 포트 인덱스 안전 복원
    int idx = ui->cbPortList->findText(currentText);
    if (idx != -1) {
        ui->cbPortList->setCurrentIndex(idx);
    }
}
```

* **효과**: 포트 리스트를 안전하게 동적 갱신하면서도 불필요한 이벤트 전파를 완벽히 억제함.

---

## 6. 단일 TCP 소켓 세션 내 바이너리(영상)/텍스트(명령) 디멀티플렉싱

### 6.1 문제 정의

* Android와 Qt 서버 간에 고용량 비디오 스트림(JPEG 바이트 배열)과 실시간 제어 명령(`'1'`, `'0'`)을 단일 TCP 포트(8080)로 동시에 주고받아야 함.
* 이종 데이터 간의 프레임 경계 혼선 없이 단일 수신 버퍼에서 빠르고 정확하게 데이터를 분리해 낼 수 있는 프로토콜 구조가 필요함.

### 6.2 해결: 최상위 바이트 기반 조건부 분기 파서

1. **프레이밍 구조 설계**:
* **영상 패킷**: 4바이트 Big-Endian 길이 헤더 + JPEG 페이로드
* **명령 패킷**: 1바이트 ASCII 문자(`'1'`=`0x31`, `'0'`=`0x30`) + `\n`


2. **디멀티플렉싱 로직**:
* 스마트폰 카메라 JPEG 프레임 크기(수십 KB)는 4바이트 Big-Endian 정수 표현 시 최상위 바이트가 반드시 `0x00`이 됨.
* 수신 버퍼의 첫 바이트가 `0x00`이 아니면 **텍스트 제어 명령**으로 즉시 파싱하고, `0x00`이면 **4바이트 바이너리 길이 헤더**로 진입함.



```cpp
// tcpstreamserver.cpp
void TcpStreamServer::onReadyRead()
{
    m_buffer.append(m_clientSocket->readAll());

    while (!m_buffer.isEmpty()) {
        if (m_imageSize == 0) {
            // 1. 제어 명령 분기: 첫 바이트가 0x00이 아닌 경우
            if (static_cast<unsigned char>(m_buffer[0]) != 0x00) {
                int newlineIdx = m_buffer.indexOf('\n');
                if (newlineIdx != -1) {
                    QByteArray lineBytes = m_buffer.left(newlineIdx).trimmed();
                    m_buffer.remove(0, newlineIdx + 1);
                    emit valveCommandReceived(lineBytes.at(0));
                }
                continue;
            }

            // 2. 비디오 스트림 헤더 분기: 4바이트 길이 파싱
            if (m_buffer.size() < sizeof(qint32)) break;
            QDataStream stream(m_buffer.left(sizeof(qint32)));
            stream.setByteOrder(QDataStream::BigEndian);
            stream >> m_imageSize;
            m_buffer.remove(0, sizeof(qint32));
        }

        // 3. JPEG 페이로드 완성 대기 및 이미지 렌더링
        if (m_buffer.size() < m_imageSize) break;
        QByteArray jpegData = m_buffer.left(m_imageSize);
        m_buffer.remove(0, m_imageSize);
        
        QPixmap pixmap;
        if (pixmap.loadFromData(jpegData, "JPEG")) {
            emit frameReceived(pixmap);
        }
        m_imageSize = 0;
    }
}
```

---

## 7. `std::make_unique` 템플릿 인자 모호성 해결과 위임 생성자 설계

### 7.1 현상

* `MainWindow` 초기화 리스트에서 `, m_settingsManager(std::make_unique<SettingsManager>())` 작성 시 정적 분석기(IntelliSense)에서 `Too few arguments` 경고(노란색 밑줄)가 발생함.

### 7.2 원인 및 해결

* `SettingsManager(const QString& fileName = "config.ini")`처럼 단일 생성자에 기본 인자(Default Argument)가 지정되어 있을 때, C++ 가변 인자 템플릿인 `std::make_unique` 내부로 전달되는 과정에서 파서가 인자 전달 요구 조건을 오탐하는 현상임.
* C++11 위임 생성자(Delegating Constructor)를 명시하여 기본 파일명(`"config.ini"`)을 클래스 내부로 캡슐화함.

```cpp
// settingsmanager.h
class SettingsManager {
public:
    // 기본 생성자: 내부에서 기본 파일 경로 생성자로 위임
    SettingsManager() : SettingsManager("config.ini") {}
    
    explicit SettingsManager(const QString& filePath);
    // ...
};
```

* **효과**: `MainWindow`가 구체적인 설정 파일명(`config.ini`)을 알 필요가 없어 결합도(Coupling)가 낮아지고, 모든 컴파일러 및 IDE 정적 분석기에서 경고 없이 완벽히 빌드됨.