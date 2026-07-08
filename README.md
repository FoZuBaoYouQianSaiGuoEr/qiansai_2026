# SmartDesk ── Multimodal Intelligent Desktop Interaction System

**基于 STM32H7 的多模态融合智能桌面交互系统**

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)
[![MCU](https://img.shields.io/badge/MCU-STM32H7-03234B.svg)](https://www.st.com/stm32h7)
[![AI](https://img.shields.io/badge/AI-STM32Cube.AI-green.svg)](https://www.st.com/en/embedded-software/x-cube-ai.html)
[![ESP32](https://img.shields.io/badge/Voice-ESP32--S3-red.svg)](https://www.espressif.com/en/products/socs/esp32-s3)

A smart desktop companion that integrates AI vision, voice interaction, and adaptive lighting to enhance focus, correct posture, and create an intelligent workspace.

##  Features

| Feature | Description |
|---------|-------------|
| **Light Tracking** | K230 camera detects hand position → FD command frame → stepper gimbal follows hand with flashlight, eliminating writing shadows |
| **Posture Guardian** | YOLOv8n-Pose detects 17 keypoints → GPIO direct connection (<1ms response) → LED strip alerts for head-down/shoulder-tilt |
| **Emotion Resonance** | Dual-path: STM32 Cube.AI runs fer_mini_xception locally (8 expressions) + ESP32 cloud LLM semantic emotion → ambient lighting |
| **AI Pomodoro** | TIM6 hardware timer → 25min focus (5500K) / 5min rest (3200K) auto-cycle, voice-controllable |
| **Voice & Sentry** | ESP32-S3 running xiaozhi-ai firmware; face-count change detection for intruder alert |

##  System Architecture

```
  +----------+   USART3   +----------+   USART2   +-----------+
  |   K230   |----------->|          |----------->| Motor 1   |
  | (Vision) |  GPIO x3   | STM32H7  |   UART4    | (Pan)     |
  +----------+----------->|  (Main)  |----------->+-----------+
                          |          |   UART7    +-----------+
  +----------+  DCMI      |          |<-----------| ESP32-S3  |
  |  OV5640  |----------->|          |   SPI1     | (Voice)   |
  | (Camera) |            |          |----------->+-----------+
  +----------+            +----------+            | SK9822    |
                          |   GPIO               | (30 LEDs) |
  [5 Buttons]------------>|                      +-----------+
```

##  Hardware

| Module | Component | Interface |
|--------|-----------|-----------|
| MCU | STM32H7 (Cortex-M7, 480MHz) | ── |
| Vision | K230 + GC2093 + KPU | USART3 + GPIO |
| Camera | OV5640 5MP | DCMI (8-bit parallel + DMA) |
| Voice | ESP32-S3 N16R8 | UART7 (115200 bps) |
| Motor | NEMA17 closed-loop ×2 | USART2, UART4 (FD 13-byte frame) |
| LED Strip | SK9822 ×30 | SPI1 (4MHz) |
| LCD | 1.54" SPI 240×240 | SPI6 (60MHz) |

### Pin Mapping

| Peripheral | STM32 Pin | Function |
|-----------|-----------|----------|
| USART1 | PA9/PA10 | Debug / Serial CMD |
| USART2 | PD5 | Motor 1 (pan) |
| UART4 | PA0 | Motor 2 (tilt) |
| USART3 | PB10/PB11 | K230 data |
| UART7 | PF6/PF7 | ESP32 commands |
| SPI1 | PA5/PA7 | SK9822 LED strip |
| SPI6 | PG8/13/14/15/12 | LCD display |
| DCMI | PA4/6,PC6/7,PD3,PE4-6,PG9-11 | OV5640 |
| GPIO IN | PD8/PD9/PD10 | Posture alerts (K230→STM32) |
| GPIO IN | PB12/13/14,PD11/12 | 5 buttons |

##  Firmware Structure

```
firmware/
├── Core/
│   ├── main.c              # Main loop (camera + AI + 4-mode state machine)
│   ├── irq_handlers.c      # Interrupt handlers
│   ├── main.h / hal_conf.h # Headers
│   └── startup.s           # Startup (Stack: 8KB, Heap: 4KB)
├── Drivers/
│   ├── uart.c/h            # 5-channel UART driver
│   ├── motor.c/h           # Stepper motor FD frame + posture GPIO
│   ├── led_strip.c/h       # SK9822 SPI driver, color temp, animations
│   ├── light.c/h           # 8 emotion effects + Pomodoro state machine
│   ├── state.c/h           # 4-mode system state machine
│   ├── serial.c/h          # Serial command parser (USART1 IRQ)
│   ├── button.c/h          # 5-button scanner
│   ├── camera.c/h          # OV5640 DCMI driver
│   ├── lcd.c/h             # 1.54" SPI LCD driver
│   └── board_led.c/h       # Onboard LED
├── AI/
│   ├── network.c/h         # AI model runtime
│   ├── network_data.c/h    # Model weights (~60KB Flash)
│   └── network_config.h    # Model config
└── Protocol/
    ├── k230.c/h            # K230 binary frame parser (0xAA 0x55)
    └── esp32.c/h           # ESP32 text command parser (strstr match)
```

##  Build

### STM32 Firmware
1. Open with **Keil MDK v5** (ARM Compiler 5/6)
2. Select target: `STM32H743`
3. Add Middlewares: `Middlewares/ST/AI/Inc` to include path
4. Enable `HAL_CRC_MODULE_ENABLED` in `hal_conf.h`
5. Build → Flash via ST-Link or USB DFU

### K230 Scripts
See `k230_scripts/` ── deploy `.kmodel` and `.py` to TF card.

### ESP32 Bridge
See `esp32_bridge/` ── compile with ESP-IDF: `python scripts/release.py bread-compact-wifi`

##  Communication Protocols

### K230 → STM32 (USART3, 115200 bps)
- **Binary Frame**: `0xAA 0x55 + LEN + CMD + PAYLOAD + XOR` (gesture/emotion/posture/sentry)
- **FD Frame**: 13-byte motor command `[Addr,0xFD,Dir,Speed[2],Accel,Pulse[4],Rsvd[2],0x6B]` (ASCII hex or raw binary)

### ESP32 → STM32 (UART7, 115200 bps)
Text commands, `\n` terminated:
```
EMOTION:sad           # Set emotion lighting
LIGHT:MODE:focus      # Cool white (5500K)
LIGHT:MODE:rest       # Warm light (3200K)
LIGHT:MODE:off        # Turn off
POMODORO:START:25     # Start Pomodoro timer
```

### STM32 Serial CMD (USART1, 115200 bps)
```
M 0 15      # Motor 0 forward 15 degrees
MR 0 15     # Motor 0 reverse 15 degrees
MODE 2      # Switch to Hand-Follow mode
BR 80       # Brightness 80%
TEMP 5500   # Color temperature 5500K
HELP        # Command list
```

##  STM32 Cube.AI Model

| Property | Value |
|----------|-------|
| Architecture | fer_mini_xception (quantized) |
| Input | 64×64×1, int8 |
| Output | 7 classes, int8 |
| Weights | 59,884 bytes (Flash) |
| Activations | 96,112 bytes (SRAM, static allocated) |
| Inference | ~20ms @480MHz, every 5th frame (~8 FPS) |

##  License

Apache License 2.0 ── see [LICENSE](LICENSE)

##  Acknowledgments

- [X-CUBE-AI](https://www.st.com/en/embedded-software/x-cube-ai.html) ── STM32 AI expansion pack
- [face_classification](https://github.com/oarriaga/face_classification) ── fer_mini_xception model
- [xiaozhi-esp32](https://github.com/78/xiaozhi-esp32) ── ESP32 AI voice assistant
- [LeRobot-Anything-U-Arm](https://github.com/MINT-SJTU/LeRobot-Anything-U-Arm) ── Hardware inspiration
- [YOLOv8](https://github.com/ultralytics/ultralytics) ── Ultralytics
- [K230 SDK](https://developer.canaan-creative.com) ── Canaan Creative
