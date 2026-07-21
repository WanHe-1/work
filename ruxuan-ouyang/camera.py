import json
import time
from pathlib import Path

import cv2
import mediapipe as mp
import numpy as np
import serial

# =========================
# 配置
# =========================
DEVICES = [
    {"name": "Device 1", "camera_index": 1, "serial_port": "COM16", "calibration_file": "gaze_calibration_1.json"},
    {"name": "Device 0", "camera_index": 0, "serial_port": "COM3", "calibration_file": "gaze_calibration_0.json"},
    {"name": "Device 3", "camera_index": 3, "serial_port": "COM18", "calibration_file": "gaze_calibration_3.json"},
]

BAUD_RATE = 9600

SCREEN_W = 1920
SCREEN_H = 1080

# 正脸触发时间（核心）
FRONTAL_REQUIRED_SECONDS = 3.0
TRIGGER_COOLDOWN_SECONDS = 6.0

# LED效果
LED_FULL_ON_TIME = 3.0
LED_FADE_TIME = 3.0

SMOOTH_ALPHA = 0.4

FRONTAL_NOSE_OFFSET_LIMIT = 0.12
FRONTAL_EYE_RATIO_LIMIT = 0.55


# =========================
# MediaPipe
# =========================
mp_face_mesh = mp.solutions.face_mesh

LEFT_EYE_POINTS = [33, 133, 159, 145, 153, 154, 155, 173]
RIGHT_EYE_POINTS = [362, 263, 386, 374, 380, 381, 382, 398]
LEFT_IRIS_POINTS = [468, 469, 470, 471]
RIGHT_IRIS_POINTS = [473, 474, 475, 476]


# =========================
# 核心类
# =========================
class GazeDevice:
    def __init__(self, config):
        self.name = config["name"]
        self.camera_index = config["camera_index"]
        self.serial_port = config["serial_port"]

        self.capture = cv2.VideoCapture(self.camera_index)
        if not self.capture.isOpened():
            print(f"{self.name}: 摄像头打开失败")
            self.capture = None

        self.arduino = None
        try:
            self.arduino = serial.Serial(self.serial_port, BAUD_RATE, timeout=1)
            time.sleep(2)
            print(f"{self.name}: Arduino已连接")
        except Exception as e:
            print(f"{self.name}: 串口失败 {e}")

        self.face_mesh = mp_face_mesh.FaceMesh(
            static_image_mode=False,
            max_num_faces=1,
            refine_landmarks=True,
            min_detection_confidence=0.6,
            min_tracking_confidence=0.6
        )

        # 状态
        self.frontal_start_time = None
        self.last_trigger_time = 0

        self.smoothed_x = SCREEN_W / 2
        self.smoothed_y = SCREEN_H / 2

        self.status_text = "INIT"
        self.trigger_time = None
        self.last_led_value = -1

        self.calibration = {"offset_x": 0, "offset_y": 0, "scale_x": 1, "scale_y": 1}

    @property
    def available(self):
        return self.capture is not None

    # =========================
    # Arduino控制（0~255）
    # =========================
    def send_led(self, value):
        if self.arduino:
            try:
                self.arduino.write((str(int(value)) + "\n").encode())
            except:
                pass

    # =========================
    # 正脸判断
    # =========================
    def is_frontal_face(self, lm, w, h):
        nose_x = lm[1].x * w
        left = lm[234].x * w
        right = lm[454].x * w

        face_w = max(abs(right - left), 1)
        center = (left + right) / 2

        nose_ratio = abs(nose_x - center) / face_w

        le = abs(lm[33].x - lm[133].x)
        re = abs(lm[362].x - lm[263].x)
        eye_ratio = min(le, re) / max(le, re, 1e-6)

        return (nose_ratio < FRONTAL_NOSE_OFFSET_LIMIT and
                eye_ratio > FRONTAL_EYE_RATIO_LIMIT)

    # =========================
    # LED状态机（重点）
    # =========================
    def update_led(self):
        if self.trigger_time is None:
            return

        t = time.time() - self.trigger_time

        # 亮3秒
        if t <= LED_FULL_ON_TIME:
            val = 255

        # 渐暗3秒
        elif t <= LED_FULL_ON_TIME + LED_FADE_TIME:
            fade = (t - LED_FULL_ON_TIME) / LED_FADE_TIME
            val = int(255 * (1 - fade))

        else:
            val = 0
            self.trigger_time = None

        if val != self.last_led_value:
            self.send_led(val)
            self.last_led_value = val

    # =========================
    # 核心处理
    # =========================
    def process_once(self):
        if self.capture is None:
            return None

        ok, frame = self.capture.read()
        if not ok:
            return None

        frame = cv2.flip(frame, 1)
        rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)

        result = self.face_mesh.process(rgb)

        is_frontal = False

        if result.multi_face_landmarks:
            lm = result.multi_face_landmarks[0].landmark
            h, w = frame.shape[:2]

            is_frontal = self.is_frontal_face(lm, w, h)

        self.handle_frontal(is_frontal)
        self.update_led()

        self.draw(frame)

        return frame

    # =========================
    # 正脸计时逻辑（核心改动）
    # =========================
    def handle_frontal(self, is_frontal):
        now = time.time()

        if is_frontal:
            if self.frontal_start_time is None:
                self.frontal_start_time = now

            duration = now - self.frontal_start_time

            self.status_text = f"FRONTAL {duration:.1f}s"

            can_trigger = now - self.last_trigger_time > TRIGGER_COOLDOWN_SECONDS

            if duration >= FRONTAL_REQUIRED_SECONDS and can_trigger:
                self.last_trigger_time = now
                self.frontal_start_time = None

                # ⭐ 触发LED
                self.trigger_time = now
                self.send_led(255)

                print(f"{self.name}: 正脸≥3秒，触发LED")

        else:
            self.frontal_start_time = None
            self.status_text = "NO FACE"

    # =========================
    # UI
    # =========================
    def draw(self, frame):
        cv2.putText(frame, self.name, (20, 30),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0,255,255), 2)

        cv2.putText(frame, self.status_text, (20, 70),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0,255,255), 2)

    # =========================
    def close(self):
        if self.capture:
            self.capture.release()
        if self.arduino:
            self.arduino.close()
        self.face_mesh.close()


# =========================
# 主程序
# =========================
def main():
    devices = [GazeDevice(c) for c in DEVICES]
    devices = [d for d in devices if d.available]

    if not devices:
        raise RuntimeError("没有可用摄像头")

    print("ESC退出")

    while True:
        for d in devices:
            frame = d.process_once()
            if frame is not None:
                cv2.imshow(d.name, frame)

        key = cv2.waitKey(1)
        if key == 27:
            break

    for d in devices:
        d.close()

    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()