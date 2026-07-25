# 佛祖保佑嵌赛进国二

```text
                        _oo0oo_
                       o8888888o
                       88" . "88
                      (|  -_-  |)
                      0\   =   /0
                    ___/`-----'\___
                  .' \\|       |// '.
                 / \\|||  :  |||// \
                / _||||| -卍- |||||- \
               |   | \\\  -  /// |   |
               | \_|  ''\---/''  |_/ |
               \  .-\__  '-'  ___/-. /
             ___'. .'  /--.--\  `. .'___
          ."" '<  `.___\_<|>_/___.' >' "".
         | | :  `- \`.;`\ _ /`;.`/ - ` : | |
         \  \ `_.   \_ __\ /__ _/   .-` /  /
     =====`-.____`.___ \_____/___.-`___.-'=====
                       `=---='

              佛祖保佑 · 永无 BUG · 永不烧板
```

> 愿代码一次编译通过，硬件一次点亮，联调一路绿灯，嵌赛顺利冲进国二！

## 尼古拉·特斯拉

```text
                    .-""""""""-.
                  .'            '.
                 /   _..----.._   \
                /  .'  //////  '.  \
               |  /   ////////   \  |
               | |   /  _  _ \   | |
               | |      o  o      | |
               | |       /\       | |
               | |     .____.     | |
               |  \    `----'    /  |
                \  '.    __    .'  /
                 '.  `--.__.--'  .'
                   `-._  ||  _.-'
                    /  `-||-'  \
                   / /|  ||  |\ \
                  /_/ |  ||  | \_\
                 /____|__||__|____\
                    /___/\___\
                   /____||____\

       特斯拉保佑 · 直流稳定 · 永不掉压
```

> 愿 12V 动力充沛，5V 纹波归零，3V3 稳如泰山；不反接、不过流，魔烟永远封印在芯片里面！

---

# SmartDesk ── 多模态融合智能桌面交互系统

**基于 STM32H7 + K230 + ESP32-S3 的三芯异构桌面AI系统**

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)
[![MCU](https://img.shields.io/badge/MCU-STM32H7-03234B.svg)](https://www.st.com/stm32h7)
[![AI Engine](https://img.shields.io/badge/AI-STM32Cube.AI-green.svg)](https://www.st.com/en/embedded-software/x-cube-ai.html)
[![Vision](https://img.shields.io/badge/Vision-K230-50B86C.svg)](https://developer.canaan-creative.com)
[![Voice](https://img.shields.io/badge/Voice-ESP32--S3-E85D75.svg)](https://www.espressif.com/en/products/socs/esp32-s3)

> 2026 嵌入式芯片应用竞赛作品

## 目录

- [功能概览](#功能概览)
- [系统架构](#系统架构)
- [硬件清单与引脚](#硬件清单与引脚)
- [工作模式](#工作模式)
- [固件结构](#固件结构)
- [通信协议](#通信协议)
- [AI 模型](#ai-模型)
- [快速开始](#快速开始)
- [仓库结构](#仓库结构)
- [许可证](#许可证)

## 功能概览

| 功能 | 描述 | 技术路线 |
|------|------|----------|
| **🖐 光随手动** | 摄像头检测手部位置，驱动二维云台上的手电筒实时跟随，消除书写阴影 | K230 YOLOv5n → FD帧 → STM32H7 → 步进电机 |
| **🧍 坐姿守护** | 实时监测低头/高低肩，毫秒级灯带变色预警，蜂鸣器同步提醒 | K230 YOLOv8n-Pose → GPIO直连 → STM32H7 → SK9822 |
| **🎭 表情共鸣灯** | 端侧CNN本地推理+云端大模型语义分析，双路融合驱动8种情绪灯效 | STM32 Cube.AI + ESP32 云端LLM |
| **⏱ AI 番茄钟** | 25分钟专注/5分钟休息自动循环，语音或按键触发，色温自动切换 | TIM6硬件定时器 + ESP32 MCP |
| **🗣 全场景语音** | 自然语言控制灯光、番茄钟、模式切换 | ESP32-S3 小智AI + 云端ASR/LLM/TTS |
| **🛡 哨兵模式** | 人脸计数变化法检测陌生人入侵，灯带全红+蜂鸣器告警 | K230 RetinaFace → GPIO44 → STM32H7 |

## 系统架构

```
                      ┌─────────────────────────────┐
                      │         云端 AI 服务          │
                      │   ASR引擎 │ LLM大模型 │ TTS   │
                      └──────────┬──────────────────┘
                                 │ WiFi
                      ┌──────────┴──────────────────┐
                      │        ESP32-S3              │
                      │   小智AI固件 (FreeRTOS)       │
                      │   MCP工具链 → 文本指令转换     │
                      └──────────┬──────────────────┘
                                 │ UART7 (115200)
    ┌──────────┐                  │                   ┌──────────┐
    │   K230   │  USART3+GPIOx3  │   SPI1 (4MHz)     │ SK9822   │
    │ (视觉AI) │◄───────────────►│◄─────────────────►│ (30 LED) │
    │          │                 │                   │          │
    │ RetinaFace│                │                   │ 5500K⇔3200K│
    │ YOLOv8   │                 │  STM32H7          │ 8种情绪  │
    │ YOLOv5n  │                 │  (主控 480MHz)     │ 呼吸/彩虹│
    └──────────┘                 │                   └──────────┘
                                 │
    ┌──────────┐  DCMI+DMA       │  USART2+UART4     ┌──────────┐
    │  OV5640  │◄───────────────►│◄─────────────────►│ 二维云台  │
    │ (摄像头) │                 │    FD 13字节帧     │ 手电筒   │
    └──────────┘                 │                   └──────────┘
                                 │
    ┌──────────┐  SPI6           │  GPIOx5           ┌──────────┐
    │ 1.54 LCD │◄───────────────►│◄─────────────────►│ 5个按键   │
    │ 240×240  │                 │                   └──────────┘
    └──────────┘                 └───────────────────┘
