# 智能学习护眼台灯项目 ── ESP32-S3 语音助手模块交接与代码包文档

本篇交接文档详细记录了 **ESP32-S3 (N16R8) 语音助手模块** 在本项目中的系统架构、硬件接线规范、串口交互协议、修改的全部核心代码以及编译烧录指南。您可以直接复制本 MD 文档进行项目交付或归档。

---

## 一、 项目整体架构与双主控协同

本项目采用 **双主控（STM32 + ESP32-S3）** 的桥接架构，以应对多模态视觉处理和低延迟语音交互的双重挑战：
*   **视觉与控制中心（STM32 + K230）**：K230 负责摄像头的人脸、手势与坐姿识别；STM32 作为底层执行器负责台灯 PWM 调光（SK9822）、云台电机转向和极速报警控制。
*   **语音与 AI 网关（ESP32-S3）**：负责用户语音拾取、上传 AI 云端大模型、接收 TTS 音频流播放；同时通过 MCP 协议将大模型意图翻译为控制命令，通过 **UART1** 串口下发给 STM32。

---

## 二、 硬件接口与引脚连接规范

ESP32-S3 模块的硬件接口采用官方 `bread-compact-wifi` 面包板标准并外引串口线：

### 1. I2S 语音输入与输出（INMP441 麦克风 & MAX98357 功放）
*   **INMP441 数字麦克风**：
    *   `VDD` ──> **3.3V**
    *   `GND` ──> **GND**
    *   `L/R` ──> **GND**（拉低代表单声道/左声道模式）
    *   `SCK` ──> **GPIO 5** (I2S BCLK 时钟)
    *   `WS` ──> **GPIO 4** (I2S WS 帧选择)
    *   `SD` ──> **GPIO 6** (I2S DIN 数据输入)
*   **MAX98357 数字功放**：
    *   `VIN` ──> **5V**（提供高达 3.2W 的音频放音输出功率）
    *   `GND` ──> **GND**
    *   `DIN` ──> **GPIO 7** (I2S DOUT 数据输出)
    *   `BCLK` ──> **GPIO 15** (I2S BCLK 时钟)
    *   `LRC` ──> **GPIO 16** (I2S WS 帧选择)

### 2. 0.96 / 1.3 寸 OLED 屏幕（SSD1306）
ESP32 本地保留一颗物理 OLED，用于呈现小智小眼睛表情动画和连网状态：
*   `SDA` ──> **GPIO 41** (I2C 数据线)
*   `SCL` ──> **GPIO 42** (I2C 时钟线)
*   `VCC` ──> **3.3V**
*   `GND` ──> **GND**

### 3. UART1 串口桥接（ESP32 ──> STM32）
*   `TXD` (GPIO 17) ──> **STM32 的串口 RX 引脚**
*   `GND` ──> **STM32 的 GND**（**必须共地**，否则无法正常进行串口数据接收）
*   *(由于报警处理已在 K230/STM32 侧闭环，ESP32 仅做下发，因此 RX (GPIO 18) 无需接线)*

---

## 三、 串口通信协议设计

所有的串口指令均以 **115200 波特率** 的明文文本方式发送，且每条指令末尾都必须带有换行符 **`\n`**。

### 1. 护眼灯光调节协议
*   **高色温专注白光**：`LIGHT:MODE:focus\n`
*   **低色温温润暖光**：`LIGHT:MODE:rest\n`
*   **关闭灯光**：`LIGHT:MODE:off\n`

### 2. 节律番茄钟状态机控制协议
*   **启动番茄钟（默认为25分钟）**：`POMODORO:START:25\n`
*   *(STM32 接收后启动本地 `light_ctrl.c` 中的番茄钟倒计时)*

