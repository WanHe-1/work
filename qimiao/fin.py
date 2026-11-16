
import cv2
import numpy as np
import math
import serial
import time

# ==================== 基本参数 ====================

CAMERA_INDEX = 0
SERIAL_PORT = 'COM3'
BAUD_RATE = 9600

MIN_RADIUS = 40
MAX_RADIUS = 600
MIN_AREA = 1000

CIRCLE_THRESHOLD = 0.75
LOST_FRAMES = 10

# ==================== Arduino 串口连接 ====================

arduino = None

try:
    arduino = serial.Serial(
        SERIAL_PORT,
        BAUD_RATE,
        timeout=1,
        write_timeout=1
    )
    time.sleep(2)
    arduino.reset_input_buffer()

except serial.SerialException:
    arduino = None

# ==================== 摄像头初始化 ====================

cap = cv2.VideoCapture(CAMERA_INDEX)

if not cap.isOpened():
    if arduino is not None:
        arduino.close()
    raise SystemExit

# ==================== HSV 颜色配置 ====================

colors = {
    'green': {
        'lower': np.array([35, 20, 80]),
        'upper': np.array([80, 150, 255]),
        'draw_color': (0, 255, 0)
    },

    'yellow': {
        'lower': np.array([15, 25, 120]),
        'upper': np.array([35, 180, 255]),
        'draw_color': (0, 255, 255)
    },

    'blue': {
        'lower': np.array([85, 15, 100]),
        'upper': np.array([125, 150, 255]),
        'draw_color': (255, 0, 0)
    }
}

# ==================== 防重复发送状态 ====================

detected_state = {
    'green': False,
    'yellow': False,
    'blue': False
}

lost_count = {
    'green': 0,
    'yellow': 0,
    'blue': 0
}

# ==================== 主程序 ====================

try:
    while True:

        ret, img = cap.read()

        if not ret:
            break

        hsv_img = cv2.cvtColor(
            img,
            cv2.COLOR_BGR2HSV
        )

        current_detected = {
            'green': False,
            'yellow': False,
            'blue': False
        }

        # ==================== 颜色识别 ====================

        for color_name, config in colors.items():

            mask = cv2.inRange(
                hsv_img,
                config['lower'],
                config['upper']
            )

            kernel = np.ones((5, 5), np.uint8)

            mask = cv2.morphologyEx(
                mask,
                cv2.MORPH_OPEN,
                kernel
            )

            mask = cv2.morphologyEx(
                mask,
                cv2.MORPH_CLOSE,
                kernel
            )

            contours, _ = cv2.findContours(
                mask,
                cv2.RETR_EXTERNAL,
                cv2.CHAIN_APPROX_SIMPLE
            )

            for cnt in contours:

                area = cv2.contourArea(cnt)

                if area < MIN_AREA:
                    continue

                center, radius = cv2.minEnclosingCircle(cnt)

                if radius <= 0:
                    continue

                if not (MIN_RADIUS < radius < MAX_RADIUS):
                    continue

                circle_area = math.pi * radius * radius
                rate = area / circle_area

                # 只识别圆形餐盘
                if rate < CIRCLE_THRESHOLD:
                    continue

                cx = int(center[0])
                cy = int(center[1])
                r = int(radius)

                # 只绘制餐盘轮廓，不显示文字
                cv2.circle(
                    img,
                    (cx, cy),
                    r,
                    config['draw_color'],
                    2
                )

                current_detected[color_name] = True

        # ==================== 串口发送 ====================

        for color_name in colors:

            if current_detected[color_name]:

                lost_count[color_name] = 0

                # 首次识别到该颜色时发送一次
                if not detected_state[color_name]:

                    detected_state[color_name] = True

                    if arduino is not None and arduino.is_open:
                        try:
                            arduino.write(b'1')
                            arduino.flush()

                        except serial.SerialException:
                            pass

            else:

                if detected_state[color_name]:

                    lost_count[color_name] += 1

                    # 连续消失10帧后解除锁定
                    if lost_count[color_name] >= LOST_FRAMES:

                        detected_state[color_name] = False
                        lost_count[color_name] = 0

        # ==================== 显示画面 ====================

        cv2.imshow('Plate Detection', img)

        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

finally:

    cap.release()
    cv2.destroyAllWindows()

    if arduino is not None and arduino.is_open:
        arduino.close()