```

## 硬件清单与引脚

### BOM

| 模块 | 型号/规格 | 数量 | 接口 |
|------|-----------|------|------|
| 主控板 | STM32H7 (LXB743ZIT6-P1) | 1 | — |
| 视觉模块 | K230 庐山派 + GC2093 | 1 | USART3 + GPIOx3 |
| 语音模块 | ESP32-S3 N16R8 + INMP441 + MAX98357 | 1 | UART7 |
| 摄像头 | OV5640 500万像素自动对焦 | 1 | DCMI 8bit |
| LCD屏 | 1.54寸 SPI 240×240 | 1 | SPI6 |
| 步进电机 | NEMA17 闭环 + 驱动器 | 2 | USART2, UART4 |
| LED灯带 | SK9822 30颗 | 1 | SPI1 |
| OLED屏 | 0.96寸 SSD1306 128×64 | 1 | ESP32 I2C |
| 电源 | 12V/5A + RK-770降压板 | 1 | — |
| 按键 | 6×6mm 轻触开关 | 5 | GPIO |

### 引脚全映射

| 外设 | 接口 | STM32引脚 | 对方引脚 | 参数 |
|------|------|-----------|----------|------|
| 调试串口 | USART1 | PA9(TX)/PA10(RX) | CH340→PC | 115200 |
| 电机1(左右) | USART2 | PD5(TX) | 闭环驱动器1 | 115200 |
| 电机2(上下) | UART4 | PA0(TX) | 闭环驱动器2 | 115200 |
| K230数据 | USART3 | PB10(TX)/PB11(RX) | K230 UART | 115200 |
| ESP32语音 | UART7 | PF7(TX)/PF6(RX) | ESP32 GPIO17 | 115200 |
| 灯带 | SPI1 | PA5(SCK)/PA7(MOSI) | SK9822 CI/DI | 4MHz |
| LCD屏 | SPI6 | PG8/13/14/15/12 | LCD模块 | 60MHz |
| 摄像头 | DCMI | PA4/6,PC6/7,PD3,PE4-6,PG9-11 | OV5640 | JPG |
| 摄像头I2C | SCCB | PF14(SCL)/PF15(SDA) | OV5640 | 100KHz |
| 坐姿-高低肩 | GPIO IN | PD8 | K230 GPIO42 | 高电平=告警 |
| 坐姿-低头 | GPIO IN | PD9 | K230 GPIO35 | 高电平=告警 |
| 陌生人检测 | GPIO IN | PD10 | K230 GPIO44 | 高电平=告警 |
| 番茄灯键 | GPIO IN | PB12 | 按键→GND | 内部上拉 |
| 光随手动键 | GPIO IN | PB13 | 按键→GND | 内部上拉 |
| 哨兵键 | GPIO IN | PB14 | 按键→GND | 内部上拉 |
| 亮度+ | GPIO IN | PD11 | 按键→GND | 内部上拉 |
| 亮度- | GPIO IN | PD12 | 按键→GND | 内部上拉 |
| 板载LED | GPIO OUT | PC13 | LED1 | 低电平亮 |

## 工作模式

系统通过按键或语音在 4 种模式间切换：

| 模式 | 按键 | 核心行为 | 灯光 |
|------|------|----------|------|
| 情绪调节 | MODE 0 (串口) | 端侧AI表情识别 + 云端语义情绪 → 灯效 | 8种情绪灯效 |
| 番茄灯 | PB12 | TIM6 25+5min倒计时，色温自动切换 | 5500K⇔3200K |
| 光随手动 | PB13 | K230手势→FD帧→电机→手电筒跟随 | 正常照明 |
| 哨兵 | PB14 | 人脸计数法，陌生人→GPIO44→告警 | 红灯闪烁 |

> 坐姿检测（GPIO直连）在**任何模式**下都生效，优先级最高。

## 固件结构

```
firmware/
├── Core/                      # 核心层
│   ├── main.c                 # 主循环：摄像头+AI推理+4模式状态机
│   ├── irq_handlers.c         # 中断服务：DCMI/DMA/USART/TIM
│   ├── main.h                 # 主头文件
│   ├── hal_conf.h             # HAL模块配置 (CRC/TIM/USART/SPI/DCMI)
│   └── startup.s              # 启动文件 (Stack:8KB, Heap:4KB)
├── Drivers/                   # 驱动层
│   ├── uart.c/h               # 5路UART驱动 (USART1-3, UART4, UART7)
│   ├── motor.c/h              # 步进电机FD帧 + 3路姿势GPIO检测
│   ├── led_strip.c/h          # SK9822 SPI驱动 + 色温转换 + 动画
│   ├── light.c/h              # 灯光控制：8情绪+番茄钟状态机
│   ├── state.c/h              # 4模式系统状态机
│   ├── serial.c/h             # 串口命令解析 (USART1中断驱动)
│   ├── button.c/h             # 5按键扫描 (30ms去抖)
│   ├── camera.c/h             # OV5640 DCMI驱动 + DMA
│   ├── lcd.c/h                # 1.54" SPI LCD驱动 + 字库
│   └── board_led.c/h          # 板载LED (PC13)
├── AI/                        # AI推理层
│   ├── network.c/h            # 模型运行时 (X-CUBE-AI生成)
│   ├── network_data.c/h       # 权重表声明
│   ├── network_data_params.c/h # 模型权重 (~60KB Flash)
│   └── network_config.h       # 模型配置
└── Protocol/                  # 协议层
    ├── k230.c/h               # K230二进制帧解析 (0xAA 0x55头)
    └── esp32.c/h              # ESP32文本指令解析 (环形缓冲+strstr)