### 3. 云端大模型情绪氛围灯控制协议
大模型在分析当前对话语义后将情绪代码下传：
*   **沮丧/悲伤**：`EMOTION:sad\n` ──> STM32 切换为治愈系粉色呼吸灯并播乐。
*   **快乐/兴奋**：`EMOTION:happy\n` ──> STM32 切换为动感暖色灯效。
*   **平常/冷静**：`EMOTION:neutral\n` ──> STM32 恢复正常环境照度。
*   **其他情绪**：`EMOTION:confused\n`、`EMOTION:angry\n`。

---

## 四、 核心代码修改打包

所有的修改已直接无缝嵌入在官方开发板文件 `main/boards/bread-compact-wifi/compact_wifi_board.cc` 中。

以下是我们在该文件中加入的**全部代码块**：

### 1. 顶部新增的头文件与串口常量定义
```cpp
#include <driver/uart.h>

#define STM32_UART_NUM           UART_NUM_1
#define STM32_UART_TX_PIN        GPIO_NUM_17
#define STM32_UART_BAUD_RATE     115200
```

### 2. 桥接显示类 `BridgeOledDisplay` 的实现
该类继承自官方 OLED 类，在重写表情动作的同时，通过串口透传发给 STM32：
```cpp
class BridgeOledDisplay : public OledDisplay {
public:
    BridgeOledDisplay(esp_lcd_panel_io_handle_t io, esp_lcd_panel_handle_t panel, int width, int height, bool mirror_x, bool mirror_y)
        : OledDisplay(io, panel, width, height, mirror_x, mirror_y) {}

    virtual void SetEmotion(const char* emotion) override {
        // 1. 让连接在 ESP32 上的物理屏幕画出眼部动画
        OledDisplay::SetEmotion(emotion);
        
        // 2. 将情绪数据转换为自定义命令字通过串口发给 STM32
        std::string uart_cmd = "EMOTION:" + std::string(emotion) + "\n";
        uart_write_bytes(STM32_UART_NUM, uart_cmd.c_str(), uart_cmd.length());
        ESP_LOGI("BridgeOledDisplay", "Forwarded emotion to STM32: %s", emotion);
    }
};
```

### 3. ESP32 串口发送通道初始化
在 `CompactWifiBoard` 类中定义：
```cpp
    void InitializeUart() {
        uart_config_t uart_config = {
            .baud_rate = STM32_UART_BAUD_RATE,
            .data_bits = UART_DATA_8_BITS,
            .parity    = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .source_clk = UART_SCLK_DEFAULT,
        };
        ESP_ERROR_CHECK(uart_param_config(STM32_UART_NUM, &uart_config));
        ESP_ERROR_CHECK(uart_set_pin(STM32_UART_NUM, STM32_UART_TX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
        ESP_ERROR_CHECK(uart_driver_install(STM32_UART_NUM, 256, 0, 0, NULL, 0)); // 仅发送模式，无需开辟接收环形缓冲区
    }
```
并在 `CompactWifiBoard()` 构造函数的第一行调用该初始化函数：
```cpp
    CompactWifiBoard() : ... {
        InitializeUart(); // 优先拉起串口
        InitializeDisplayI2c();
        InitializeSsd1306Display();
        ...
    }
```

### 4. 替换屏幕实例化对象
在 `InitializeSsd1306Display()` 函数末尾，将实例化代码改为我们定制的包装类：
```cpp
        // 原代码：display_ = new OledDisplay(...)
        // 修改为如下代码：
        display_ = new BridgeOledDisplay(panel_io_, panel_, DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y);
```

