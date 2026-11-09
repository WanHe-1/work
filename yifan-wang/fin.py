
import cv2
import mediapipe as mp
import numpy as np
import urllib.request
import time


# ==================================================
# ESP32-CAM 配置
# ==================================================

CAMERA_URL = "http://192.168.104.205:81/stream"


# ==================================================
# MediaPipe 初始化
# ==================================================

mp_face_mesh = mp.solutions.face_mesh

face_mesh = mp_face_mesh.FaceMesh(
    max_num_faces=1,
    refine_landmarks=True,
    min_detection_confidence=0.6,
    min_tracking_confidence=0.6
)


# ==================================================
# 眼睛关键点
# ==================================================

# 左眼虹膜
IRIS_LEFT = [468, 469, 470, 471, 472]

# 右眼虹膜
IRIS_RIGHT = [473, 474, 475, 476, 477]

# 左眼 EAR
EAR_LEFT = [33, 160, 158, 133, 153, 144]

# 右眼 EAR
EAR_RIGHT = [362, 385, 387, 263, 373, 380]


# ==================================================
# 眨眼检测参数
# ==================================================

EAR_THRESHOLD = 0.20

# 闭眼连续达到指定帧数才计数
BLINK_FRAMES = 2

blink_count = 0
blink_frames = 0
blink_locked = False


# ==================================================
# 五点标定
# ==================================================

calibration = {}

CALIBRATION_NAMES = {
    1: "CENTER",
    2: "LEFT",
    3: "RIGHT",
    4: "UP",
    5: "DOWN"
}

current_direction = "CENTER"


# ==================================================
# 计算多个关键点的中心
# ==================================================

def get_point(landmarks, indices, width, height):

    points = []

    for index in indices:

        x = int(landmarks[index].x * width)
        y = int(landmarks[index].y * height)

        points.append([x, y])

    return np.mean(points, axis=0).astype(int)


# ==================================================
# 计算 EAR 眼睛纵横比
# ==================================================

def calculate_ear(landmarks, indices):

    points = []

    for index in indices:

        x = landmarks[index].x
        y = landmarks[index].y

        points.append(np.array([x, y]))

    p1, p2, p3, p4, p5, p6 = points

    vertical1 = np.linalg.norm(p2 - p6)
    vertical2 = np.linalg.norm(p3 - p5)

    horizontal = np.linalg.norm(p1 - p4)

    if horizontal == 0:
        return 0

    ear = (
        vertical1 + vertical2
    ) / (2.0 * horizontal)

    return ear


# ==================================================
# 分别获取左右眼虹膜坐标
# ==================================================

def get_gaze_position(landmarks, width, height):

    left_iris = get_point(
        landmarks,
        IRIS_LEFT,
        width,
        height
    )

    right_iris = get_point(
        landmarks,
        IRIS_RIGHT,
        width,
        height
    )

    return left_iris, right_iris


# ==================================================
# 五点标定
# ==================================================

def calibrate(direction, gaze_position):

    calibration[direction] = gaze_position.copy()

    print(
        "标定完成:",
        direction,
        gaze_position.tolist()
    )


# ==================================================
# 眼球方向识别
# ==================================================

def get_direction(gaze_position):

    if len(calibration) < 5:
        return "CENTER"

    center = calibration["CENTER"]
    left = calibration["LEFT"]
    right = calibration["RIGHT"]
    up = calibration["UP"]
    down = calibration["DOWN"]

    x = gaze_position[0]
    y = gaze_position[1]

    center_x = center[0]
    center_y = center[1]

    left_range = center_x - left[0]
    right_range = right[0] - center_x

    up_range = center_y - up[1]
    down_range = down[1] - center_y

    horizontal_threshold = max(
        8,
        min(abs(left_range), abs(right_range)) * 0.35
    )

    vertical_threshold = max(
        8,
        min(abs(up_range), abs(down_range)) * 0.35
    )

    dx = x - center_x
    dy = y - center_y

    direction = "CENTER"

    if left_range > 0 and dx < -horizontal_threshold:
        direction = "LEFT"

    elif right_range > 0 and dx > horizontal_threshold:
        direction = "RIGHT"

    if up_range > 0 and dy < -vertical_threshold:
        direction = "UP"

    elif down_range > 0 and dy > vertical_threshold:
        direction = "DOWN"

    return direction


# ==================================================
# 连接 ESP32-CAM
# ==================================================

