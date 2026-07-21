import socket
import cv2
import threading
import time
import numpy as np

videos = {
    1: "001.mp4",
    2: "002.mp4",
    3: "003.mp4"
}

current_zone = 0     
active_zone = 0         
play_start_time = 0      
cooldown_end_time = 0   

def udp_listener():
    global current_zone
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(("0.0.0.0", 8888))
    zones = {1: 0, 2: 0}  

    while True:
        data, addr = sock.recvfrom(1024)
        try:
            parts = data.decode().strip().split(',')
            dev_id = int(parts[0])
            zone = int(parts[1])
            zones[dev_id] = zone
            new_zone = max(zones.values())
            if new_zone != current_zone:
                current_zone = new_zone
                print(f"[UDP] Received zone: {current_zone}")
        except Exception as e:
            print("err:", e)

def create_black_frame(width=640, height=480):
    return np.zeros((height, width, 3), dtype=np.uint8)

def play_video():
    global , actcurrent_zoneive_zone, play_start_time, cooldown_end_time

    cap = None
    black_frame = create_black_frame()

    while True:
        now = time.time()

        in_cooldown = (now < cooldown_end_time)
        is_playing = (active_zone != 0) and (not in_cooldown)
        play_elapsed = now - play_start_time if is_playing else 0

        if is_playing and play_elapsed >= 60:
            print(f"[PLAY] Zone {active_zone}  3s.")
            if cap:
                cap.release()
                cap = None
            active_zone = 0
            cooldown_end_time = now + 3  
            in_cooldown = True

        if not in_cooldown and active_zone == 0 and current_zone != 0:
            active_zone = current_zone
            play_start_time = now
            print(f"[PLAY] Sta{active_zone}")
            if cap:
                cap.release()
            cap = cv2.VideoCapture(videos[active_zone])

        if is_playing and play_elapsed < 60:
            ret, frame = cap.read()
            if not ret:
                print(f"[PLAY] cooldown.")
                cap.release()
                cap = None
                active_zone = 0
                cooldown_end_time = now + 3
                frame = black_frame
            else:
                cv2.imshow('Zone Video', frame)
        else:
            cv2.imshow('Zone Video', black_frame)

        if cv2.waitKey(30) & 0xFF == ord('q'):
            break

        time.sleep(0.1)

    if cap:
        cap.release()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    listener_thread = thr