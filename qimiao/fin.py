
import cv2
import numpy as np
import math
import serial
import time

# ==================== 参数配置 ====================

CAMERA_INDEX = 0
SERIAL_PORT = 'COM5'
BAUD_RATE = 9600

MIN_RADIUS = 100
MAX_RADIUS = 160

# 连续多少帧没有检测到餐盘，才认为餐盘已经离开
LOST_FRAMES = 10

# ==================== Arduino 串口初始化 ====================

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
    print("Arduino 串口连接成功:", SERIAL_PORT)

except serial.SerialException as e:
    print("Arduino 串口连接失败:", e)
    print("请检查 COM5 是否正确，以及串口是否被其他程序占用")

# ==================== 摄像头初始化 ====================

cap = cv2.VideoCapture(CAMERA_INDEX)

if not cap.isOpened():
    print("摄像头打开失败！")
    if arduino is not None:
        arduino.close()
    raise SystemExit

print("摄像头启动成功")
print("黄色餐盘发送 1")
print("绿色餐盘发送 2")
print("蓝色餐盘发送 3")
print("按 Q 退出程序")

# ==================== 颜色配置 ====================

colors = {
    'yellow': {
        'lower': np.array([15, 65, 65]),
        'upper': np.array([35, 255, 255]),
        'signal': b'1',
        'draw_color': (0, 255, 255),
        'name': 'yellow'
    },

    'green': {
        'lower': np.array([30, 65, 65]),
        'upper': np.array([85, 255, 255]),
        'signal': b'2',
        'draw_color': (0, 255, 0),
        'name': 'green'
    },

    'blue': {
        'lower': np.array([90, 65, 65]),
        'upper': np.array([130, 255, 255]),
        'signal': b'3',
        'draw_color': (255, 0, 0),
        'name': 'blue'
    }
}

# ==================== 防重复发送状态 ====================

# 每种颜色独立记录检测状态
detected_state = {
    'yellow': False,
    'green': False,
    'blue': False
}

lost_count = {
    'yellow': 0,
    'green': 0,
    'blue': 0
}

# ==================== 主程序 ====================

try:

    while True:

        ret, img = cap.read()

        if not ret:
            print("摄像头读取失败")
            break

        # BGR 转 HSV
        hsv_img = cv2.cvtColor(img, cv2.COLOR_BGR2HSV)

        # 当前帧各颜色是否检测到
        current_detected = {
            'yellow': False,
            'green': False,
            'blue': False
        }

        # 当前帧数量统计
        counts = {
            'yellow_circle': 0,
            'yellow_rect': 0,
            'green_circle': 0,
            'green_rect': 0,
            'blue_circle': 0,
            'blue_rect': 0
        }

        # ==================== 三种颜色餐盘检测 ====================

        for color_name, config in colors.items():

            lower = config['lower']
            upper = config['upper']
            draw_color = config['draw_color']

            # 提取颜色
            mask = cv2.inRange(hsv_img, lower, upper)

            # 中值滤波
            mask = cv2.medianBlur(mask, 5)

            # 查找轮廓
            contours, hierarchy = cv2.findContours(
                mask,
                cv2.RETR_EXTERNAL,
                cv2.CHAIN_APPROX_NONE
            )

            for cnt in contours:

                area = cv2.contourArea(cnt)

                center, radius = cv2.minEnclosingCircle(cnt)

                if radius <= 0:
                    continue

                # 限制餐盘半径
                if not (MIN_RADIUS < radius < MAX_RADIUS):
                    continue

                rate = area / (math.pi * radius * radius)

                cx = int(center[0])
                cy = int(center[1])
                r = int(radius)

                # ================= 圆形餐盘 =================

                if rate >= 0.9:

                    cv2.circle(
                        img,
                        (cx, cy),
                        r,
                        draw_color,
                        2
                    )

                    cv2.putText(
                        img,
                        color_name + '_circle',
                        (cx, cy),
                        cv2.FONT_HERSHEY_SIMPLEX,
                        0.8,
                        draw_color,
                        2
                    )

                    counts[color_name + '_circle'] += 1

                    current_detected[color_name] = True

                # ================= 矩形餐盘 =================

                elif 0.6 < rate < 0.9:

                    rect = cv2.minAreaRect(cnt)

                    box = cv2.boxPoints(rect)

                    box = box.astype(np.int32)

                    cv2.drawContours(
                        img,
                        [box],
                        0,
                        draw_color,
                        2
                    )

                    cv2.putText(
                        img,
                        color_name + '_rect',
                        (cx, cy),
                        cv2.FONT_HERSHEY_SIMPLEX,
                        0.8,
                        draw_color,
                        2
                    )

                    counts[color_name + '_rect'] += 1

                    current_detected[color_name] = True

        # ==================== Arduino 信号发送 ====================

        for color_name, config in colors.items():

            # 检测到餐盘
            if current_detected[color_name]:

                lost_count[color_name] = 0

                # 只有第一次检测到才发送
                if not detected_state[color_name]:

                    detected_state[color_name] = True

                    if arduino is not None and arduino.is_open:

                        try:
                            arduino.write(config['signal'])

                            print(
                                "检测到",
                                color_name,
                                "餐盘，发送信号:",
                                config['signal'].decode()
                            )

                        except serial.SerialException as e:
                            print("Arduino 发送失败:", e)

            # 没有检测到餐盘
            else:

                if detected_state[color_name]:

                    lost_count[color_name] += 1

                    # 连续 LOST_FRAMES 帧没有检测到
                    # 才允许下一次重新发送
                    if lost_count[color_name] >= LOST_FRAMES:

                        detected_state[color_name] = False

                        lost_count[color_name] = 0

                        print(color_name, "餐盘已经离开，可以重新识别")

        # ==================== 显示统计信息 ====================

        yellow_num = (
            counts['yellow_circle'] +
            counts['yellow_rect']
        )

        green_num = (
            counts['green_circle'] +
            counts['green_rect']
        )

        blue_num = (
            counts['blue_circle'] +
            counts['blue_rect']
        )

        cv2.putText(
            img,
            'Yellow: ' + str(yellow_num),
            (20, 35),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.8,
            (0, 255, 255),
            2
        )

        cv2.putText(
            img,
            'Green: ' + str(green_num),
            (20, 70),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.8,
            (0, 255, 0),
            2
        )

        cv2.putText(
            img,
            'Blue: ' + str(blue_num),
            (20, 105),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.8,
            (255, 0, 0),
            2
        )

        # ==================== 显示画面 ====================

        cv2.imshow('Plate Detection', img)

        # 按 Q 退出
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

finally:

    cap.release()

    cv2.destroyAllWindows()

    if arduino is not None and arduino.is_open:
        arduino.close()

    print("程序已退出")
