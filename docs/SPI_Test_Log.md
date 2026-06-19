# SPI 通信调试日志

## 测试时间

2026-06-19

## 硬件连接

| Master (RA8P1) | Slave | 说明 |
|----------------|-------|------|
| SCK (P601) | SCK | SPI 时钟 7.8125 MHz |
| MOSI (P602) | MOSI | Master 输出，Slave 输入 |
| MISO (P603) | MISO | Slave 输出，Master 输入 |
| CS (P6.04) | CS | 片选信号，低有效，GPIO 手动控制 |

## 配置参数

| 参数 | 值 |
|------|-----|
| SPI 模式 | Mode 0 (CPOL=0, CPHA=0) |
| 位序 | MSB First |
| 数据宽度 | 8-bit |
| 时钟频率 | 7.8125 MHz |
| CS 引脚 | P6.04（原 P6.00 为板载蓝色 LED，已换到 P6.04） |

## 修复记录

### 问题

Slave 的 CS 接 P604 无法正常通信，必须手动断开再连接才能短暂工作。

### 根因

P604 默认配置为 **SCI0 外设功能模式**（`IOPORT_PERIPHERAL_SCI0_2_4_6_8`），而非 GPIO 模式。代码中 `R_IOPORT_PinWrite()` 无法控制外设模式下的引脚电平，导致 Slave 接收不到有效的 CS 信号。

### 修改

1. **`ra_gen\pin_data.c`** — P604 从外设模式改为 GPIO 输出，低电平初始，高驱动强度：
   ```c
   { .pin = BSP_IO_PORT_06_PIN_04, .pin_cfg = ((uint32_t) IOPORT_CFG_DRIVE_HIGH
             | (uint32_t) IOPORT_CFG_PORT_DIRECTION_OUTPUT | (uint32_t) IOPORT_CFG_PORT_OUTPUT_LOW) }, /* CS_SCI0_SPI0 */
   ```

2. **`src\hal_entry.c`** — CS 操作从 `BSP_IO_PORT_06_PIN_00` 改为 `BSP_IO_PORT_06_PIN_04`

### 测试结果

- 通信正常，Slave 可正确接收数据帧
- 帧格式：`SOF(0xA5) + CMD(0x02) + LEN(0x03) + PAYLOAD(0xAA,0xBB,0xCC) + CRC`
- 发送频率：1 Hz
- UART9 调试输出正常

## 注意事项

- `pin_data.c` 由 FSP 自动生成，重新生成配置会覆盖修改，需在 FSP Configurator 中同步修改 P604 配置
- P6.00 目前仍配置为 GPIO 输出低电平（蓝色 LED），如需恢复 LED 功能需重新配置