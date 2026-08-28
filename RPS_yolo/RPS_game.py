# RPS_game_YOLO.py
# YOLO 기반 실시간 가위바위보 대전 게임 (젯슨 오린 나노)
#
# 흐름: 카메라 프레임 -> letterbox 정사각형 변환 -> TensorRT YOLO 추론
#       -> 디코딩 + NMS -> 손 2개(P1/P2) 배정 -> 안정성 기반 자동 판정

import numpy as np
import time
import cv2
from collections import deque, Counter
import pycuda.autoinit  # PyCUDA 컨텍스트 자동 관리 초기화용

from trt_module import TRTInferenceEngine

# ==================== 설정 값 ====================
ENGINE_PATH = 'RPS_YOLO.engine'      # trtexec로 변환한 엔진 파일
IMG_SIZE = 320                        # 학습 시 imgsz와 동일하게
NUM_CLASSES = 3

# data.yaml 의 names 순서를 그대로 반영 (['Paper', 'Rock', 'Scissors'])
CLASS_NAMES = ['paper', 'rock', 'scissors']
COLOR_LIST = [(60, 200, 255), (100, 220, 120), (255, 140, 60)]  # 주황빛/초록/파랑 계열 (BGR)

CONF_THRES = 0.5
NMS_THRES = 0.45

STABLE_WINDOW = 20           # 최근 이 프레임 수만큼을 버퍼에 담아서 판단
STABLE_MATCH_COUNT = 12      # 버퍼 안에서 같은 조합이 이 횟수 이상 나오면 확정 (한두 프레임 노이즈는 무시됨)
MISS_RESET_LIMIT = 8         # 손이 2개 아닌 상태가 이만큼 연속되면 버퍼 초기화
RESULT_HOLD_FRAMES = 60      # 결과 화면 유지 프레임 수 (약 2초, 30fps 기준)

BEATS = {'rock': 'scissors', 'scissors': 'paper', 'paper': 'rock'}

# ---- 화면 표시(UI) 관련 설정 ----
DISPLAY_SCALE = 2            # 캡처 해상도(320x240) 대비 표시 배율 (선명하게 키워서 보여줌)
HUD_COLOR = (40, 40, 40)     # HUD 패널 배경색
ACCENT_COLOR = (60, 200, 255)
WIN_COLOR = (120, 230, 120)
DRAW_COLOR = (200, 200, 90)
TEXT_COLOR = (255, 255, 255)

# ==================== 초기화 ====================
trt_engine = TRTInferenceEngine(ENGINE_PATH)


def letterbox(frame, size):
    """비율 유지한 채 정사각형(size x size)으로 패딩."""
    h, w = frame.shape[:2]
    scale = size / max(h, w)
    nw, nh = int(w * scale), int(h * scale)
    resized = cv2.resize(frame, (nw, nh))

    canvas = np.full((size, size, 3), 114, dtype=np.uint8)
    top = (size - nh) // 2
    left = (size - nw) // 2
    canvas[top:top + nh, left:left + nw] = resized

    return canvas, scale, left, top


def postprocess(output_flat, num_classes, conf_thres, nms_thres):
    """YOLO raw 출력(4+num_classes, num_anchors) 디코딩 + NMS."""
    output = output_flat.reshape(4 + num_classes, -1).T

    boxes_cxcywh = output[:, :4]
    scores = output[:, 4:4 + num_classes]
    class_ids = np.argmax(scores, axis=1)
    confidences = np.max(scores, axis=1)

    mask = confidences > conf_thres
    boxes_cxcywh = boxes_cxcywh[mask]
    class_ids = class_ids[mask]
    confidences = confidences[mask]

    if len(boxes_cxcywh) == 0:
        return []

    boxes_tlwh = np.zeros_like(boxes_cxcywh)
    boxes_tlwh[:, 0] = boxes_cxcywh[:, 0] - boxes_cxcywh[:, 2] / 2
    boxes_tlwh[:, 1] = boxes_cxcywh[:, 1] - boxes_cxcywh[:, 3] / 2
    boxes_tlwh[:, 2] = boxes_cxcywh[:, 2]
    boxes_tlwh[:, 3] = boxes_cxcywh[:, 3]

    indices = cv2.dnn.NMSBoxes(
        boxes_tlwh.tolist(), confidences.tolist(), conf_thres, nms_thres
    )

    results = []
    if len(indices) > 0:
        for i in np.array(indices).flatten():
            x, y, w, h = boxes_tlwh[i]
            results.append({
                'box': (x, y, x + w, y + h),
                'class_id': int(class_ids[i]),
                'conf': float(confidences[i]),
            })
    return results


def to_original_coords(box, scale, left, top):
    x1, y1, x2, y2 = box
    ox1 = (x1 - left) / scale
    oy1 = (y1 - top) / scale
    ox2 = (x2 - left) / scale
    oy2 = (y2 - top) / scale
    return int(ox1), int(oy1), int(ox2), int(oy2)


