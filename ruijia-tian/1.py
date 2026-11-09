
import cv2
import serial
import time
import os

# =========================
# 1. 串口配置
# =========================
PORT = "/dev/cu.usbserial-210"
BAUD_RATE = 9600

ser = serial.Serial(PORT, BAUD_RATE, timeout=0.01)
time.sleep(2)

print("Arduino 串口连接成功")
print("等待触摸信号 100...")

# =========================
# 2. 视频配置
# =========================
BASE_DIR = os.path.dirname(os.path.abspath(__file__))

videos = [
    os.path.join(BASE_DIR, "1.mp4"),
    os.path.join(BASE_DIR, "2.mp4"),
    os.path.join(BASE_DIR, "3.mp4")
]

current_index = -1
video_cap = None

# =========================
# 3. 切换视频
# =========================
def switch_video():
    global current_index, video_cap

    current_index = (current_index + 1) % len(videos)
    video_path = videos[current_index]

    print("准备播放：", os.path.basename(video_path))

    if not os.path.exists(video_path):
        print("视频文件不存在：", video_path)
        return False

    new_cap = cv2.VideoCapture(video_path)

    if not new_cap.isOpened():
        print("视频打开失败：", video_path)
        new_cap.release()
        return False

    if video_cap is not None:
        video_cap.release()

    video_cap = new_cap
    print("开始播放：", os.path.basename(video_path))
    return True


# =========================
# 4. 主循环
# =========================
try:
    while True:

        # 视频正在播放：不接收新的触摸信号
        if video_cap is not None:

            ret, frame = video_cap.read()

            if ret:
                cv2.imshow("Video Player", frame)

            else:
                # 视频播放完成
                video_cap.release()
                video_cap = None

                print("视频播放结束")
                print("重新等待触摸信号 100...")

                # 清除播放期间积累的串口数据
                ser.reset_input_buffer()

        else:
            # 只有没有播放视频时，才接收触摸信号
            if ser.in_waiting > 0:

                data = ser.readline().decode(
                    "utf-8", errors="ignore"
                ).strip()

                if data:
                    print("串口收到：", repr(data))

                if data == "100":
                    switch_video()

        # ESC 退出
        if cv2.waitKey(1) & 0xFF == 27:
            break

except KeyboardInterrupt:
    print("程序停止")

finally:
    if video_cap is not None:
        video_cap.release()

    ser.close()
    cv2.destroyAllWindows()
