import serial
import time
import os
import pygame
import threading
SERIAL_PORT = 'COM16' 
AUDIO_FOLDER = r'D:\Desktop\code\arduino\work\piano\video'
def init_audio():
    pygame.mixer.init(frequency=22050, size=-16, channels=2, buffer=512)
    pygame.mixer.set_num_channels(8) 

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
            print(f" 没通道")
    except Exception as e:
        print(f" {e}")
        import traceback
        traceback.print_exc()

def main():
    init_audio()

    ser = None
    max_retries = 3
    for attempt in range(max_retries):
        try:
            if ser and hasattr(ser, 'is_open') and ser.is_open:
                ser.close()
                time.sleep(1)
                
            print(f" 正在连接串口 {SERIAL_PORT}...（尝试 {attempt + 1}/{max_retries}）")
            ser = serial.Serial()
            ser.port = SERIAL_PORT
            ser.baudrate = 9600
            ser.timeout = 1
            ser.write_timeout = 1
            ser.inter_byte_timeout = None
            
            ser.open()
            
            if ser.is_open:
                print(f" 成功连接到 {SERIAL_PORT}")
                time.sleep(2)
                break
                
        except (serial.SerialException, OSError) as e:
            print(f" 连接失败：{e}")
            if attempt < max_retries - 1:
                print(" 3s...")
                time.sleep(3)
            else:
                return
    
    if not ser or not ser.is_open:
        print(" err")
        return
        
    try:
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        while ser.is_open:
            try:
                serial_data = ser.readline().decode('utf-8', errors='ignore').strip()
                if serial_data: 
                    print(f" 收到：'{serial_data}'")
                    if 'Touch Value:' in serial_data:
                        continue
                    if serial_data.isdigit():
                        melody_num = int(serial_data)
                        if 1 <= melody_num <= 7:  
                            play_audio(melody_num)
                        else:
                            print(f" 无效：{serial_data}")
                    else:
                        print(f" 无效：{serial_data}")
            except serial.SerialException as e:
                print(f" erro：{e}")
                break
            except UnicodeDecodeError:
                pass
    except KeyboardInterrupt:
        print("\n 退出...")
    except Exception as e:
        print(f" erro：{e}")
        import traceback
        traceback.print_exc()
    finally:
        try:
            if ser and hasattr(ser, 'is_open') and ser.is_open:
                ser.close()
                print(" 关闭")
        except Exception as e:
            print(f" 关闭：{e}")
        
        pygame.mixer.quit()  

if __name__ == "__main__":
    main()