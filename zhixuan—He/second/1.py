
import serial
from pynput import keyboard

PORT = "COM9"
BAUD_RATE = 9600

# 连接 Arduino
arduino = serial.Serial(PORT, BAUD_RATE, timeout=1)

print(f"已连接 Arduino：{PORT}")
print("在任意页面按下回车键，即发送 1")
print("按 ESC 退出程序")

def on_press(key):
    try:
        if key == keyboard.Key.enter:
            arduino.write(b'1')
            arduino.flush()
            print("已发送：1")
        elif key == keyboard.Key.esc:
            arduino.close()
            
            return False
    except Exception as e:
        print("发送失败：", e)

with keyboard.Listener(on_press=on_press) as listener:
    listener.join()