print("正在连接 ESP32-CAM...")

try:

    stream = urllib.request.urlopen(
        CAMERA_URL,
        timeout=10
    )

    print("ESP32-CAM 连接成功！")

except Exception as e:

    print("摄像头连接失败:", e)

    raise SystemExit


bytes_data = b""


# ==================================================
# 主循环
# ==================================================

print("\n操作说明:")
print("1：标定中心")
print("2：标定左边")
print("3：标定右边")
print("4：标定上方")
print("5：标定下方")
print("R：重新标定")
print("C：清零眨眼次数")
print("ESC：退出程序\n")


try:

    while True:

        chunk = stream.read(4096)

        if not chunk:
            continue

        bytes_data += chunk

        start = bytes_data.find(b'\xff\xd8')
        end = bytes_data.find(b'\xff\xd9')

        if start != -1 and end != -1 and end > start:

            jpg = bytes_data[start:end + 2]

            bytes_data = bytes_data[end + 2:]

            frame = cv2.imdecode(
                np.frombuffer(
                    jpg,
                    dtype=np.uint8
                ),
                cv2.IMREAD_COLOR
            )

            if frame is None:
                continue

            height, width = frame.shape[:2]

            rgb = cv2.cvtColor(
                frame,
                cv2.COLOR_BGR2RGB
            )

            results = face_mesh.process(rgb)

            # 用于保存当前帧标定坐标
            gaze_position = None

            # ==================================================
            # 人脸检测
            # ==================================================

            if results.multi_face_landmarks:

                landmarks = results.multi_face_landmarks[0].landmark

                # ==================================================
                # 眨眼检测
                # ==================================================

                ear_left = calculate_ear(
                    landmarks,
                    EAR_LEFT
                )

                ear_right = calculate_ear(
                    landmarks,
                    EAR_RIGHT
                )

                ear = (ear_left + ear_right) / 2.0

                if ear < EAR_THRESHOLD:

                    blink_frames += 1

                    if (
                        blink_frames >= BLINK_FRAMES
                        and not blink_locked
                    ):

                        blink_count += 1

                        blink_locked = True

                else:

                    blink_frames = 0

                    blink_locked = False

                # ==================================================
                # 获取左右眼虹膜坐标
                # ==================================================

                left_iris, right_iris = get_gaze_position(
                    landmarks,
                    width,
                    height
                )

                # 左眼绿色坐标点
                cv2.circle(
                    frame,
                    tuple(left_iris),
                    5,
                    (0, 255, 0),
                    -1
                )

                # 右眼绿色坐标点
                cv2.circle(
                    frame,
                    tuple(right_iris),
                    5,
                    (0, 255, 0),
                    -1
                )

                # ==================================================
                # 双眼平均坐标，仅用于方向识别
                # ==================================================

                gaze_position = (
                    (
                        left_iris.astype(float)
                        + right_iris.astype(float)
                    ) / 2
                ).astype(int)

                current_direction = get_direction(
                    gaze_position
                )

            # ==================================================
            # 显示眨眼次数
            # ==================================================

            cv2.putText(
                frame,
                f"Blinks: {blink_count}",
                (20, 45),
                cv2.FONT_HERSHEY_SIMPLEX,
                1,
                (0, 255, 255),
                2
            )

            # 显示窗口
            cv2.imshow(
                "Eye Tracking",
                frame
            )

            key = cv2.waitKey(1) & 0xFF

            # ==================================================
            # 键盘控制
            # ==================================================

            if key == 27:
                break

            elif key in [
                ord('1'),
                ord('2'),
                ord('3'),
                ord('4'),
                ord('5')
            ]:

                number = int(chr(key))

                if gaze_position is not None:

                    name = CALIBRATION_NAMES[number]

                    calibrate(
                        name,
                        gaze_position
                    )

                else:

                    print("没有检测到眼睛，无法标定")

            elif key in [ord('r'), ord('R')]:

                calibration.clear()

                current_direction = "CENTER"

                print("标定数据已清除，请重新标定")

            elif key in [ord('c'), ord('C')]:

                blink_count = 0
                blink_frames = 0
                blink_locked = False

                print("眨眼次数已清零")


except Exception as e:

    print("程序运行错误:", e)


finally:

    stream.close()

    face_mesh.close()

    cv2.destroyAllWindows()

    print("程序已退出")
