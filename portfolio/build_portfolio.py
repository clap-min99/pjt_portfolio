"""Generate the portrait portfolio from repository evidence and confirmed contributions."""
from pathlib import Path
import sys
import html

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'portfolio'
sys.path.insert(0, str(ROOT / '.portfolio-tools'))
from reportlab.pdfgen import canvas
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import Paragraph
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.colors import HexColor, white
from reportlab.lib.utils import ImageReader
from PIL import Image

pdfmetrics.registerFont(TTFont('KR', 'C:/Windows/Fonts/malgun.ttf'))
pdfmetrics.registerFont(TTFont('KRB', 'C:/Windows/Fonts/malgunbd.ttf'))
W, H = 595.276, 841.89
L, CW = 45, W - 90
INK, MUTED, BLUE = '#16283D', '#58677A', '#126E82'
PALE, LINE = '#EDF5F6', '#DCE5EB'
PDF = OUT / '박수민_마키나락스_Vision_포트폴리오.pdf'
c = canvas.Canvas(str(PDF), pagesize=(W, H), pageCompression=1)
c.setTitle('박수민 | Vision AI Portfolio | 마키나락스 FDE')
c.setAuthor('박수민')
c.setSubject('Anomaly Detection · Object Detection · Edge AI')
notes, boxes = [], []
page = 0


def text(s, x, y, size=10, bold=False, color=INK, width=CW, leading=None):
    style = ParagraphStyle('p', fontName='KRB' if bold else 'KR', fontSize=size,
                           leading=leading or size * 1.65, textColor=HexColor(color),
                           wordWrap='CJK', spaceAfter=0)
    p = Paragraph(s, style)
    _, ph = p.wrap(width, H)
    assert y + ph < (H - 15 if y >= 800 else H - 50), (page, s[:60], y, ph)
    p.drawOn(c, x, H - y - ph)
    boxes.append((page, x, y, width, ph))
    notes.append(html.unescape(s.replace('<br/>', '\n')))
    return y + ph


def rect(x, y, w, h, fill=PALE, radius=10):
    c.setFillColor(HexColor(fill))
    c.roundRect(x, H-y-h, w, h, radius, fill=1, stroke=0)


def rule(y):
    c.setStrokeColor(HexColor(LINE))
    c.line(L, H-y, W-L, H-y)


def start(kicker, title, sub):
    global page
    if page:
        c.showPage()
    page += 1
    notes.append('\n\n---\n\n# ' + title)
    c.setFillColor(HexColor(BLUE))
    c.rect(0, H-8, W, 8, fill=1, stroke=0)
    text(kicker, L, 31, 9, True, BLUE)
    text(title, L, 60, 25, True, width=CW, leading=34)
    text(sub, L, 107, 10, color=MUTED)
    rule(145)
    text('박수민  |  VISION AI PORTFOLIO', L, 803, 7.5, color=MUTED, width=400)
    text(f'{page:02d} / 10', W-95, 803, 7.5, color=MUTED, width=55)


def section(title, body, y, width=CW, x=L):
    y = text(title, x, y, 12, True, width=width) + 7
    return text(body, x, y, 10, width=width) + 19


def bullet(label, body, y):
    text(label, L, y, 11, True, BLUE, width=115)
    return text(body, L+125, y, 10, width=CW-125) + 17


def picture(path, x, y, w, h):
    im = Image.open(path).convert('RGB')
    iw, ih = im.size
    scale = min(w/iw, h/ih)
    dw, dh = iw*scale, ih*scale
    c.drawImage(ImageReader(im), x+(w-dw)/2, H-y-dh, dw, dh, mask='auto')


def flow(items, y, h=67):
    gap = 14
    width = (CW-gap*(len(items)-1))/len(items)
    for i, (label, body) in enumerate(items):
        x = L+i*(width+gap)
        rect(x, y, width, h)
        text(label, x+10, y+10, 10, True, BLUE, width=width-20)
        text(body, x+10, y+30, 8, width=width-20, leading=12)
        if i < len(items)-1:
            text('›', x+width+4, y+20, 14, True, BLUE, width=12)


