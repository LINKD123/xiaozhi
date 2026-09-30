# 冰达 NanoPro + 小智语音开车

用 ESP32-S3 小智代替损坏的 Jetson Nano B01，**不改 STM32 固件**，语音控制底盘。

本仓库只放小智侧要改的代码和交接说明，不含整车 ROS 资料（体积过大）。

官方小智源码：[78/xiaozhi-esp32](https://github.com/78/xiaozhi-esp32)

---

## 目标架构

```
语音 → 小智 MCP 工具 → UART 0x5A 速度帧 → STM32 USART1 → 电机
```

- STM32（Nano_Controller V2.0/V2.1，STM32F103RCT6）继续管电机、编码器、IMU、PS2
- 小智假装自己是原来的 Nano：往 USART1 发和 `base_control` 相同的二进制帧
- **整套 Nano 开发套件必须拔掉**，不要叠 40 针，不要接脚 2/4 的 5V

---

## 硬件（已选定）

北科芯域 TELESKY：**ESP32-S3 N16R8 面包板小智套件**（麦、功放、屏、排针）。

- 不要 C3，不要 N8R2
- 商家语雀是刷机/接线，不是二次开发文档
- 屏、麦、功放驱动已在小智仓库的 `bread-compact-wifi` / `bread-compact-wifi-lcd`
- 商家预编译 `.bin` 就是这套源码编出来的；加 UART 要自己编源码，不会把屏和麦弄丢

商家资料：

- [ESP32-S3/C3](https://telesky.yuque.com/bdys8w/01/kbdblr1iq7zr0579)
- [小智AI固件代码](https://telesky.yuque.com/bdys8w/01/ofd7sks9vcr58x4s)（实际是成品 bin，不是工程）
- [搭建 FAQ](https://telesky.yuque.com/bdys8w/01/uwb7r9hg1pwp2yrk)

Windows 烧录 CH340 驱动：给 Type-C 刷机用，和 STM32 那路 UART 无关。

---

## GPIO（面包板紧凑版，与官方 `config.h` 一致）

商家原理图只有开发板本身。麦 / 功放 / OLED 是跳线模块，看 FAQ 实物图。

| 模块 | GPIO |
| --- | --- |
| INMP441 麦 | WS=4，SCK=5，SD=6 |
| MAX98357 功放 | DIN=7，BCLK=15，LRC=16 |
| OLED | SDA=41，SCL=42 |
| 本仓库 UART | **TX=8，RX=9** |

DevKitC-1 排针**没有引出 GPIO17/18**，不要用旧示例里的 17/18。

UART 不要占用上表麦、功放、屏的脚。

---

## 接到小车 40 针

STM32 USART1：PA9 TX / PA10 RX，115200 8N1。排针实际有线的只有：

| 40 针 | 作用 |
| --- | --- |
| 8 | 原 Nano TX → STM32 RX |
| 10 | 原 Nano RX ← STM32 TX |
| 6 等 | GND |
| 2、4 | VDD_5V，**小智不要接**（Nano 短路曾把整车 5V 拉死） |

接线：

- 小智 GPIO8（TX）→ 脚 8
- 小智 GPIO9（RX）→ 脚 10（可先不接）
- 小智 GND → 脚 6
- 共地必须有；电源建议先独立：小智用 Type-C 5V，STM32 用原车电池
- 不要把车上 12V 直接给小智
- USB 插着电脑时不要再并底板 5V

---

## 协议（STM32 不改）

12 字节速度帧，大端 `int16`，单位 mm/s 或 mrad/s（值 ×1000）：

```
5A 0C 01 01  vxH vxL  vyH vyL  wzH wzL  00 FF
```

- 帧头 `0x5A`，长度 `0x0C`，设备/命令 `01 01`
- 末字节 `0xFF` 可跳过 STM32 CRC
- 约 800 ms 收不到新速度会停车，所以前进/转向要每 200 ms 重复发
- **不要发 ASCII**（例如 `FWD`），会打乱 0x5A 组帧

MCP 工具：`self.car.forward/back/left/right/stop`

---

## 如何并进小智工程

1. 克隆 [xiaozhi-esp32](https://github.com/78/xiaozhi-esp32)，ESP-IDF ≥ 5.3
2. `menuconfig` 板型：OLED 用 `bread-compact-wifi`，彩屏用 `bread-compact-wifi-lcd`
3. 对照商家接线图和 `config.h`，不一致只改 `config.h`
4. 把 `xiaozhi_car_mcp_example.cpp` 并进该板的 `InitializeTools()`（构造函数里原有屏、I2S 初始化不要删）
5. 编译烧录；先单独验证语音，再接线开车

MCP 写法见官方 [mcp-usage_zh.md](https://github.com/78/xiaozhi-esp32/blob/main/docs/mcp-usage_zh.md)

---

## 换电脑怎么续

Cursor 聊天记录**不会**随 GitHub 走。本 README 就是交接。新电脑：

```bash
git clone https://github.com/LINKD123/xiaozhi.git
```

把本文件和 `xiaozhi_car_mcp_example.cpp` 丢进新对话即可继续。

尚未做：买板、刷通语音、并代码、接线实测。
