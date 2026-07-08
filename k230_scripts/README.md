# 人脸检测 (Face Detection)

## 功能介绍

基于 RetinaFace 轻量化人脸检测模型，在嘉楠 K230 / 庐山派开发板上实现**实时单类别人脸检测**。当摄像头画面中出现人脸时，系统自动绘制黄色矩形检测框，同时通过板载无源蜂鸣器发出间歇式提示音，GPIO42 引脚同步输出高电平信号，可用于驱动外部 LED 指示灯或继电器模块。

### 技术路线

```
GC2093摄像头(1920×1080@30fps)
    ↓
VPSS 缩放至 1280×720 (rgb888p_size)
    ↓
Ai2d 硬件预处理: letterbox pad + resize → 320×320 NCHW
    ↓
KPU 加载 face_detection_320.kmodel 推理 (RetinaFace架构)
    ↓
aidemo.face_det_post_process: 4200锚点 NMS后处理
    ↓
OSD 叠加: 黄色检测框 + 蜂鸣器/GPIO告警
    ↓
HDMI/LCD 显示
```

### 推理管线详解

本脚本基于 K230 官方 `AIBase` + `PipeLine` SDK 框架（嘉楠科技 CanMV 官方示例架构），主要流程：

1. **PipeLine 初始化**：配置 `display_mode`（`"hdmi"` / `"lcd"` / `"lt9611"` / `"st7701"` 等），创建 sensor → VPSS → OSD → 显示输出 的图像管线
2. **每帧循环**：
   - `pl.get_frame()` 从 VPSS 获取当前帧（RGB888 格式）
   - `face_det.run(img)` 将图像送入 Ai2d 硬件模块做 letterbox 填充 + bilinear resize → 输入 KPU 推理 → NMS 后处理
   - `face_det.draw_result(pl, dets)` 将检测框绘制到 OSD 图层并触发蜂鸣/GPIO
   - `pl.show_image()` 将 OSD 叠加到视频帧上送显

### 蜂鸣器告警机制

采用**非阻塞状态机**设计，避免了单线程 MicroPython 环境下 `sleep_ms()` 阻塞主循环导致帧率下降和 DDR/VPSS 时序异常的问题：

| 参数 | 值 | 说明 |
|------|-----|------|
| 蜂鸣频率 | 2700 Hz | 无源蜂鸣器谐振频率，人耳敏感区间 |
| PWM 占空比 | 60% | duty=60，响度适中 |
| 鸣叫时长 | 120 ms | 短促，不刺耳 |
| 鸣叫间隔 | 600 ms | 间歇式，两次鸣叫之间至少间隔 600ms |
| 开关控制 | `BUZZER_ENABLE` | 全局开关，设为 `False` 同时关闭蜂鸣器和 GPIO |

每帧调用 `buzz_tick(alert_active)`：
- **检测到人脸**（`alert_active=True`）：计时器控制每 600ms 自动触发一次 120ms 短鸣
- **人脸消失**（`alert_active=False`）：立即将 `buzzer.duty(0)` 静音，防止蜂鸣器卡在鸣叫状态

### GPIO 告警输出

| 引脚 | 信号 | 行为 |
|------|------|------|
| GPIO42 | 人脸检测指示 | 检测到人脸 → 拉高(3.3V)；无人脸 → 拉低(0V) |

可外接 LED（串联 220Ω 限流电阻到 GND）或 3.3V 继电器模块。

### 模型信息

| 属性 | 值 |
|------|-----|
| 模型架构 | RetinaFace（轻量化变体） |
| 输入尺寸 | 320×320, NCHW, uint8 |
| 输出格式 | 检测框 [x, y, w, h] × NMS |
| 锚点数量 | 4200 个（prior_data_320.bin） |
| 置信度阈值 | 0.5 |
| NMS 阈值 | 0.2 |
| 输入分辨率 | 1280×720（传感器直出） |
| 模型文件 | `face_detection_320.kmodel`（571 KB） |

## 文件清单

```
1_face_detection/
├── README.md                     # 本文件
├── face_detection.py             # K230 CanMV 推理主脚本
└── face_detection_320.kmodel     # K230 NPU 推理模型 (571 KB)
```

### face_detection.py 结构说明

| 行号区间 | 内容 |
|----------|------|
| L1-L10 | 依赖导入（libs.PipeLine, AIBase, AI2D, aidemo 等） |
| L12-L13 | `BUZZER_ENABLE` 蜂鸣器全局开关 |
| L15-L30 | 蜂鸣器初始化：FPIOA 配置 GPIO43→PWM1，开机自检短鸣 |
| L32-L69 | `buzz_tick(alert_active)` 非阻塞蜂鸣状态机 |
| L71-L81 | GPIO42 初始化：Pin(42, Pin.OUT) |
| L83-L144 | `FaceDetectionApp(AIBase)` 类：init / 预处理配置 / 后处理 / 绘图 |
| L146-L178 | `__main__` 主程序：PipeLine 创建 + 主循环 |

## K230 板端依赖

以下文件需位于 K230 TF 卡 `/sdcard/` 路径下（CanMV 固件 + 官方 AI Demo 自带，无需额外安装）：

| 依赖 | TF卡路径 | 说明 |
|------|----------|------|
| PipeLine | `/sdcard/examples/libs/PipeLine.py` | sensor→VPSS→OSD→display 图像管线 |
| AIBase | `/sdcard/examples/libs/AIBase.py` | AI 推理基类（KPU 加载+推理抽象） |
| AI2D | `/sdcard/examples/libs/AI2D.py` | 硬件图像预处理（letterbox/resize/pad） |
| Utils | `/sdcard/examples/libs/Utils.py` | 工具函数（ScopedTiming, letterbox_pad_param 等） |
| 锚点数据 | `/sdcard/examples/utils/prior_data_320.bin` | 4200个锚点二进制文件 |
| aidemo | K230 固件内置 C 模块 | `face_det_post_process` / `person_kp_postprocess` 等后处理 |

## 部署步骤

1. 将 `face_detection_320.kmodel` 复制到 TF 卡 `/sdcard/examples/kmodel/`
2. 将 `face_detection.py` 复制到 TF 卡 `/sdcard/examples/05-AI-Demo/`
3. K230 上电，CanMV IDE 连接设备
4. 打开 `face_detection.py` 并运行

## 运行效果

- HDMI / LCD 显示器上实时显示摄像头画面
- 检测到人脸时画黄色矩形框（RGBA: `255, 255, 0, 255`，线宽 2px）
- 蜂鸣器每 600ms 鸣叫 120ms
- GPIO42 输出高电平
- 终端输出初始化日志：
  ```
  [Buzzer] OK - GPIO43 -> PWM1
  [GPIO] OK - GPIO42(face_detected)
  ```

## 配置选项

| 变量 | 位置 | 默认值 | 说明 |
|------|------|--------|------|
| `BUZZER_ENABLE` | L13 | `True` | 蜂鸣器+GPIO 总开关 |
| `BUZZER_GPIO` | L16 | `43` | 蜂鸣器 GPIO 引脚 |
| `BUZZER_PWM` | L17 | `1` | PWM 通道号 |
| `display_mode` | L148 | `"hdmi"` | 显示模式：hdmi / lcd / lt9611 / st7701 |
| `rgb888p_size` | L150 | `[1280, 720]` | 传感器输出分辨率 |
| `confidence_threshold` | L154 | `0.5` | 人脸检测置信度阈值 |
| `nms_threshold` | L155 | `0.2` | NMS IoU 阈值 |
