import pygame
import os
import time

# 测试音频播放
AUDIO_FOLDER = r'D:\Desktop\code\arduino\work\piano\video'

print("=== 音频播放测试程序 ===")
print("初始化 pygame...")

try:
    pygame.mixer.init()
    print("✅ pygame 初始化成功")
    
    # 测试播放第一个音频
    audio_file = os.path.join(AUDIO_FOLDER, '1.mp3')
    
    if os.path.exists(audio_file):
        print(f"📁 找到文件：{audio_file}")
        print("🎵 尝试播放...")
        
        try:
            sound = pygame.mixer.Sound(audio_file)
            print(f"✅ 音频加载成功，时长：{sound.get_length():.2f}秒")
            
            channel = sound.play()
            print("🔊 正在播放...")
            
            # 等待播放完成
            while channel.get_busy():
                time.sleep(0.1)
            
            print("✅ 播放完成！")
            
        except Exception as e:
            print(f"❌ 播放失败：{e}")
            print("\n可能的原因：")
            print("1. MP3 编码格式不兼容")
            print("2. 需要转换为 WAV 格式")
    else:
        print(f"❌ 文件不存在：{audio_file}")
        
except Exception as e:
    print(f"❌ 初始化失败：{e}")

finally:
    pygame.mixer.quit()
    print("\n程序结束")
