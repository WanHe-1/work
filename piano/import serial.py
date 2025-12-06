import serial
import time
import os
import pygame
import threading

SERIAL_PORT = 'COM16' 

AUDIO_FOLDER = r'D:\Desktop\code\arduino\work\piano\video'

def init_audio():
    pygame.mixer.init(frequency=22050, size=-16, channels=2, buffer=512)
    pygame.mixer.set_num_channels(7)

def play_audio(melody_num):
    audio_formats = ['.wav', '.mp3']
    audio_path = None
    
    for fmt in audio_formats:
        test_path = os.path.join(AUDIO_FOLDER, f'{melody_num}{fmt}')
        if os.path.exists(test_path):
            audio_path = test_path
            break
  
    if not audio_path:

        return

    try:
        sound = pygame.mixer.Sound(audio_path)
        sound.set_volume(1.0) 
        channel = pygame.mixer.find_channel()
        if channel:
            channel.play(sound)
        else:
            print(f"⚠️ 没通道")
    except Exception as e:
        print(f"❌ {e}")
        import traceback
        traceback.print_exc()

def main():
    init_audio()

    ser = None
    max_retries = 3
    for attempt in range(max_retries):
        try:
            print(f" 正在连接串口 {SERIAL_PORT}...（尝试 {attempt + 1}/{max_retries}）")
            ser = serial.Serial(
                port=SERIAL_PORT,
                baudrate=9600,
                timeout=1,
                write_timeout=1,
                inter_byte_timeout=None
            )
            time.sleep(3)   
            break
        except serial.SerialException as e:
            print(f" 失败：{e}")
            if attempt < max_retries - 1:
                time.sleep(3)
            else:
                return
    
    try:
        ser.reset_input_buffer()
        ser.reset_output_buffer()

        while True:
            try:
                serial_data = ser.readline().decode('utf-8', errors='ignore').strip()
                if serial_data: 
                    print(f" 收到串口数据：'{serial_data}'")
                    if serial_data.isdigit():
                        melody_num = int(serial_data)
                        if 1 <= melody_num <= 7:
                            play_audio(melody_num)
                        else:
                            print(f" 无效编号：{serial_data}")
                    else:
                        print(f" 无效信号：{serial_data}")
            except serial.SerialException as e:
                print(f" 串口读取错误：{e}")
                break
            except UnicodeDecodeError:
                pass
    
    except KeyboardInterrupt:
        print("\n 退出程序中...")
    except Exception as e:
        print(f" 发生错误：{e}")
        import traceback
        traceback.print_exc()
    finally:
        if ser and ser.is_open:
            ser.close()
        pygame.mixer.quit()

if __name__ == "__main__":
    main()