### 5. 绑定云端 AI 物联网控制工具 (MCP Tools)
在 `InitializeTools()` 方法中，将大模型执行的语音意图解析并转换发送：
```cpp
    void InitializeTools() {
        static LampController lamp(LAMP_GPIO);
        auto& mcp = McpServer::GetInstance();

        // 注册控制学习台灯的工具 (在大模型说"打开学习灯"等话术时自动触发)
        mcp.AddTool("self.lamp.set_mode", 
            "Set the study lamp mode. Arguments 'mode' can be: 'focus' (cool white light for focus), 'rest' (warm light for relaxation), or 'off' (turn off).",
            PropertyList({
                Property("mode", kPropertyTypeString)
            }),
            [this](const PropertyList& properties) -> ReturnValue {
                std::string mode = properties["mode"].value<std::string>();
                std::string uart_cmd = "LIGHT:MODE:" + mode + "\n";
                uart_write_bytes(STM32_UART_NUM, uart_cmd.c_str(), uart_cmd.length());
                ESP_LOGI(TAG, "Sent to STM32: %s", uart_cmd.c_str());
                return true;
            });

        // 注册启动番茄钟的工具
        mcp.AddTool("self.pomodoro.start", 
            "Start the Pomodoro timer.",
            PropertyList({
                Property("duration_minutes", kPropertyTypeInteger, 1, 120)
            }),
            [this](const PropertyList& properties) -> ReturnValue {
                int mins = properties["duration_minutes"].value<int>();
                std::string uart_cmd = "POMODORO:START:" + std::to_string(mins) + "\n";
                uart_write_bytes(STM32_UART_NUM, uart_cmd.c_str(), uart_cmd.length());
                ESP_LOGI(TAG, "Sent to STM32: %s", uart_cmd.c_str());
                return true;
            });
    }
```

---

## 五、 STM32 端数据接收与指令解析代码 (C语言)

在 STM32 端的接收串口中断服务函数或独立线程中，可以使用以下简易的指针切割代码来匹配执行指令：

```c
#include <string.h>
#include <stdlib.h>

// 串口接收换行符 \n 后的消息包处理函数
void Process_ESP32_Uart_Cmd(char *cmd_buf) {
    char *token;

    // 1. 过滤解析灯光模式
    if ((token = strstr(cmd_buf, "LIGHT:MODE:")) != NULL) {
        char light_mode[12];
        sscanf(token + 11, "%s", light_mode);
        
        if (strcmp(light_mode, "focus") == 0) {
            // 执行：调整为清透高色温白光 (专注状态)
        } else if (strcmp(light_mode, "rest") == 0) {
            // 执行：调整为柔和低色温暖光 (休息状态)
        } else if (strcmp(light_mode, "off") == 0) {
            // 执行：关闭灯光
        }
    }
    
    // 2. 过滤解析番茄钟开启
    else if ((token = strstr(cmd_buf, "POMODORO:START:")) != NULL) {
        int duration_mins = atoi(token + 15);
        // 执行：启动番茄钟，设定专注时间为 duration_mins 分钟并驱动对应灯带
    }
    
    // 3. 过滤解析云端情绪变化
    else if ((token = strstr(cmd_buf, "EMOTION:")) != NULL) {
        char mood[16];
        sscanf(token + 8, "%s", mood);
        
        // 配合 light_ctrl.c 执行对应的情绪灯光动画或呼吸渐变
        if (strcmp(mood, "sad") == 0) {
            // 触发治愈粉色温暖呼吸灯
        } else if (strcmp(mood, "happy") == 0) {
            // 触发欢快律动灯效
        } else if (strcmp(mood, "neutral") == 0) {
            // 恢复为默认白光模式
        }
    }
}
```

---

## 六、 编译、发布与烧录流程

### 1. 编译生成发布包
在包含 ESP-IDF 编译工具链的环境中，执行以下指令，将会自动编译并生成打包的发布压缩包：
```powershell
# 打包编译官方面包板程序（已植入我们的串口代码）
python scripts/release.py bread-compact-wifi
```
编译成功后，在项目根目录的 `releases/` 目录下会生成 `v2.2.6_bread-compact-wifi.zip` 固件包，其内已经包含了合并好的单一二进制固件 **`merged-binary.bin`**。

### 2. 烧录指令
使用如下命令，直接将合并好的固件烧录至您的 ESP32-S3 芯片中（请将 `COM3` 修改为您的 TTL 或 S3 板载串口）：
```powershell
python -m esptool --chip esp32s3 -p COM3 -b 460800 --before default_reset --after hard_reset write_flash 0x0 build/merged-binary.bin
```