def link(label, url, y, x=L, width=CW):
    return text(f'<link href="{html.escape(url, quote=True)}" color="{BLUE}">{label}</link>', x, y, 9, width=width)


# 01 — Cover
start('M A K I N A R O C K S  /  F D E · V I S I O N', '박수민', 'Forward Deployed Engineer - Vision 지원')
text('Computer Vision<br/>Project Portfolio', L, 176, 31, True, leading=45)
text('PCB 이상탐지 · 실시간 객체 탐지 · 이미지 기반 회귀<br/>데이터 수집·전처리 / 모델 학습 / 추론 최적화 / 장비 연동', L, 292, 12, leading=21)
rect(L, 365, CW, 270)
picture(OUT/'board_front.jpg', L+18, 379, 140, 239)
text('PROJECT HIGHLIGHTS', L+183, 389, 9, True, BLUE, width=270)
text('BoardGuard', L+183, 420, 20, True, width=270)
text('PCB 불량 검사·자동 배출 시스템<br/>PatchCore · Jetson–MCU 연동', L+183, 460, 10, width=270)
text('RPS_YOLO', L+183, 526, 16, True, width=270)
text('실시간 가위바위보 인식·판정 시스템<br/>YOLO 파인튜닝 · TensorRT FP16', L+183, 558, 10, width=270)
text('경북대학교 전자재료공학과  ·  ADsP  ·  품질경영기사<br/>OPIc IH  ·  JPT 420점', L, 666, 10)
link('jenny11994@naver.com', 'mailto:jenny11994@naver.com', 701)
link('github.com/clap-min99', 'https://github.com/clap-min99', 725)

# 02 — Profile
start('PROFILE / CAPABILITIES', '개발 역량 및 교육 이력', 'Computer Vision / Edge AI / Embedded Systems')
y = section('개발 분야', 'SSAFY 웹·모바일·AI 서비스 개발 및 AI시스템반도체SW개발자 과정의 임베디드 시스템 프로젝트 수행. PCB 이상탐지, 객체 탐지, 이미지 기반 회귀 모델 실험과 Jetson 기반 추론·장비 연동 경험.', 171)
y = bullet('데이터 · 모델', 'PCB 이미지 직접 촬영·정리·전처리 및 PatchCore 모델 선정·학습. YOLO 객체 탐지 파인튜닝, XGBoost 초기 회귀 실험.', y)
y = bullet('추론 · 적용', 'Jetson Orin Nano 기반 TensorRT 추론 구현. 카메라 입력, 전처리·후처리, 결과 표시 및 판정 로직 연동.', y)
y = bullet('시스템 연동', 'BoardGuard Jetson–MCU 신호 오인식 해결 참여. 펌웨어·드라이버 프로젝트의 하드웨어 입출력 및 시스템 통합.', y)
rule(y+1)
y = section('교육', 'SSAFY  |  2024.07.02 – 2025.06.30<br/>웹·모바일 서비스 팀 프로젝트, 프론트엔드와 AI 모델 개발 경험<br/><br/>AI시스템반도체SW개발자 과정  |  2026.05.27 – 2026.12.16 수료 예정<br/>AVR·STM32 펌웨어, Jetson AI 추론, Linux 디바이스 드라이버', y+22)
y = section('학력 · 자격 · 어학', '학력: 경북대학교 전자재료공학과<br/>자격: ADsP · 품질경영기사<br/>어학: OPIc IH · JPT 420점', y)
text('Python / PyTorch / anomalib / OpenCV / TensorRT / C / C++', L, y, 9, True, BLUE)

