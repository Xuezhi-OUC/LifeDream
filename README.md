# LifeDream — 人生有梦队 · 2026 瑞萨全国大学生电子设计竞赛

> **队伍**: 人生有梦队  
> **竞赛**: 2026 瑞萨全国大学生电子设计竞赛  
> **MCU**: Renesas RA8P1 (Cortex-M85)  
> **开发板**: RA8P1 EK Evaluation Kit  
> **IDE**: e2 studio (Eclipse)  
> **SDK**: FSP v6.5.1  
> **工具链**: GCC ARM Embedded 13.2.1

---

## 目录

- [项目概述](#项目概述)
- [硬件平台](#硬件平台)
- [软件架构](#软件架构)
- [工程结构](#工程结构)
- [外设配置](#外设配置)
- [核心功能](#核心功能)
- [通信协议](#通信协议)
- [开发环境搭建](#开发环境搭建)
- [构建与烧录](#构建与烧录)
- [调试说明](#调试说明)
- [已知问题](#已知问题)
- [参考资料](#参考资料)

---

## 项目概述

本项目基于瑞萨 **RA8P1** 高性能微控制器（ARM Cortex-M85），围绕瑞萨 2026 全国大学生电子设计竞赛展开。项目已完成 SPI 主从通信协议的实现与调试，通过 SPI 接口以 1 Hz 频率向从机发送正弦波数据，并通过 UART 输出调试信息。此外，已完成 OV2640 摄像头（通过 CEU 接口）的驱动移植准备工作。

### 当前里程碑

- [x] SPI 主站通信（7.8125 MHz, Mode 0）
- [x] 1 Hz 正弦波数据生成与发送
- [x] UART 调试输出（115200 baud）
- [x] SPI 协议文档 v1.0
- [ ] OV2640 摄像头驱动集成（进行中）
- [ ] CEU 图像采集与处理
- [ ] 上位机通信

---

## 硬件平台

### MCU: R7FA8P1KFC (RA8P1 系列)

| 参数 | 规格 |
|---|---|
| **核心** | ARM Cortex-M85 @ 1 GHz |
| **FPU** | 单精度 + 双精度 |
| **MVE** | M-Profile Vector Extension (Helium) |
| **SRAM** | 1.5 MB（含 256 KB TCM） |
| **Flash** | 2 MB |
| **DSP** | 支持 Cortex-M85 DSP 指令集 |
| **封装** | 176 引脚 LQFP |

### 板载外设 (RA8P1 EK)

| 外设 | 说明 |
|---|---|
| **LED** | 蓝色 LED (P1.10)，用户指示灯 |
| **USB** | USB 串口（J-Link 调试器 + 虚拟 COM 端口） |
| **调试器** | 板载 SEGGER J-Link |
| **晶振** | 24 MHz 外部主晶振 |

### 引脚分配

| 引脚 | 功能 | 说明 |
|---|---|---|
| P1.10 | DO | 蓝色 LED 控制 |
| P6.00 | DO | 板载蓝色 LED（原 SPI CS，已废弃） |
| P6.01 | SCK | SPI 时钟（SCI SPI0） |
| P6.02 | MOSI | SPI 主出从入 |
| P6.03 | MISO | SPI 主入从出 |
| P6.04 | DO | SPI 片选（GPIO 手动控制） |
| P9.00 | TXD | UART9 发送 |
| P9.01 | RXD | UART9 接收 |

> **注意**: P6.00 默认配置为 SCI 外设功能而非 GPIO，导致片选无法正常控制。已通过修改 `pin_data.c` 将 CS 移至 P6.04 并用 GPIO 软件控制。

---

## 软件架构

```
┌─────────────────────────────────────┐
│          hal_entry.c                │  ← 用户应用入口
│  正弦波生成 + SPI 发送 + UART 日志   │
└──────────┬──────────────────────────┘
           │
┌──────────▼──────────────────────────┐
│        FSP HAL 层                   │
│  r_sci_b_spi  │  r_sci_b_uart       │
│  r_iic_master │  r_ceu              │
│  r_ioport                          │
└──────────┬──────────────────────────┘
           │
┌──────────▼──────────────────────────┐
│        BSP 层                       │
│  时钟初始化 / 中断向量 / 引脚配置    │
└──────────┬──────────────────────────┘
           │
┌──────────▼──────────────────────────┐
│      CMSIS-Core (ARM)               │
│  Cortex-M85 内核抽象层               │
└─────────────────────────────────────┘
```

### 主循环流程

```
系统上电 → BSP 初始化 → 外设打开 (UART/SPI)
         → 进入主循环:
             1. 计算 sin(angle) → 0~255
             2. 组帧 (SOF + CMD + LEN + X + Y + CRC)
             3. 闪烁 LED (50 ms)
             4. 拉低 CS → SPI 发送 → 拉高 CS
             5. UART 打印正弦数据
             6. angle += 10°
             7. 等待 1 秒
```

---

## 工程结构

```
LifeDream/
├── src/                      # 用户应用源代码
│   ├── hal_entry.c           # 主程序入口
│   └── hal_warmstart.c       # BSP 热启动处理
│
├── ra_gen/                   # FSP 生成代码 (请勿手动编辑)
│   ├── main.c                # main() 入口 → 调用 hal_entry()
│   ├── hal_data.h/c          # 外设实例配置数据
│   ├── common_data.h/c       # IOPORT 实例
│   ├── bsp_clock_cfg.h       # 时钟树配置
│   ├── pin_data.c            # 引脚功能配置
│   ├── vector_data.h/c       # 中断向量表
│   └── bsp_irq_priority.h    # 中断优先级配置
│
├── ra_cfg/                   # FSP 模块配置头文件
│   └── fsp_cfg/
│       ├── bsp/              # BSP 配置
│       ├── r_ceu_cfg.h       # CEU 配置
│       ├── r_iic_master_cfg.h # I2C 配置
│       ├── r_ioport_cfg.h    # IOPORT 配置
│       ├── r_sci_b_spi_cfg.h # SPI 配置 (DMA 启用)
│       └── r_sci_b_uart_cfg.h # UART 配置
│
├── ra/                       # FSP 驱动库 (BSP + HAL)
│   ├── arm/                  # CMSIS v6 内核头文件
│   ├── board/ra8p1_ek/       # RA8P1 EK 板级支持包
│   └── fsp/                  # FSP API 头文件与驱动源码
│       ├── inc/api/          # 外设 API 声明
│       ├── inc/instances/    # 外设实例声明
│       └── src/              # 外设驱动实现
│
├── script/
│   └── fsp.ld                # 链接脚本
│
├── docs/
│   ├── SPI_Protocol.md       # SPI 通信协议规范 v1.0
│   ├── SPI_Test_Log.md       # SPI 调试日志
│   └── ov2640_CODE_REF.txt   # OV2640 驱动移植参考
│
├── Debug/                    # 构建输出
│   ├── LifeDream.elf         # 可执行文件
│   ├── LifeDream.map         # 内存映射
│   ├── LifeDream.srec        # SREC 烧录文件
│   └── compile_commands.json # 编译数据库 (clangd)
│
├── configuration.xml         # FSP RA 配置主文件
├── .cproject / .project      # e2 studio 项目配置
└── .clangd                   # clangd 代码分析配置
```

---

## 外设配置

### 时钟树

| 时钟 | 频率 | 来源 |
|---|---|---|
| **CPUCLK** | 1 GHz | PLL1 (24 MHz * 83.33) |
| **PLL1** | 2.0 GHz | 24 MHz 外部晶振 |
| **PLL2** | 2.4 GHz | 24 MHz 外部晶振 |
| **ICLK** | 500 MHz | PLL1 / 4 |
| **PCLKA** | 250 MHz | PLL1 / 8 |
| **PCLKB** | 125 MHz | PLL1 / 16 |
| **PCLKD** | 125 MHz | PLL1 / 16 |
| **UCLK** | 62.5 MHz | PLL2 / 12 |
| **FCLK** | 62.5 MHz | PLL1 / 32 |

### SPI (SCI SPI0)

| 参数 | 值 |
|---|---|
| **模式** | 主模式 (Master) |
| **时钟频率** | 7.8125 MHz |
| **CPOL** | 0 (空闲低电平) |
| **CPHA** | 0 (第一个时钟沿采样) |
| **位序** | MSB First |
| **数据长度** | 8 位 |
| **片选** | P6.04 GPIO 软件控制 |

### UART (SCI9)

| 参数 | 值 |
|---|---|
| **波特率** | 115200 |
| **数据位** | 8 |
| **校验** | 无 |
| **停止位** | 1 |
| **流控** | 无 |

### I2C (IIC0)

| 参数 | 值 |
|---|---|
| **模式** | 主模式 (Standard) |
| **速率** | ~100 kHz |
| **地址模式** | 7 位 |
| **用途** | OV2640 摄像头初始化 (SCCB 协议) |

### CEU (Capture Engine Unit)

| 参数 | 值 |
|---|---|
| **分辨率** | 640 × 480 (VGA) |
| **像素格式** | YCbCr422 |
| **数据总线** | 8 位 |
| **用途** | OV2640 摄像头图像采集 |

---

## 核心功能

### 正弦波生成与 SPI 发送

`hal_entry.c` 中实现了一个 1 Hz 正弦波生成器：

```c
#define ANGLE_STEP   10        // 步进角度
#define SINE_SCALE   127       // 幅值缩放 (0~127 → 映射到 0~255)
#define SINE_OFFSET  128       // 直流偏置

uint8_t y = (uint8_t)(sin(angle * M_PI / 180.0) * SINE_SCALE + SINE_OFFSET);
```

生成数据通过以下协议帧发送：

| 字节 | 字段 | 值 | 说明 |
|---|---|---|---|
| 0 | SOF | 0xA5 | 帧起始标识 |
| 1 | CMD | 0x02 | DATA_WRITE 命令 |
| 2 | LEN | 0x02 | 有效数据长度 |
| 3 | X | angle | 角度索引 |
| 4 | Y | sin 值 | 正弦计算结果 (0~255) |
| 5 | CRC | XOR | 校验和 (Byte0 ^ Byte1 ^ ... ^ Byte4) |

---

## 通信协议

详细的 SPI 通信协议规范见 `docs/SPI_Protocol.md`，包含：

- 物理层定义 (Mode 0, 7.8 MHz)
- 数据帧格式 (SOF + CMD + LEN + PAYLOAD + CRC)
- 命令集 (HEARTBEAT=0x01, DATA_WRITE=0x02, DATA_READ=0x03, STATUS=0x04)
- CRC 算法 (逐字节 XOR)
- 从机处理伪代码
- 主机实现参考

---

## 开发环境搭建

### 必要软件

| 软件 | 版本 | 用途 |
|---|---|---|
| **e2 studio** | 2025+ | 集成开发环境 |
| **FSP** | v6.5.1 | 灵活软件包 |
| **GCC ARM Embedded** | 13.2.1 (arm-13-7) | ARM 交叉编译工具链 |
| **SEGGER J-Link** | 最新版 | 调试与烧录 |
| **串口终端** | 任意 | UART 调试日志查看 |

### 导入工程

1. 打开 e2 studio
2. `File` → `Import` → `Existing Projects into Workspace`
3. 选择工程根目录 `LifeDream/`
4. 确保 FSP v6.5.1 已安装并关联

### 克隆仓库

```bash
git clone git@github.com:Xuezhi-OUC/LifeDream.git
cd LifeDream
```

---

## 构建与烧录

### 在 e2 studio 中构建

1. 右键工程 → `Build Project` (或 `Ctrl+B`)
2. 构建产物位于 `Debug/` 目录

### 命令行构建

```bash
cd Debug
make -j$(nproc)
```

### 烧录调试

1. 通过 USB 连接 RA8P1 EK 板到 PC
2. 在 e2 studio 中选择 `Debug Configurations` → `LifeDream Debug_Flat`
3. 点击 `Debug` 开始调试
4. J-Link 将通过 SWD 接口烧录并调试

### UART 调试输出

- 波特率: 115200
- 数据位: 8
- 校验: 无
- 停止位: 1

使用串口终端（如 Putty、screen、串口助手）连接到板载虚拟 COM 端口即可查看调试信息。

---

## 调试说明

### SPI 调试要点

- SPI CS 使用 P6.04 GPIO 软件控制，需在发送前后手动拉低/拉高
- SPI 发送使用 `g_sci_spi0.p_api->writeRead()` 同步接口
- 默认 SPI 超时设置为 1000 ms
- P6.00 因默认功能冲突已废弃作为 CS 使用

### 已知问题

1. **P6.00 引脚复用冲突** — 默认配置为 SCI 外设功能，导致无法作为 GPIO CS。已通过切换至 P6.04 解决。
2. **OV2640 驱动未集成** — FSP 配置中已启用 CEU 和 I2C，但摄像头控制代码尚未移植到 `src/` 中。

### LED 指示

| LED | 引脚 | 行为 |
|---|---|---|
| 蓝色 LED | P1.10 | SPI 发送数据时闪烁 50 ms |

---

## 参考资料

- [Renesas RA8P1 数据手册](https://www.renesas.com/products/microcontrollers-microprocessors/ra-cortex-m-mcus/ra8p1-1000mhz-cortex-m85-mcu)
- [RA8P1 EK 用户手册](https://www.renesas.com/products/microcontrollers-microprocessors/ra-cortex-m-mcus/ra8p1-ek)
- [FSP 文档 v6.5.1](https://github.com/renesas/rx-fsp)
- [ARM Cortex-M85 技术参考手册](https://developer.arm.com/documentation/102254)
- [CMSIS v6 文档](https://arm-software.github.io/CMSIS_6/latest/)
- [OV2640 数据手册](https://www.ovt.com/products/ov2640/)

---

> **人生有梦，各自精彩** — LifeDream Team © 2026