```

## 通信协议

### K230 → STM32 (USART3, 115200)

**二进制协议帧** `0xAA 0x55 + LEN + CMD + PAYLOAD + XOR`

| CMD | 功能 | 数据 |
|-----|------|------|
| 0x01 | 手势坐标 | `[ID, X_L, X_H, Y_L, Y_H]` |
| 0x02 | 坐姿状态 | `[状态]` (0=正常 1=低头 2=高低肩) |
| 0x03 | 表情分类 | `[ID]` (0-6: fer2013七分类) |
| 0x05 | 哨兵告警 | `[等级]` |

**FD 电机控制帧** (13字节二进制)

```
[Addr, 0xFD, Dir, SpeedHI, SpeedLO, Accel, Pulse[4], 0x00, 0x00, 0x6B]
 Byte0  Byte1 Byte2  Byte3     Byte4    Byte5  Byte6-9  Byte10-11 Byte12
```

支持 ASCII Hex (`01FD00640A0000005900006B`) 和原始二进制双模输入。

### ESP32 → STM32 (UART7, 115200)

明文文本指令，`\n` 结束：

```
EMOTION:angry        # 冷蓝呼吸灯
EMOTION:sad          # 暖黄呼吸灯
EMOTION:happy        # 彩虹微闪
EMOTION:neutral      # 4000K白光
EMOTION:confused     # 淡紫呼吸灯
LIGHT:MODE:focus     # 5500K专注白光
LIGHT:MODE:rest      # 3200K休息暖光
LIGHT:MODE:off       # 关闭灯光
POMODORO:START:25    # 启动25分钟番茄钟
```

## AI 模型

### 端侧表情识别 (STM32 Cube.AI)

| 属性 | 值 |
|------|-----|
| 架构 | fer_mini_xception (quantized) |
| 输入 | 64×64×1, int8 |
| 输出 | 7类 (angry/disgust/fear/happy/sad/surprise/neutral) |
| 权重 | 59,884 字节 (Flash) |
| Activations | 96,112 字节 (SRAM, 静态分配) |
| 推理耗时 | ~20ms @480MHz |
| 推理频率 | 每5帧一次 (~8 FPS) |
| 预处理 | 240×240 RGB565 → 中心裁切 → 2x降采样 → 灰度 → int8 |

### K230 端侧模型

| 模型 | 架构 | 输入 | 用途 |
|------|------|------|------|
| face_detection_320.kmodel | RetinaFace | 320×320 | 人脸检测 |
| yolov8n_pose.kmodel | YOLOv8n-Pose | 320×320 | 人体17关键点 |
| hand_pen kmodel | YOLOv5n | 320×320 | 手部检测 |

## 快速开始

### 1. STM32 固件

```bash
# Keil MDK v5 打开 firmware/Core/main.c 所在工程
# 目标芯片: STM32H743ZIT6
# 需要启用: HAL_CRC_MODULE_ENABLED
# 添加头文件路径: Middlewares/ST/AI/Inc
# 编译 → ST-Link 或 USB DFU 下载
```

### 2. K230 视觉模块

```bash
# 将 k230_scripts/ 下所有文件复制到 TF 卡:
#   .kmodel → /sdcard/examples/kmodel/
#   .py    → /sdcard/examples/05-AI-Demo/
# CanMV IDE 连接 K230 运行脚本
```

### 3. ESP32 语音模块

```bash
# 将 esp32_bridge/compact_wifi_board.cc 放入小智AI源码:
#   main/boards/bread-compact-wifi/
# 编译: python scripts/release.py bread-compact-wifi
# 烧录: esptool write_flash 0x0 build/merged-binary.bin
```

### 4. 接线

参照[引脚全映射表](#引脚全映射)连接所有模块。**关键**：所有模块 GND 必须共地。12V 电源经 RK-770 降压板为灯带和电机供电。

## 仓库结构

```
qiansai_2026/
├── README.md                     # 本文件
├── LICENSE                       # Apache 2.0
├── .gitignore
├── firmware/                     # STM32H7 固件源码
│   ├── Core/                     #   核心层 (main + IRQ + startup)
│   ├── Drivers/                  #   驱动层 (UART/Motor/LED/Light/...)
│   ├── AI/                       #   AI推理层 (Cube.AI模型)
│   └── Protocol/                 #   协议层 (K230 + ESP32)
├── k230_scripts/                 # K230 MicroPython脚本 + 模型
│   ├── face_detection.py         #   人脸检测 (含陌生人检测)
│   ├── face_detection_320.kmodel #   RetinaFace模型
│   ├── posture_detect_k230.py    #   坐姿检测
│   └── yolov8n_pose.kmodel       #   YOLOv8n-Pose模型
└── esp32_bridge/                 # ESP32-S3 小智AI桥接代码
    ├── compact_wifi_board.cc     #   主板桥接类
    └── project_handover.md       #   交接文档
```

## 许可证

Apache License 2.0 ── 详见 [LICENSE](LICENSE)

## 致谢

| 项目 | 用途 |
|------|------|
| [STM32Cube.AI](https://www.st.com/x-cube-ai) | 端侧AI推理引擎 |
| [face_classification](https://github.com/oarriaga/face_classification) | fer_mini_xception 模型 |
| [小智AI](https://github.com/78/xiaozhi-esp32) | ESP32语音助手固件 |
| [Ultralytics YOLO](https://github.com/ultralytics/ultralytics) | 目标检测/姿态估计模型 |
| [K230 SDK](https://developer.canaan-creative.com) | RISC-V KPU 推理框架 |
| [LeRobot-Anything](https://github.com/MINT-SJTU/LeRobot-Anything-U-Arm) | 机械结构参考 |
