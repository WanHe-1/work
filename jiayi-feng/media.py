import cv2
import mediapipe as mp
import math
import time
from collections import deque


# =========================
# 参数
# =========================

CAMERA_INDEX = 0

# 判断一次移动至少需要这么多像素
MOVE_THRESHOLD = 35

# 挥手识别时间窗口
WAVE_TIME_WINDOW = 1.2

# 最少方向变化次数
MIN_DIRECTION_CHANGES = 2

# 挥手触发后的冷却时间
COOLDOWN = 2.0


# =========================
# MediaPipe
# =========================

mp_hands = mp.solutions.hands
mp_draw = mp.solutions.drawing_utils

hands = mp_hands.Hands(
    static_image_mode=False,
    max_num_hands=1,
    min_detection_confidence=0.6,
    min_tracking_confidence=0.6
)


# =========================
# 摄像头
# =========================

cap = cv2.VideoCapture(CAMERA_INDEX)

if not cap.isOpened():
    print("无法打开摄像头")
    exit()


# =========================
# 挥手识别变量
# =========================

position_history = deque()

last_x = None
last_direction = 0

direction_changes = 0

wave_count = 0

last_wave_time = 0


# =========================
# 主循环
# =========================

while True:

    ret, frame = cap.read()

    if not ret:
        print("无法读取摄像头画面")
        break

    # 镜像
    frame = cv2.flip(frame, 1)

    h, w, _ = frame.shape

    # BGR -> RGB
    rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)

    results = hands.process(rgb)

    direction_text = "NONE"

    if results.multi_hand_landmarks:

        hand_landmarks = results.multi_hand_landmarks[0]

        # =========================
        # 计算手掌中心
        # =========================

        # 使用手腕 + MCP区域计算中心
        points = [
            hand_landmarks.landmark[0],   # wrist
            hand_landmarks.landmark[5],   # index MCP
            hand_landmarks.landmark[9],   # middle MCP
            hand_landmarks.landmark[13],  # ring MCP
            hand_landmarks.landmark[17]   # pinky MCP
        ]

        center_x = sum(p.x for p in points) / len(points)
        center_y = sum(p.y for p in points) / len(points)

        x = int(center_x * w)
        y = int(center_y * h)

        # =========================
        # 绘制手部
        # =========================

        mp_draw.draw_landmarks(
            frame,
            hand_landmarks,
            mp_hands.HAND_CONNECTIONS
        )

        cv2.circle(
            frame,
            (x, y),
            10,
            (255, 0, 255),
            -1
        )

        # =========================
        # 保存位置
        # =========================

        now = time.time()

        position_history.append((now, x))

        # 删除太旧的数据
        while position_history:
            if now - position_history[0][0] > WAVE_TIME_WINDOW:
                position_history.popleft()
            else:
                break

        # =========================
        # 判断移动方向
        # =========================

        if last_x is not None:

            dx = x - last_x

            if abs(dx) > MOVE_THRESHOLD:

                if dx > 0:
                    direction = 1
                    direction_text = "RIGHT"

                else:
                    direction = -1
                    direction_text = "LEFT"

                # 第一次有方向
                if last_direction == 0:

                    last_direction = direction

                # 方向发生反转
                elif direction != last_direction:

                    direction_changes += 1

                    last_direction = direction

                    print(
                        "方向变化:",
                        direction_changes
                    )

        last_x = x

        # =========================
        # 判断是否挥手
        # =========================

        if direction_changes >= MIN_DIRECTION_CHANGES:

            if now - last_wave_time > COOLDOWN:

                wave_count += 1

                last_wave_time = now

                print("================================")
                print("       WAVE DETECTED!")
                print("挥手次数:", wave_count)
                print("================================")

            # 重置
            direction_changes = 0
            last_direction = 0

        # =========================
        # 显示信息
        # =========================

        cv2.putText(
            frame,
            f"Direction: {direction_text}",
            (20, 40),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.8,
            (0, 255, 0),
            2
        )

        cv2.putText(
            frame,
            f"Direction changes: {direction_changes}",
            (20, 75),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.7,
            (255, 255, 0),
            2
        )

        cv2.putText(
            frame,
            f"Wave count: {wave_count}",
            (20, 110),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.7,
            (0, 255, 255),
            2
        )

    else:

        # 没有检测到手
        last_x = None
        last_direction = 0

        cv2.putText(
            frame,
            "No hand",
            (20, 40),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.8,
            (0, 0, 255),
            2
        )

    # =========================
    # 挥手提示
    # =========================

    if time.time() - last_wave_time < 0.8:

        cv2.putText(
            frame,
            "WAVE DETECTED!",
            (w // 2 - 180, 80),
            cv2.FONT_HERSHEY_SIMPLEX,
            1.2,
            (0, 255, 0),
            3
        )

    # =========================
    # 显示
    # =========================

    cv2.imshow(
        "Python Wave Detection",
        frame
    )

    # ESC退出
    key = cv2.waitKey(1) & 0xFF

    if key == 27:
        break


# =========================
# 释放
# =========================

cap.release()
cv2.destroyAllWindows()
hands.close()