# 03 — BoardGuard overview
start('01 / BOARDGUARD', 'BoardGuard | PCB 불량 검사 시스템', '4인 · 2주  |  팀장 / AI 모델링  |  Jetson Orin Nano + STM32F411RE')
y = section('프로젝트 개요', '컨베이어 기반 PCB 양품·불량 판별 및 불량품 자동 배출 시스템. 앞면·뒷면별 PatchCore 모델을 통한 이상탐지와 GPIO 기반 MCU 이송·배출 제어 연동.', 168)
flow([('입력', '카메라 영상\nPCB 유무 확인'), ('모델', 'PatchCore\n양품·불량 판정'), ('결과', '결함 위치 표시\n판정 기록'), ('장비', 'GPIO → MCU\n이송·배출')], y, 77)
y += 101
picture(OUT/'board_front.jpg', L, y, 165, 293)
picture(OUT/'board_back.jpg', L+175, y, 165, 293)
text('담당 업무', L+357, y+3, 12, True, BLUE, width=148)
text('PCB 데이터 촬영<br/>데이터 정리·전처리<br/>모델 선정·학습<br/>Jetson 신호 오류 해결<br/>팀장 역할', L+357, y+38, 10, width=148, leading=22)
text('팀 구현 범위', L+357, y+176, 11, True, width=148)
text('검사 화면·기록<br/>결함 유형 분류<br/>MCU 이송·배출 제어', L+357, y+205, 9, width=148, leading=20)
text('실제 동작 시연 영상에서 추출한 앞면·뒷면 검사 장면', L, y+304, 8, color=MUTED)
link('프로젝트 상세 · 시연 보기 ↗', 'https://github.com/clap-min99/pjt_portfolio/tree/main/BoardGuard', y+331)

# 04 — Data and modeling
start('01 / BOARDGUARD · DATA & MODEL', 'BoardGuard | 데이터 및 모델링', '담당 업무  |  PCB 데이터 촬영·정리·전처리 / 모델 선정·학습')
y = section('01  데이터 수집 및 구성', 'PCB 이미지 직접 촬영 및 앞면·뒷면, 정상·불량별 데이터 정리. normal / abnormal 디렉터리 기반 수집 데이터 관리. 앞·뒷면의 외형 차이를 반영한 모델 및 판정 임계값 분리.', 168)
y = section('02  PatchCore 모델 선정 및 학습', '정상 이미지 특징을 기준으로 이상 점수를 산출하는 PatchCore 적용. anomalib·PyTorch 기반 학습 노트북 구성 및 모델 학습. 최종 채택 모델: wide_resnet50_2 기반 model_03 (프로젝트 README 기록 기준).', y)
y = section('03  배포 파이프라인의 전처리 및 판정', '256×256 입력 크기 변환 및 모델 입력 축 정렬. anomaly score 기준 앞면 0.55, 뒷면 0.40의 판정 임계값 적용. anomaly map 기반 결함 영역 산출.', y)
text('프로젝트 내부 평가 기록', L, y, 12, True)
y += 33
rows = [('구분', '정상 / 불량 이미지', 'AUROC', 'F1', '임계값'),
        ('앞면', '479 / 50장', '1.000', '1.000', '0.55'),
        ('뒷면', '489 / 63장', '0.999', '1.000', '0.40')]
widths = [60, 179, 95, 80, 91]
for r, row in enumerate(rows):
    rect(L, y+r*38, CW, 36, PALE if r==0 else '#F6F8FA', 3)
    x = L+10
    for j, item in enumerate(row):
        text(item, x, y+r*38+9, 9, r==0, width=widths[j]-12)
        x += widths[j]
y += 129
text('출처: BoardGuard/README.md, model_03 평가 표. 이미지 수: README의 데이터 기록 기준 (독립 테스트셋 수와 구분). 평가 범위: 프로젝트 촬영 환경의 내부 평가. 다른 현장에 대한 일반화 성능은 별도 검증 대상.', L, y, 8, color=MUTED, leading=13)