def judge(g1, g2):
    if g1 == g2:
        return 'DRAW'
    return 'P1' if BEATS[g1] == g2 else 'P2'


# ==================== UI 헬퍼 함수 ====================

def draw_corner_box(img, x1, y1, x2, y2, color, thickness=2, length=14):
    """꽉 찬 사각형 대신 모서리만 그려서 좀 더 '타겟팅' 느낌 나게."""
    pts = [
        ((x1, y1), (x1 + length, y1), (x1, y1 + length)),
        ((x2, y1), (x2 - length, y1), (x2, y1 + length)),
        ((x1, y2), (x1 + length, y2), (x1, y2 - length)),
        ((x2, y2), (x2 - length, y2), (x2, y2 - length)),
    ]
    for corner, h_end, v_end in pts:
        cv2.line(img, corner, h_end, color, thickness, cv2.LINE_AA)
        cv2.line(img, corner, v_end, color, thickness, cv2.LINE_AA)


def draw_panel(img, x1, y1, x2, y2, color, alpha=0.55):
    """반투명 패널(HUD 배경) 그리기."""
    overlay = img.copy()
    cv2.rectangle(overlay, (x1, y1), (x2, y2), color, -1)
    cv2.addWeighted(overlay, alpha, img, 1 - alpha, 0, img)


def put_text_centered(img, text, cy, font, scale, color, thickness):
    (tw, th), _ = cv2.getTextSize(text, font, scale, thickness)
    w = img.shape[1]
    cv2.putText(img, text, ((w - tw) // 2, cy + th // 2), font, scale, color, thickness, cv2.LINE_AA)


# ==================== 게임 상태 ====================
state = 'WAITING'
pair_buffer = deque(maxlen=STABLE_WINDOW)
miss_count = 0        # 손이 2개가 아니었던 연속 프레임 수
best_match_count = 0  # 화면 표시용 (progress bar에 사용)
result_text = ''
result_color = TEXT_COLOR
result_timer = 0
score_p1 = 0
score_p2 = 0
just_scored = False  # WAITING->RESULT 전환 프레임에서만 점수 반영하기 위한 플래그

# ==================== 카메라 설정 ====================
cap = cv2.VideoCapture(0, cv2.CAP_V4L2)
cap.set(cv2.CAP_PROP_FRAME_WIDTH, 320)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 240)
cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)

WIN_NAME = 'Rock Paper Scissors'
cv2.namedWindow(WIN_NAME, cv2.WINDOW_NORMAL)
cv2.resizeWindow(WIN_NAME, 320 * DISPLAY_SCALE, (240 + 90) * DISPLAY_SCALE)

startTime = time.time()

