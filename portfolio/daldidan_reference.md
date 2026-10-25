# 🍎 AI 기반 사과 당도 예측 서비스, **달디단**

## 📅 프로젝트 진행 기간

**2025.04.14 ~ 2025.05.22 (6주)**

## 📑 프로젝트 기획 및 회의 노션

[🔗 달디단 프로젝트 Notion 바로가기](https://laced-brand-a7e.notion.site/3-1d5cb8da549480b79cbce87e4c00a1c3)

---

## 🌟 서비스 한줄 소개

> **AI 기반 사과 당도 예측 서비스**

---

## 📝 개요

**달디단**(**Daldidan**)은 로그인을 하지 않아도 스마트폰 카메라를 통해 간편하게 **사과의 당도를 예측**할 수 있는 서비스입니다.
YOLO 기반 객체 탐지와 CNN 기반 회귀 모델을 통해 사과의 당도를 정밀하게 분석하여 사용자에게 실시간으로 결과를 제공합니다.

---

## 🎯 프로젝트 목표

- 📷 카메라 화면에서 사과를 실시간으로 인식
- 📡 인식된 사과 정보를 AI 서버로 전송
- 🧠 AI 서버에서 사과만 포함된 이미지를 분리 및 전처리
- 🔬 사과의 시각적 특성을 분석하여 당도(Brix) 예측
- 📲 예측 결과를 사용자 친화적으로 시각화하여 제공

---

## 🛠️ 기술 스택

### Frontend

- React Native
- Expo
- Frame Processor
- EfficientDet-lite0 (TFLite)

### Backend

- FastAPI
- PyTorch
- YOLOv8, EfficientNet

### Infra

- Gitlab
- Jenkins: GitLab 연동 자동 빌드 & 배포 파이프라인 구축
- Docker: FastAPI 백엔드 컨테이너화
- AWS EC2: Jenkins, 백엔드 서버 등 배포 환경 운영
- Nginx: 리버스 프록시 및 HTTPS 인증 설정

---

## 🚀 주요 기능

### 1. AI 기반 사과 탐지 및 당도 예측

- 다중 사과 인식 및 개별 당도 분석
- RGB 색상, 광택, 크기 기반 시각 특징 분석

### 2. 직관적인 당도 시각화

- 터치 시 예측된 당도 값을 시각적으로 표시
- 실시간 결과 제공

### 3. 모바일 중심 접근성

- 스마트폰만으로 사용 가능 (기기 불필요)
- Android 지원 (iOS는 추후 예정)

### 4. REST API 기반 실시간 예측

- YOLOv8 + CNN+MLP 조합 예측을 FastAPI로 처리
- 단일 이미지 요청에 빠른 응답 제공

### 5. 비회원 사용 모델

- 로그인 없이 예측 기능 사용 가능
- 향후 간단한 기록/저장 기능 제공 예정

---

## 🧪 주요 기술

### 1. YOLOv8l-seg 기반 사과 인식

- COCO 기반 사전학습된 YOLOv8l-seg 모델 사용
- Polygon segmentation mask 추출
- 잘린/겹치는 사과 필터링 로직 구현

### 2. CNN 기반 특징 추출 및 병합

- EfficientNet-B0의 최종 임베딩 벡터(1280차원)를 추출 (Classifier 제거 후 feature extractor로 사용)
- RGB, YCbCr, GLCM 기반의 수작업 시각 특징 6차원과 병합
- 총 1286차원의 벡터를 Fully Connected Layer에 입력

### 3. PyTorch 기반 MLP 회귀 모델

- CNN 임베딩 + 수작업 feature를 concat한 벡터를 기반으로 브릭스(Brix) 값을 회귀 방식으로 예측
- 1286 → 128 → 1 구조의 MLP(다층 퍼셉트론)로 학습
- CNN은 전체 학습 가능하도록 설정 (전이학습 기반 End-to-End 회귀 모델)

### 4. 실시간 모바일 객체 인식 최적화

- FrameProcessor 사용, 프레임 단위 연산 수행
- EfficientDet-lite0 (양자화 모델)로 경량화

### 5. FastAPI 기반 AI 예측 서버

- 이미지 업로드 → YOLO 탐지 → Crop → CNN+MLP 추론
- REST API 구조로 프론트 연동 최적화
- Dispatcher 구조로 추론 로직 분리 → 유지보수 용이

---

## 📸 서비스 화면 예시

- 측정 대기 화면
- 사과즙 화면
- 결과 화면

---

## 👨‍👩‍👧‍👦 팀원

| 이름   | 역할                        |
| ------ | --------------------------- |
| 최진문(팀장) | Frontend, AI, Infra |
| 박수민 | Frontend, AI               |
| 이원재 | Frontend, AI               |
| 하건수 | Frontend, AI               |
| 전민경 | Backend, AI, Infra        |
| 정한균 | Backend, AI                |

## 📌 역할 및 담당 업무

### 🎨 Front-End

#### 최진문(팀장) [Frontend]

- 프론트측 사과 객체 인식 모델(EfficientDet lite-0.tflite) 적용
- 실시간 카메라뷰 프레임 연산을 위한 FrameProcessor 적용
- 당도 페이지 구현
- Infra (expo 앱 배포)

#### 박수민 [Frontend]

- 프론트측 사과 객체 인식 모델
- 초기 당도 예측 모델 개발(XGBoost)
- 초기 프론트측 사과 객체 인식 모델(YOLOv8n)
- UX/UI 디자인
- 당도 페이지 구현

#### 이원재 [Frontend]

- 프론트측 사과 객체 인식 모델
- 초기 프론트측 사과 객체 인식 모델(YOLOv8n)
- 로딩 페이지 구현
- 당도 페이지 구현

#### 하건수 [Frontend]

- 프론트측 사과 객체 인식 모델
- 초기 프론트측 사과 객체 인식 모델(YOLOv8n)
- UX/UI 디자인
- WebScocket 구현
- 당도 페이지 구현

### 🖥️ Back-End

#### 전민경 [Backend]

- 당도 예측 모델 개발 (CNN + 수작업 특징 기반 Fusion 모델 구현, Fine-tuning 및 Feature 확장 실험)
- 초기 당도 예측 모델 개발(Linear Regression)
- API 구현
- Infra (백엔드 배포)

#### 정한균 [Backend]

- 백엔드측 사과 객체 인식 모델
- 초기 당도 예측 모델 개발(EfficientNet-B0+LightGBM, LightGBM)
- FastAPI 구조 설계
- API 구현

---

## 📌 향후 개선 방향

- 사과 품종 확대 (후지 → 홍로, 아리수, 시나노골드 등 다양한 품종 지원)
- 사과 신선도 예측 기능 추가 (부패 감지, 저장일 추정 등)
- 기타 과일(수박, 귤 등)의 당도 예측 기능 추가
- 육류 이미지 기반 고기 부위 판별 및 등급 분류 기능 확장

---

## 🌐 시스템 아키텍처

![시스템 아키텍쳐](./readme_assets/system_architecture.jpg)

---

## 📂 프로젝트 구조

### Backend

```
📁 BE/
├── .gitignore
├── README.md
├── requirements.txt
├── requirements-gpu.txt

├── ai/
│   ├── main.py
│   ├── requirements-total.txt
│   ├── api/v1/routes.py
│   ├── common_utils/image_cropper.py
│   ├── schemas/predict.py
│   ├── scripts/
│   │   ├── move_invalid_pairs.py
│   │   ├── only_image_embedding.py
│   │   ├── predict_sugar.py
│   │   ├── sample_dataset.py
│   │   ├── test_predict_api.py
│   │   └── validate_dataset.py
│   ├── services/
│   │   ├── predict_service.py
│   │   ├── detect_service.py
│   │   ├── cnn_feature_maskcrop_seg/
│   │   │   ├── create_scaler.py
│   │   │   ├── train.py
│   │   │   ├── model_loader.py
│   │   │   ├── predictor.py
│   │   │   ├── fusion_model.py
│   │   │   ├── extract_features.py
│   │   │   └── apple_dataset.py
│   │   ├── yolov8/
│   │   │   ├── inference/
│   │   │   │   ├── backend_tflite.py
│   │   │   │   ├── predictor.py
│   │   │   │   └── predictor_pt.py
│   │   │   ├── models/
│   │   │   │   ├── yolov8l-seg.pt
│   │   │   │   ├── yolov8n_seg_float32.tflite
│   │   │   │   └── yolov8n_bbox_int8.tflite
│   │   │   └── scripts/
│   │   │       ├── bbox_crop.py
│   │   │       ├── check_outputs.py
│   │   │       └── draw_boundingbox.py

├── be/
│   ├── Dockerfile
│   ├── main.py
│   ├── api/v1/routes.py
│   ├── core/config.py
│   ├── schemas/
│   │   ├── api.py
│   │   └── socket.py
│   ├── services/remote_model.py
│   ├── socketio_app/
│   │   ├── events.py
│   │   └── manager.py
│   └── utils/image.py

```

---

### Frontend

```
📁 FE/
└── daldidan/
    ├── app/                      # 라우팅 및 레이아웃 구성
    │   ├── index.tsx
    │   └── _layout.tsx
    ├── assets/                   # 이미지, Lottie, 사운드 등 리소스
    │   ├── images/
    │   ├── fonts/
    │   ├── lottie/
    │   └── sounds/
    ├── components/              # 공통 UI 컴포넌트
    │   ├── AppleBar.tsx
    │   ├── AppleButton.tsx
    │   ├── CaptureOverlay.tsx
    │   └── ui/IconSymbol.tsx
    ├── constants/               # API, 모델 설정, 색상 등 상수 정의
    │   ├── api.ts
    │   ├── model.ts
    │   └── Colors.ts
    ├── hooks/                   # 커스텀 훅
    │   ├── useObjectDetection.ts
    │   ├── useImageProcessing.ts
    │   └── useAnalysisApiHandler.ts
    ├── android/                 # 안드로이드 네이티브 설정
    │   └── app/
    │       ├── src/main/java/.../MainActivity.kt
    │       └── res/drawable-*/splashscreen_logo.png
    ├── types/                   # 타입 선언
    │   └── sound.d.ts
    ├── scripts/
    │   └── reset-project.js
    ├── app.json
    ├── package.json
    └── tsconfig.json
```