# 05 — Field troubleshooting
start('01 / BOARDGUARD · FIELD INTEGRATION', 'BoardGuard | GPIO 연동 오류 개선', '담당 업무  |  Jetson–STM32 신호 오인식 원인 점검 및 개선 참여')
y = section('문제 현상  |  인접 GPIO 신호 오인식', 'Jetson의 PASS 단일 HIGH 신호 전송 시 STM32의 인접 STOP·FAIL 핀에서도 HIGH 인식. 복수 인터럽트 발생에 따른 검사 결과 전달 및 장비 제어 오류.', 170)
flow([('Jetson', 'STOP / PASS / FAIL\nGPIO 3개 신호'), ('신호선 구간', '모터 구동 환경에서\n인접 핀 오인식'), ('STM32', 'EXTI 이벤트\n상태 머신 제어')], y, 79)
y += 106
y = section('원인 분석  |  장비 연결 조건별 신호 확인', 'Jetson 단독 신호 발생 시 정상 동작 확인. 모터·모터드라이버 연결 환경과 비교 후 신호선 유입 노이즈를 원인으로 판단. Jetson–STM32 배선 구간 점검.', y)
y = section('개선 조치 및 결과  |  10kΩ 저항 추가', 'Jetson–STM32 신호선 구간에 10kΩ 저항 추가. 조치 후 인접 핀 오인식 해소 및 프로젝트 장비 동작 확인. 원인과 해결 과정의 트러블슈팅 문서화.', y)
rect(L, y, CW, 103)
text('시스템 검증 항목', L+17, y+14, 12, True, BLUE, width=CW-34)
text('모델 판정 결과와 MCU 입력 신호 간 일치 여부 확인. 모터 구동 환경의 신호 전달 안정성 점검을 통한 AI 검사 파이프라인과 물리 장비의 연동 검증.', L+17, y+44, 10, width=CW-34)
text('검증 범위: 프로젝트 장비 동작 확인. 노이즈 파형·오류율의 정량 계측 자료 미포함.', L, y+123, 8, color=MUTED)

# 06 — Daldidan
start('02 / DALDIDAN', '달디단 | 사과 당도 예측 서비스', 'SSAFY · 6인 · 2025.04.14 – 2025.05.22  |  초기 모델 실험 / 프론트엔드')
y = section('프로젝트 개요', '스마트폰 카메라 기반 사과 인식 및 이미지의 시각적 특성을 활용한 당도(Brix) 예측 서비스. 이미지 촬영, AI 추론 요청 및 결과 시각화로 구성된 모바일 서비스 개발.', 170)
rect(L, y, CW, 131)
text('담당 업무', L+18, y+15, 12, True, BLUE, width=CW-36)
text('• XGBoost 기반 초기 당도 예측 모델 실험<br/>• 초기 YOLOv8n 기반 사과 객체 인식 모델 개발 참여<br/>• 프론트엔드 사과 인식 기능·당도 페이지 구현<br/>• UX/UI 디자인 참여', L+18, y+45, 10, width=CW-36, leading=20)
y += 157
y = section('모델 실험 및 서비스 구현 범위', '개인 담당: XGBoost 기반 초기 당도 예측 모델 실험 및 프론트엔드 기능 개발.<br/>팀 최종 모델: EfficientNet 특징과 수작업 특징을 결합한 CNN·MLP 구조.<br/>초기 회귀 모델 실험과 예측 결과 화면 개발 수행.', y)
flow([('사용자', '카메라 촬영\n사과 인식'), ('AI 서비스', '사과 영역 추출\n당도 추론'), ('화면', 'Brix 예측값\n사용자에게 시각화')], y, 78)
y += 105
y = section('프로젝트 수행 범위', '농산물 이미지 기반 회귀 문제의 초기 모델 실험. 사과 인식 및 당도 결과 페이지 구현을 통한 모델 출력과 모바일 서비스 UI 연동.', y)
link('달디단 저장소 · 팀 역할 및 서비스 구조 ↗', 'https://github.com/clap-min99/daldidan', y)

