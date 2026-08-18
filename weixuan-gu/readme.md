保留 GSR 肌电/皮电检测
保留 TENS 模块的 Wire 控制
新增一个压力传感器输入 A1
当压力串口值 >= 1000 时，自动让 TENS 运行 5 秒，然后关闭
关键实现点：

GSR_PIN = A0
PRESSURE_PIN = A1
PRESSURE_THRESHOLD = 1000
TENS_RUN_TIME_MS = 5000
用 pressureArmed 做了触发防抖，避免压力一直大于 1000 时反复触发