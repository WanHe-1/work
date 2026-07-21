import cv2
import serial
import time
import sys
import os

SERIAL_PORT = 'COM3'
BAUD_RATE = 9600
IMAGE_PATH = '001.jng'
VIDEO_PATH = '001.mp4'
TEMP_THRESHOLD = 29.0

if not os.path.exists(IMAGE_PATH) or not os.path.exists(VIDEO_PATH):
    sys.exit("err")
ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
img = cv2.imread(IMAGE_PATH)
if img is None:
    sys.exit("err")
cv2.imshow("Display", img)
cv2.waitKey(1)
playing_video = False
cap = None
try:
    while True:
        if ser.in_waiting:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line != "ERROR":
                try:
                    temp = float(line)
                    if temp >= TEMP_THRESHOLD and not playing_video:
                        playing_video = True
                        cap = cv2.VideoCapture(VIDEO_PATH)
                except ValueError:
                    pass
        if playing_video:
            ret, frame = cap.read()
            if ret:
                cv2.imshow("Display", frame)
            else:
                playing_video = False
                cap.release()
                cv2.imshow("Display", img)
        else:
            cv2.imshow("Display", img)
        if cv2.waitKey(30) & 0xFF == 27:
            break
except KeyboardInterrupt:
    pass
finally:
    ser.close()
    if cap:
        cap.release()
    cv2.destroyAllWindows()