# 07 — RPS overview
start('03 / RPS_YOLO', 'RPS_YOLO | 가위바위보 판정 시스템', '개인 · 1일  |  담당: 모델 파인튜닝 / 추론 구현 / 판정 로직 개발')
y = section('프로젝트 개요', '단일 카메라 기반 2인 실시간 가위바위보 인식·승패 판정 시스템. YOLOv11n을 통한 손 영역 탐지 및 제스처 분류, TensorRT FP16 기반 Jetson 추론 구현.', 168)
picture(OUT/'rps.jpg', L, y, 230, 250)
text('학습 구성', L+254, y+5, 12, True, BLUE, width=251)
text('데이터: Roboflow Universe<br/>rock-paper-scissors-sxsw v14<br/>클래스: paper / rock / scissors<br/>사전학습 모델: yolo11n.pt<br/>학습: 80 epochs · imgsz 320<br/>환경: Google Colab GPU', L+254, y+39, 10, width=251, leading=22)
y += 272
flow([('영상 입력', '320×240\n카메라 프레임'), ('전처리', 'Letterbox\n320×320 / NCHW'), ('추론', 'TensorRT FP16\nconfidence · NMS'), ('서비스', '두 손 매칭\n안정화 · 승패 표시')], y, 78)
y += 105
rect(L, y, CW, 77)
text('mAP50 ≈ 0.95', L+15, y+13, 17, True, BLUE, width=240)
text('GPU latency ≈ 2.1ms', L+260, y+13, 17, True, BLUE, width=230)
text('mAP50–95 ≈ 0.72 · README 학습 결과', L+15, y+47, 8, width=240)
text('trtexec 기준 · 전체 영상 처리 시간과 구분', L+260, y+47, 8, width=230)
link('학습 노트북 · 추론 코드 · 시연 보기 ↗', 'https://github.com/clap-min99/pjt_portfolio/tree/main/RPS_yolo', y+98)

# 08 — RPS engineering
start('03 / RPS_YOLO · INFERENCE ENGINEERING', 'RPS_YOLO | 추론 최적화 및 안정화', '담당 업무  |  모델 변환 / 입력·출력 정합성 개선 / 프레임 간 판정 안정화')
y = section('01  TensorRT 추론 경로 구성', '학습 모델의 ONNX 내보내기 및 Jetson용 TensorRT FP16 엔진 변환. TensorRT 10.x와 PyCUDA pagelocked·mapped 메모리 기반 입출력 버퍼 연결. 명시적 CPU–GPU 복사 없는 추론 래퍼 구성.', 170)
y = section('02  입력 텐서 및 탐지 좌표 정합성 개선', '초기 NHWC 입력과 엔진 요구 형식 NCHW 간 불일치 해결을 위한 축 변환 적용. Letterbox 비율·패딩 정보를 이용한 원본 영상 좌표 복원. confidence 필터 및 NMS 기반 중복 박스 제거.', y)
y = section('03  다수결 기반 프레임 간 판정 안정화', '연속 프레임 일치 방식의 순간 미검출에 따른 카운트 초기화 문제 개선. 최근 20프레임의 손 조합을 누적하고 동일 조합 12회 이상 관측 시 결과를 확정하는 다수결 로직 적용.', y)
flow([('관측', '최근 20프레임\n손 조합 누적'), ('집계', '가장 빈번한 조합\n12회 이상 일치'), ('확정', '승패 계산\n화면 결과 표시')], y, 78)
y += 105
y = section('추론 성능 및 판정 안정성', 'FP16 엔진·메모리 경로 구성을 통한 추론 최적화. 프레임 간 집계 기반 순간 미검출·탐지 변동 대응. 모델 추론과 결과 확정 로직의 분리 구현.', y)
text('측정 범위: 약 2.1ms는 trtexec GPU latency 기록. 카메라 수신·전처리·후처리·렌더링을 포함하는 End-to-end 지연과 구분.', L, y, 8, color=MUTED)

