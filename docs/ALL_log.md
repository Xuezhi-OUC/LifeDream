# 外设调试日志

## 概述

RA MCU 通过 I2C 初始化 OV2640 摄像头传感器，读取芯片 ID 验证通信正常后，将检测结果通过 SPI 协议帧发送给 ESP32 显示。

## 硬件连接

| 信号 | RA 引脚 | 目标 | 电平要求 |
|------|---------|------|---------|
| OV_PWDN | P1.09 | OV2640 PWDN | LOW = 正常工作，HIGH = 掉电 |
| OV_RESET | P7.09 | OV2640 RESET | 先 LOW 再 HIGH = 脉冲复位 |
| I2C_SCL | IIC0_SCL | OV2640 SCL | 上拉 4.7kΩ |
| I2C_SDA | IIC0_SDA | OV2640 SDA | 上拉 4.7kΩ |
| SPI_SCK | SCI0_SCK | ESP32 SPI Slave | — |
| SPI_MOSI | SCI0_TXD | ESP32 MOSI | — |
| SPI_CS | P6.04 | ESP32 CS | — |
| XCLK | 外部时钟源 | OV2640 XCLK | 6–27MHz 时钟输入 |

## 调试过程

### 阶段 1：初始代码存在致命错误

`src/ov2640.c` 中的 `ov2640_write_reg()` 函数有复制粘贴错误：

```
// ❌ 错误：调用了 R_IIC_MASTER_Read，且使用了未定义的变量 p_data
err = R_IIC_MASTER_Read(&g_i2c_master0_ctrl, p_data, 1, false);

// ✅ 正确：应调用 R_IIC_MASTER_Write，使用 tx_buffer，长度 2
err = R_IIC_MASTER_Write(&g_i2c_master0_ctrl, tx_buffer, 2, false);
```

同时 `ov2640_read_reg()` 中的 `R_IIC_MASTER_Read` 缺少第 4 个 `restart` 参数。

**修复**: 改正函数调用，补充参数。`iic_callback` 增加 `I2C_MASTER_EVENT_ABORTED` 处理防止中断死等。

### 阶段 2：Include 路径错误

`hal_entry.c` 中 `#include "OV2640/ov2640.h"` 指向已删除的旧目录，新文件在 `src/` 根目录。

**修复**: 改为 `#include "ov2640.h"`。

### 阶段 3：硬件复位时序缺失

OV2640 需要外部硬件复位和掉电控制才能正常工作：

```
// PWDN = LOW (退出掉电)
R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_01_PIN_09, BSP_IO_LEVEL_LOW);
// RESET = LOW (进入复位)
R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_07_PIN_09, BSP_IO_LEVEL_LOW);
delay(10ms);
// RESET = HIGH (释放复位)
R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_07_PIN_09, BSP_IO_LEVEL_HIGH);
delay(20ms);
```

不加这步时序前，I2C 读 PID 返回错误值（如 `0x38`）；加上后正确读到 `0x26`。

### 阶段 4：UART 输出中断导致调试困惑

FSP UART `write()` 是非阻塞的，连续多次调用 `uart_send()` 时后一次会覆盖前一次的缓冲区，导致串口输出错乱：

```
// ❌ 输出被截断和混叠
[DBG] re[DBG] re[DBG] read COM3   e=0  va--- I2C diag done ---
```

**解决**: 将所有调试信息拼接到一个大缓冲区中，单次 `uart_send` 发送，并在其后加延时等待传输完成。

### 阶段 5：十进制 vs 十六进制误会

ESP32 端使用 `Serial.printf("%3d", payload[i])` 以**十进制**打印 payload。初始看到 SPI 输出 `38 66 1 0`，被误认为十六进制 `0x38 0x66 0x01 0x00`（怀疑 `0x38 ≠ 0x26`），实际十进制 `38` = 十六进制 `0x26`，通信完全正常。

## 最终验证结果

I2C 读回 OV2640 寄存器：

| 寄存器 | 值 | 含义 |
|--------|-----|------|
| 0x0A (PID_H) | `0x26` | OV2640 产品 ID ✅ |
| 0x0B (PID_L) | `0x42` | 版本号 |
| 0x1C (MIDH) | `0x7F` | 制造商 ID 高字节 |
| 0x1D (MIDL) | `0xA2` | 制造商 ID 低字节 |

## SPI 协议帧格式

MCU → ESP32 发送 4 字节 payload：

| 偏移 | 内容 | 说明 |
|------|------|------|
| 0 | PID_H | OV2640 产品 ID 高字节 |
| 1 | PID_L | 产品 ID 低字节 |
| 2 | result | 1=检测到摄像头, 0=未检测到 |
| 3 | reserved | 保留 (0x00) |

帧封装：`SOF(0xA5) + CMD(0x02) + LEN(0x04) + PAYLOAD(4B) + CRC(1B)`

## 经验教训

1. **I2C 读写函数写完后立即检查参数和函数名**，复制粘贴极易出错
2. **OV2640 必须做硬件复位**（PWDN=0 + RESET 脉冲），否则 I2C 无响应
3. **UART buffer 生命周期**：非阻塞模式下局部变量 buffer 在 write 返回后可能被复用
4. **串口打印统一一次发送**，避免多次调用 write 产生的竞争
5. **跨 MCU 调试注意进制**：发送端和接收端应统一数据表示方式（HEX 或 DEC）
