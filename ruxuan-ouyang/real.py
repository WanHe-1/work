import json
import time
from pathlib import Path

import cv2
import mediapipe as mp
import numpy as np
import serial

DEVICES = [
    {"name": "Device 1", "camera_index": 1, "serial_port": "COM16", "send_message": "ON", "calibration_file": "gaze_calibration_1.json"},
    {"name": "Device 0", "camera_index": 0, "serial_port": "COM17", "send_message": "ON", "calibration_file": "gaze_calibration_0.json"},
    {"name": "Device 3", "camera_index": 3, "serial_port": "COM18", "send_message": "ON", "calibration_file": "gaze_calibration_3.json"},
]

BAUD_RATE = 9600
SCREEN_W = 1920
SCREEN_H = 1080
SMOOTH_ALPHA = 0.40
EDGE_GAIN_X = 1.15
EDGE_GAIN_Y = 0.95
LOOK_AT_CAMERA_SECONDS = 3.0
TRIGGER_COOLDOWN_SECONDS = 6.0
FRONTAL_NOSE_OFFSET_LIMIT = 0.12
FRONTAL_EYE_RATIO_LIMIT = 0.55
GAZE_CENTER_TOLERANCE_X = 0.14
GAZE_CENTER_TOLERANCE_Y = 0.16

# ⭐ 新增：LED控制参数
LED_FULL_ON_TIME = 3.0
LED_FADE_TIME = 3.0

mp_face_mesh = mp.solutions.face_mesh
LEFT_EYE_POINTS = [33, 133, 159, 145, 153, 154, 155, 173]
RIGHT_EYE_POINTS = [362, 263, 386, 374, 380, 381, 382, 398]
LEFT_IRIS_POINTS = [468, 469, 470, 471]
RIGHT_IRIS_POINTS = [473, 474, 475, 476]