# 09 — Supporting experience
start('SUPPORTING EXPERIENCE / SYSTEMS', '임베디드 시스템 및 서비스 프로젝트', '보조 프로젝트  |  Linux 드라이버 · MCU 제어 · 서비스 개발')
y = section('SmartFan  |  Linux 드라이버 기반 팬 제어 시스템', '4인 · 3일. 초음파 센서·엔코더 입력 및 팬 PWM·LED 출력 연동. Character Device의 read() / write() 기반 커널·사용자 프로그램 통신. 상태 머신 기반 자동·수동 팬 제어.', 170)
text('C · GPIO/IRQ · Completion · spinlock · PWM Framework · FSM', L, y-7, 8, True, BLUE)
y += 20
y = section('Smart Gas Monitoring  |  가스 누출 감지·원격 관제 시스템', '4인 · 5일. 가스 누출 감지에 따른 밸브 차단·환기·경보 동작 구현. UART·TCP 기반 STM32 레지스터 펌웨어, Qt 관제 및 Android 클라이언트 연동.', y)
text('Bare-metal C · ADC · PWM · UART · Qt/C++ · TCP · Android', L, y-7, 8, True, BLUE)
y += 20
y = section('AVR 개인 프로젝트  |  센서 입력 및 FSM 기반 제어', 'AutoCar: 초음파 센서 3개 기반 장애물 감지 및 수동·자율주행 RC카.<br/>WasherFSM: 세탁·헹굼·탈수 순차 제어를 위한 상태 머신.<br/>LCD_CAL_RTC: 키패드 계산기 및 I2C 기반 RTC 시계.', y)
y = section('서비스 개발 경험', 'izi_auto: 피아노 스튜디오 예약·입금 확인·문자 발송 자동화. 프론트엔드, 문자 발송 로직, 외부 API 연동과 운영 이슈 대응 담당.<br/>SSD Study App: 뉴스 수집과 개념 학습·복습을 위한 개인 웹 앱 기획·개발·배포.', y)
text('SmartFan·GasMonitoring: 팀 프로젝트 전체 구현 범위 기준. 핵심 비전 프로젝트의 개인 담당 업무는 해당 프로젝트 페이지에 별도 표기.', L, y, 8, color=MUTED)

# 10 — Evidence index
start('PROJECT INDEX / CONTACT', '프로젝트 저장소 및 연락처', '프로젝트별 소스 코드 / 학습 노트북 / 시연 영상 / 기술 문서')
repos = [
    ('01  BoardGuard', '학습·데이터 수집 코드 / 모델 평가 기록 / PCB 검사 시연 / GPIO 트러블슈팅', 'https://github.com/clap-min99/pjt_portfolio/tree/main/BoardGuard'),
    ('02  달디단', '서비스 아키텍처 / 최종 모델 구조 / 팀원별 담당 역할', 'https://github.com/clap-min99/daldidan'),
    ('03  RPS_YOLO', 'YOLO 학습 노트북 / TensorRT 래퍼 / 전처리·후처리 / 실시간 시연', 'https://github.com/clap-min99/pjt_portfolio/tree/main/RPS_yolo'),
    ('04  임베디드 프로젝트 모음', 'Linux 드라이버 / MCU 펌웨어 / 회로·핀맵 / 시스템 연동 문서', 'https://github.com/clap-min99/pjt_portfolio'),
]
y = 173
for title, body, url in repos:
    text(title, L, y, 13, True)
    text(body, L, y+30, 9, color=MUTED)
    link('저장소에서 자세히 보기 ↗', url, y+56)
    rule(y+87)
    y += 108
rect(L, 628, CW, 119)
text('박수민', L+18, 642, 18, True, width=CW-36)
link('jenny11994@naver.com', 'mailto:jenny11994@naver.com', 683, L+18, CW-36)
link('github.com/clap-min99', 'https://github.com/clap-min99', 707, L+18, CW-36)
c.save()
assert page == 10
(OUT/'포트폴리오_본문.md').write_text('\n\n'.join(notes), encoding='utf-8')
print(str(PDF))

# Verify dimensions, text extraction, links and content safety; render review images.
import fitz
doc = fitz.open(PDF)
assert len(doc) == 10
all_text = ''.join(p.get_text() for p in doc)
assert all(term in all_text for term in ['박수민', 'PatchCore', 'XGBoost', '10kΩ'])
assert all(abs(p.rect.width-W) < 1 and abs(p.rect.height-H) < 1 for p in doc)
assert sum(len(p.get_links()) for p in doc) >= 10
for i, p in enumerate(doc):
    assert len(p.get_text().strip()) > 200, (i, 'missing text')
    p.get_pixmap(matrix=fitz.Matrix(0.9, 0.9)).save(OUT/f'page_{i+1:02}.png')
thumbs = Image.new('RGB', (5*300, 2*438), '#dce5eb')
for i in range(len(doc)):
    im = Image.open(OUT/f'page_{i+1:02}.png')
    im.thumbnail((288, 420))
    thumbs.paste(im, ((i%5)*300+6, (i//5)*438+6))
thumbs.save(OUT/'전체_미리보기.jpg')
print(f'CHECKED: {len(doc)} portrait A4 pages; searchable Korean text; working PDF link annotations.')
