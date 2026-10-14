import cv2
import mediapipe as mp
import time
import math
from pathlib import Path
from collections import deque

CAMERA_INDEX = 0

MOVE_THRESHOLD = 25

WAVE_TIME_WINDOW = 1.2

# 最少方向变化次数
MIN_DIRECTION_CHANGES = 2

PINCH_THRESHOLD = 0.25
PINCH_RELEASE_THRESHOLD = 0.45

COOLDOWN = 2.0
PHOTO_DURATION = 3.0
MEDIA_DIR = Path(__file__).resolve().parent / "color" / "color"
MAIN_VIDEO_PATH = MEDIA_DIR / "video.mp4"
MEDIA_SEQUENCE = [
    "color1.mp4",
    "color2.mp4",
    "color3.mp4",
    "color4.mp4",
    "color5_00691.jpg",
    "color6.mp4",
    "color7_01145.jpg",
    "color8_01402.jpg",
    "color9_01630.jpg",
]


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



#点与点
position_history = deque()

last_x = None
last_direction = 0

direction_changes = 0

pinch_active = False
pinch_armed = True

wave_count = 0

last_wave_time = 0

main_video = cv2.VideoCapture(str(MAIN_VIDEO_PATH))

if not main_video.isOpened():
    print("无法打开主视频:", MAIN_VIDEO_PATH)
    cap.release()
    exit()

media_index = -1
media_capture = None
media_started_at = 0
media_finished = True
main_video_resume_frame = 0


def release_media():
    global media_capture, media_finished

    if media_capture is not None:
        media_capture.release()
        media_capture = None
    media_finished = True


def resume_main_video():
    if main_video.isOpened():
        main_video.set(cv2.CAP_PROP_POS_FRAMES, main_video_resume_frame)


def finish_media():
    global media_finished

    release_media()
    resume_main_video()
    cv2.destroyWindow("Color Media")


def start_media(index):
    global media_capture, media_started_at, media_finished
    global main_video_resume_frame

    main_video_resume_frame = max(
        int(main_video.get(cv2.CAP_PROP_POS_FRAMES)) - 1,
        0
    )
    release_media()
    media_path = MEDIA_DIR / MEDIA_SEQUENCE[index]

    if media_path.suffix.lower() == ".mp4":
        media_capture = cv2.VideoCapture(str(media_path))
        if not media_capture.isOpened():
            print("无法打开跳转视频:", media_path)
            release_media()
            resume_main_video()
            return False

    media_started_at = time.time()
    media_finished = False
    print("检测到捏合，暂停主视频并播放:", media_path)
    return True


def show_media():
    if media_index < 0 or media_finished:
        return

    media_path = MEDIA_DIR / MEDIA_SEQUENCE[media_index]

    if media_path.suffix.lower() == ".mp4":
        ret, media_frame = media_capture.read()
        if not ret:
            finish_media()
            return
        cv2.imshow("Color Media", media_frame)
    elif time.time() - media_started_at < PHOTO_DURATION:
        media_frame = cv2.imread(str(media_path))
        if media_frame is not None:
            cv2.imshow("Color Media", media_frame)
    else:
        finish_media()


def show_main_video():
    ret, main_frame = main_video.read()
    if not ret:
        main_video.set(cv2.CAP_PROP_POS_FRAMES, 0)
        ret, main_frame = main_video.read()

    if ret:
        cv2.imshow("Main Video", main_frame)
    return ret

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

        thumb_tip = hand_landmarks.landmark[4]
        index_tip = hand_landmarks.landmark[8]
        index_mcp = hand_landmarks.landmark[5]
        pinky_mcp = hand_landmarks.landmark[17]
        fingertip_distance = math.hypot(
            thumb_tip.x - index_tip.x,
            thumb_tip.y - index_tip.y
        )
        palm_scale = math.hypot(
            index_mcp.x - pinky_mcp.x,
            index_mcp.y - pinky_mcp.y
        )
        pinch_ratio = fingertip_distance / max(palm_scale, 1e-6)
        pinch_active = pinch_ratio < PINCH_THRESHOLD

        if pinch_ratio > PINCH_RELEASE_THRESHOLD:
            pinch_armed = True

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
        direction_text = "PINCH" if pinch_active else "OPEN"

        if not media_finished:
            pinch_armed = False
            direction_text = "LOCKED"

        if media_finished and pinch_active and pinch_armed:
            now = time.time()

            if now - last_wave_time > COOLDOWN:
                wave_count += 1
                last_wave_time = now
                pinch_armed = False

                if media_index < len(MEDIA_SEQUENCE) - 1:
                    media_index += 1
                    start_media(media_index)

                print("================================")
                print("       PINCH DETECTED!")
                print("捏合次数:", wave_count)
                print("================================")

    else:

        pinch_active = False
        pinch_armed = True
        position_history.clear()


    if media_finished:
        show_main_video()
    else:
        show_media()

    cv2.imshow(
        "Python Wave Detection",
        frame
    )
    # ESC
    key = cv2.waitKey(1) & 0xFF

    if key == 27:
        break

cap.release()
release_media()
main_video.release()
cv2.destroyAllWindows()
hands.close()