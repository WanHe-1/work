# pc_csv_reader.py
import os, csv, time
from datetime import datetime
import serial

PORT = "COM3"               # 改成你的端口；macOS 形如 "/dev/tty.usbserial-xxxx"
BAUD = 57600
OUT_FILE = "D:\Desktop"
ADD_HEADER = False          # 若 Arduino 已打印表头就设 False；否则设 True 自行加

os.makedirs(os.path.dirname(os.path.abspath(OUT_FILE)), exist_ok=True)
ser = serial.Serial(PORT, BAUD, timeout=1.0)

# UNO 打开串口会复位，等半秒
time.sleep(0.5)

# 可选：加表头
if ADD_HEADER and not os.path.exists(OUT_FILE):
    with open(OUT_FILE, "w", newline="") as f:
        csv.writer(f).writerow(["timestamp_ms", "signal_quality", "attention", "meditation"])

buf = []
try:
    while True:
        line = ser.readline().decode(errors="ignore").strip()
        if not line:
            continue
        # 跳过 Arduino 发的表头
        if line.startswith("timestamp_ms"):
            continue

        parts = line.split(",")
        if len(parts) != 4:
            continue
        ts_str, sq_str, att_str, med_str = parts
        try:
            ts  = int(float(ts_str))
            sq  = int(sq_str)
            att = int(att_str)
            med = int(med_str)
        except ValueError:
            continue

        buf.append([ts, sq, att, med])

        if len(buf) >= 100:
            with open(OUT_FILE, "a", newline="") as f:
                csv.writer(f).writerows(buf)
            print(f"[INFO] wrote {len(buf)} rows; last={buf[-1]}")
            buf.clear()
except KeyboardInterrupt:
    pass
finally:
    if buf:
        with open(OUT_FILE, "a", newline="") as f:
            csv.writer(f).writerows(buf)
    ser.close()
    print(f"[INFO] Saved to {OUT_FILE}")