class GazeDevice:
    def __init__(self, config):
        self.name = config["name"]
        self.camera_index = config["camera_index"]
        self.serial_port = config["serial_port"]
        self.calibration_path = Path(config["calibration_file"])

        self.capture = cv2.VideoCapture(self.camera_index)
        if not self.capture.isOpened():
            print(f"{self.name}: 无法打开摄像头 {self.camera_index}")
            self.capture = None

        self.arduino = None
        try:
            self.arduino = serial.Serial(self.serial_port, BAUD_RATE, timeout=1)
            time.sleep(2)
            print(f"{self.name}: 已连接 Arduino {self.serial_port}")
        except Exception as e:
            print(f"{self.name}: Arduino 串口连接失败 {self.serial_port}: {e}")

        self.calibration = {"offset_x": 0.0, "offset_y": 0.0, "scale_x": 1.0, "scale_y": 1.0}
        if self.calibration_path.exists():
            try:
                self.calibration.update(json.loads(self.calibration_path.read_text(encoding="utf-8")))
            except Exception as e:
                print(f"{self.name}: 校正文件读取失败: {e}")

        self.face_mesh = mp_face_mesh.FaceMesh(
            static_image_mode=False,
            max_num_faces=1,
            refine_landmarks=True,
            min_detection_confidence=0.6,
            min_tracking_confidence=0.6
        )

        self.look_start_time = None
        self.last_trigger_time = 0.0

        self.smoothed_x = SCREEN_W / 2
        self.smoothed_y = SCREEN_H / 2
        self.last_gaze_x = SCREEN_W / 2
        self.last_gaze_y = SCREEN_H / 2

        self.status_text = "INIT"
        self.look_progress = 0.0

        # ⭐ LED状态
        self.led_trigger_time = None
        self.last_brightness_sent = -1

    @property
    def available(self):
        return self.capture is not None

    # ⭐ 修改：发送亮度值（0-255）
    def send_to_arduino(self, brightness: int):
        if self.arduino is None:
            return
        try:
            brightness = int(np.clip(brightness, 0, 255))
            self.arduino.write((str(brightness) + "\n").encode("utf-8"))
        except Exception as e:
            print(f"{self.name}: 发送串口数据失败: {e}")

    def save_calibration(self):
        self.calibration["offset_x"] += SCREEN_W / 2 - self.last_gaze_x
        self.calibration["offset_y"] += SCREEN_H / 2 - self.last_gaze_y
        self.calibration_path.write_text(json.dumps(self.calibration, ensure_ascii=False, indent=2), encoding="utf-8")
        print(f"{self.name}: 已保存校正参数")

    def reset_calibration(self):
        self.calibration = {"offset_x": 0.0, "offset_y": 0.0, "scale_x": 1.0, "scale_y": 1.0}
        if self.calibration_path.exists():
            self.calibration_path.unlink()
        print(f"{self.name}: 已重置校正参数")

    def close(self):
        if self.capture is not None:
            self.capture.release()
        if self.arduino is not None:
            self.arduino.close()
        self.face_mesh.close()

    def process_once(self):
        if self.capture is None:
            return None

        ret, frame = self.capture.read()
        if not ret:
            return None

        frame = cv2.flip(frame, 1)
        rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        result = self.face_mesh.process(rgb)

        gaze_x = self.smoothed_x
        gaze_y = self.smoothed_y
        looking_at_this_camera = False
        self.status_text = "NO FACE"

        if result.multi_face_landmarks:
            landmarks = result.multi_face_landmarks[0].landmark
            h, w = frame.shape[:2]

            if self.is_frontal_face(landmarks, w, h):
                left_eye = self.eye_bounds(landmarks, LEFT_EYE_POINTS, w, h)
                right_eye = self.eye_bounds(landmarks, RIGHT_EYE_POINTS, w, h)
                left_iris = self.iris_center(landmarks, LEFT_IRIS_POINTS, w, h)
                right_iris = self.iris_center(landmarks, RIGHT_IRIS_POINTS, w, h)

                raw_x, raw_y = self.map_to_screen(left_iris, right_iris, left_eye, right_eye)

                self.smoothed_x = SMOOTH_ALPHA * raw_x + (1.0 - SMOOTH_ALPHA) * self.smoothed_x
                self.smoothed_y = SMOOTH_ALPHA * raw_y + (1.0 - SMOOTH_ALPHA) * self.smoothed_y

                gaze_x = self.smoothed_x
                gaze_y = self.smoothed_y

                looking_at_this_camera = self.is_gaze_centered(gaze_x, gaze_y)
                self.status_text = "LOOKING" if looking_at_this_camera else "FACE OK"
            else:
                self.status_text = "SIDE FACE"

        self.last_gaze_x = gaze_x
        self.last_gaze_y = gaze_y

        self.handle_look_duration(looking_at_this_camera)
        self.update_led()  # ⭐ 新增

        self.draw_ui(frame, gaze_x, gaze_y)
        return frame

    def handle_look_duration(self, looking_at_this_camera):
        now = time.time()

        if looking_at_this_camera:
            if self.look_start_time is None:
                self.look_start_time = now

            self.look_progress = min(now - self.look_start_time, LOOK_AT_CAMERA_SECONDS)

            can_trigger = now - self.last_trigger_time > TRIGGER_COOLDOWN_SECONDS

            if self.look_progress >= LOOK_AT_CAMERA_SECONDS and can_trigger:
                self.last_trigger_time = now
                self.look_start_time = None
                self.look_progress = 0.0

                # ⭐ 触发LED动画
                self.led_trigger_time = now
                self.send_to_arduino(255)

                self.status_text = "TRIGGERED"
                print(f"{self.name}: 触发点亮 + 渐暗流程")

        else:
            self.look_start_time = None
            self.look_progress = 0.0

    # ⭐ 新增：LED渐变逻辑
    def update_led(self):
        if self.led_trigger_time is None:
            return

        now = time.time()
        t = now - self.led_trigger_time

        if t <= LED_FULL_ON_TIME:
            brightness = 255
        elif t <= LED_FULL_ON_TIME + LED_FADE_TIME:
            fade_t = (t - LED_FULL_ON_TIME) / LED_FADE_TIME
            brightness = int(255 * (1.0 - fade_t))
        else:
            brightness = 0
            self.led_trigger_time = None  # 完成周期

        if brightness != self.last_brightness_sent:
            self.send_to_arduino(brightness)
            self.last_brightness_sent = brightness

    def draw_ui(self, frame, gaze_x, gaze_y):
        h, w = frame.shape[:2]
        px = int(gaze_x * w / SCREEN_W)
        py = int(gaze_y * h / SCREEN_H)

        cv2.circle(frame, (px, py), 8, (0, 0, 255), -1)

        progress_ratio = self.look_progress / LOOK_AT_CAMERA_SECONDS
        bar_w = int(240 * progress_ratio)

        cv2.rectangle(frame, (20, 120), (260, 140), (80, 80, 80), 2)
        cv2.rectangle(frame, (20, 120), (20 + bar_w, 140), (0, 255, 0), -1)

        cv2.putText(frame, self.name, (20, 35), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 255), 2)
        cv2.putText(frame, self.status_text, (20, 70), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 255), 2)

        cv2.putText(frame,
                    f"Look:{self.look_progress:.1f}/{LOOK_AT_CAMERA_SECONDS:.1f}s",
                    (20, 105),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.75,
                    (0, 255, 255), 2)

    def is_frontal_face(self, landmarks, width, height):
        nose_x = landmarks[1].x * width
        left_face_x = landmarks[234].x * width
        right_face_x = landmarks[454].x * width

        face_width = max(abs(right_face_x - left_face_x), 1.0)
        face_center_x = (left_face_x + right_face_x) / 2.0

        nose_offset_ratio = abs(nose_x - face_center_x) / face_width

        left_eye_outer_x = landmarks[33].x * width
        left_eye_inner_x = landmarks[133].x * width
        right_eye_inner_x = landmarks[362].x * width
        right_eye_outer_x = landmarks[263].x * width

        left_eye_width = abs(left_eye_inner_x - left_eye_outer_x)
        right_eye_width = abs(right_eye_inner_x - right_eye_outer_x)

        eye_width_ratio = min(left_eye_width, right_eye_width) / max(left_eye_width, right_eye_width, 1.0)

        return nose_offset_ratio < FRONTAL_NOSE_OFFSET_LIMIT and eye_width_ratio > FRONTAL_EYE_RATIO_LIMIT

    def is_gaze_centered(self, gaze_x, gaze_y):
        center_x = abs(gaze_x - SCREEN_W / 2) <= SCREEN_W * GAZE_CENTER_TOLERANCE_X
        center_y = abs(gaze_y - SCREEN_H / 2) <= SCREEN_H * GAZE_CENTER_TOLERANCE_Y
        return center_x and center_y

    def eye_bounds(self, landmarks, indices, width, height):
        points = [(landmarks[i].x * width, landmarks[i].y * height) for i in indices]
        return (min(p[0] for p in points), min(p[1] for p in points),
                max(p[0] for p in points), max(p[1] for p in points))

    def iris_center(self, landmarks, indices, width, height):
        xs = [landmarks[i].x * width for i in indices]
        ys = [landmarks[i].y * height for i in indices]
        return float(sum(xs) / len(xs)), float(sum(ys) / len(ys))

    def map_to_screen(self, left_iris, right_iris, left_eye, right_eye):
        def norm_in_eye(iris, eye):
            iris_x, iris_y = iris
            ex_min, ey_min, ex_max, ey_max = eye
            eye_w = max(ex_max - ex_min, 1.0)
            eye_h = max(ey_max - ey_min, 1.0)
            return (iris_x - ex_min) / eye_w, (iris_y - ey_min) / eye_h

        lnx, lny = norm_in_eye(left_iris, left_eye)
        rnx, rny = norm_in_eye(right_iris, right_eye)

        nx = (lnx + rnx) / 2.0
        ny = (lny + rny) / 2.0

        x = (nx - 0.5) * SCREEN_W * EDGE_GAIN_X * self.calibration["scale_x"] + SCREEN_W / 2 + self.calibration["offset_x"]
        y = (ny - 0.5) * SCREEN_H * EDGE_GAIN_Y * self.calibration["scale_y"] + SCREEN_H / 2 + self.calibration["offset_y"]

        return float(np.clip(x, 0, SCREEN_W - 1)), float(np.clip(y, 0, SCREEN_H - 1))


def main():
    devices = [GazeDevice(config) for config in DEVICES]
    devices = [device for device in devices if device.available]

    if not devices:
        raise RuntimeError("没有任何摄像头可以打开，请检查 camera_index 配置")

    print("按 ESC 退出")
    print("按 1/2/3 校正")

    try:
        while True:
            for device in devices:
                frame = device.process_once()
                if frame is not None:
                    cv2.imshow(device.name, frame)

            key = cv2.waitKey(1)
            if key == 27:
                break

            if key in (ord("1"), ord("2"), ord("3")):
                idx = key - ord("1")
                if idx < len(devices):
                    devices[idx].save_calibration()

            if key in (ord("!"), ord("@"), ord("#")):
                idx = [ord("!"), ord("@"), ord("#")].index(key)
                if idx < len(devices):
                    devices[idx].reset_calibration()

    finally:
        for device in devices:
            device.close()
        cv2.destroyAllWindows()


if __name__ == "__main__":
    main()