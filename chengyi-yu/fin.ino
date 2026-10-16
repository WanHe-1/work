import time
import serial
import os
from pathlib import Path

SERIAL_PORT = "COM5"
BAUD_RATE = 9600
THRESHOLD = 400
BASE_DIR = Path(__file__).resolve().parent
VIDEO_DARK = BASE_DIR / "1.mp4"
VIDEO_LIGHT = BASE_DIR / "2.mp4"

for video in (VIDEO_DARK, VIDEO_LIGHT):
    if not video.is_file():
        raise FileNotFoundError(f"not：{video}")

current_state = None


def play_video(video_path):

    os.startfile(str(video_path))
    print(f"open：{video_path.name}")


try:
    with serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1) as ser:
        time.sleep(2)
        ser.reset_input_bufopenfer()

        while True:
            message = ser.readline().decode(
                "utf-8", errors="ignore"
            ).strip()

            if not message:
                continue

            try:
                intensity = int(message)
            except ValueError:
                continue

            print(f"now：{intensity}")

            new_state = "LIGHT" if intensity > THRESHOLD else "DARK"

            if new_state != current_state:
                current_state = new_state

                if current_state == "LIGHT":
                    play_video(VIDEO_LIGHT)
                else:
                    play_video(VIDEO_DARK)