while cap.isOpened():
    ret, frame = cap.read()
    if not ret:
        break

    # ---------- 전처리 ----------
    canvas, scale, left, top = letterbox(frame, IMG_SIZE)
    img = cv2.cvtColor(canvas, cv2.COLOR_BGR2RGB).astype(np.float32) / 255.0
    img = np.transpose(img, (2, 0, 1))
    inp = np.expand_dims(img, 0)

    # ---------- 추론 ----------
    output_host = trt_engine.infer(inp)
    detections = postprocess(output_host, NUM_CLASSES, CONF_THRES, NMS_THRES)

    hands = []
    for det in detections:
        x1, y1, x2, y2 = to_original_coords(det['box'], scale, left, top)
        cx = (x1 + x2) / 2
        hands.append({
            'box': (x1, y1, x2, y2),
            'cx': cx,
            'class_id': det['class_id'],
            'gesture': CLASS_NAMES[det['class_id']],
            'conf': det['conf'],
        })
    hands.sort(key=lambda h: h['cx'])

    # ---------- 상태 머신 (최근 프레임 다수결 방식) ----------
    just_scored = False
    if state == 'WAITING':
        if len(hands) == 2:
            miss_count = 0
            current_pair = (hands[0]['gesture'], hands[1]['gesture'])
            pair_buffer.append(current_pair)
        else:
            miss_count += 1
            if miss_count >= MISS_RESET_LIMIT:
                pair_buffer.clear()

        if pair_buffer:
            best_pair, best_match_count = Counter(pair_buffer).most_common(1)[0]
        else:
            best_match_count = 0

        if best_match_count >= STABLE_MATCH_COUNT:
            winner = judge(best_pair[0], best_pair[1])
            if winner == 'DRAW':
                result_text = f'DRAW  ({best_pair[0]} vs {best_pair[1]})'
                result_color = DRAW_COLOR
            else:
                result_text = f'{winner} WINS  ({best_pair[0]} vs {best_pair[1]})'
                result_color = WIN_COLOR
                if winner == 'P1':
                    score_p1 += 1
                else:
                    score_p2 += 1
                just_scored = True
            state = 'RESULT'
            result_timer = RESULT_HOLD_FRAMES

    elif state == 'RESULT':
        result_timer -= 1
        if result_timer <= 0:
            state = 'WAITING'
            pair_buffer.clear()
            miss_count = 0
            best_match_count = 0

    # ==================== 화면 구성 (아래부터 전부 그리기) ====================
    # 캔버스: 카메라 화면(위) + HUD 바(아래 90px) 를 합친 뒤 전체를 확대
    canvas_h = 240 + 90
    display = np.zeros((canvas_h, 320, 3), dtype=np.uint8)
    display[:] = (25, 25, 25)
    display[0:240, 0:320] = frame

    # ---- 손 bbox + 라벨 (코너 브라켓 스타일) ----
    for i, h in enumerate(hands):
        x1, y1, x2, y2 = h['box']
        color = COLOR_LIST[h['class_id']]
        draw_corner_box(display, x1, y1, x2, y2, color, thickness=2, length=12)
        label = f"P{i+1} {h['gesture'].upper()} {h['conf']*100:.0f}%"
        (tw, th), _ = cv2.getTextSize(label, cv2.FONT_HERSHEY_SIMPLEX, 0.4, 1)
        draw_panel(display, x1, max(y1 - th - 10, 0), x1 + tw + 8, max(y1 - 2, th + 8), color, alpha=0.75)
        cv2.putText(display, label, (x1 + 4, max(y1 - 6, th + 4)),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.4, (20, 20, 20), 1, cv2.LINE_AA)

    # ---- 상단 스코어 HUD (반투명 바) ----
    draw_panel(display, 0, 0, 320, 22, HUD_COLOR, alpha=0.6)
    cv2.putText(display, f'P1 {score_p1}', (6, 16), cv2.FONT_HERSHEY_SIMPLEX, 0.5, ACCENT_COLOR, 1, cv2.LINE_AA)
    p2_text = f'P2 {score_p2}'
    (tw, _), _ = cv2.getTextSize(p2_text, cv2.FONT_HERSHEY_SIMPLEX, 0.5, 1)
    cv2.putText(display, p2_text, (320 - tw - 6, 16), cv2.FONT_HERSHEY_SIMPLEX, 0.5, ACCENT_COLOR, 1, cv2.LINE_AA)

    hint_text = "'r' reset  'q' quit"
    (tw, _), _ = cv2.getTextSize(hint_text, cv2.FONT_HERSHEY_SIMPLEX, 0.35, 1)
    cv2.putText(display, hint_text, (320 - tw - 4, canvas_h - 6),
                cv2.FONT_HERSHEY_SIMPLEX, 0.35, (255, 255, 255), 1, cv2.LINE_AA)

    curTime = time.time()
    fps = 1 / (curTime - startTime)
    startTime = curTime
    fps_text = f'{fps:.0f} FPS'
    (tw, _), _ = cv2.getTextSize(fps_text, cv2.FONT_HERSHEY_SIMPLEX, 0.4, 1)
    cv2.putText(display, fps_text, ((320 - tw) // 2, 16), cv2.FONT_HERSHEY_SIMPLEX, 0.4, (150, 150, 150), 1, cv2.LINE_AA)

    # ---- 하단 HUD 패널 ----
    draw_panel(display, 0, 240, 320, canvas_h, HUD_COLOR, alpha=0.95)

    if state == 'WAITING':
        if len(hands) == 2:
            status = 'HOLD STEADY...'
            # 안정성 progress bar
            bar_x1, bar_y1, bar_x2, bar_y2 = 20, 275, 300, 285
            cv2.rectangle(display, (bar_x1, bar_y1), (bar_x2, bar_y2), (70, 70, 70), -1)
            fill_w = int((bar_x2 - bar_x1) * min(best_match_count / STABLE_MATCH_COUNT, 1.0))
            cv2.rectangle(display, (bar_x1, bar_y1), (bar_x1 + fill_w, bar_y2), ACCENT_COLOR, -1)
        else:
            status = 'show both hands'
        put_text_centered(display, status, 260, cv2.FONT_HERSHEY_SIMPLEX, 0.55, TEXT_COLOR, 1)

    elif state == 'RESULT':
        put_text_centered(display, result_text, 268, cv2.FONT_HERSHEY_SIMPLEX, 0.6, result_color, 2)
        if just_scored:
            put_text_centered(display, 'POINT!', 300, cv2.FONT_HERSHEY_SIMPLEX, 0.45, TEXT_COLOR, 1)

    # ---- 결과 시 화면 테두리 하이라이트 ----
    if state == 'RESULT':
        cv2.rectangle(display, (1, 1), (318, 238), result_color, 3)

    # ---- 확대해서 표시 ----
    display_big = cv2.resize(display, (320 * DISPLAY_SCALE, canvas_h * DISPLAY_SCALE),
                              interpolation=cv2.INTER_NEAREST)
    cv2.imshow(WIN_NAME, display_big)

    key = cv2.waitKey(10) & 0xFF
    if key == ord('q'):
        break
    elif key == ord('r'):
        score_p1 = 0
        score_p2 = 0

cap.release()
cv2.destroyAllWindows()