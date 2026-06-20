# LifeDream — 2026 瑞萨全国大学生电子设计竞赛

**队伍**: 人生有梦队 | **MCU**: Renesas RA8P1 (Cortex-M85 @ 1 GHz) | **IDE**: e2 studio + FSP v6.5.1 | **工具链**: GCC ARM Embedded 13.2.1

---

## 项目概述

基于 RA8P1 的 OV2640 摄像头 I2C 驱动移植 + SPI 主从通信。通过 I2C 复位并配置 OV2640，读取芯片 ID 验证通信后，将检测结果通过 SPI 协议帧发送给 ESP32 从机。

## 里程碑

- [x] SPI 主站通信 (7.8125 MHz, Mode 0)
- [x] OV2640 I2C 驱动初始化 (硬件复位 + PID 读取)
- [x] OV2640 → ESP32 SPI 数据帧传输
- [x] SPI 协议文档 v1.0
- [ ] OV2640 完整寄存器配置（输出图像）
- [ ] CEU 图像采集与处理
- [ ] 上位机通信

## 主循环流程

```
上电 → BSP初始化 → 打开UART/SPI → OV2640硬件复位(PWDN→LOW, RESET→脉冲)
→ I2C读PID(0x26) → 组帧(SOF+CMD+LEN+PAYLOAD+CRC)
→ SPI发送4字节检测结果给ESP32 → LED心跳闪烁(1Hz)
```

## 工程结构

```
src/          ─ 用户代码 (hal_entry, ov2640驱动, SPI通信)
ra_gen/       ─ FSP生成代码 (勿手动编辑)
ra_cfg/       ─ 外设配置头文件
ra/           ─ FSP驱动库 + CMSIS
docs/         ─ 协议文档 & 调试日志
Debug/        ─ 构建输出 (.elf, .srec, .map)
```

## 关键引脚

| 引脚 | 功能 |
|---|---|
| P6.01-04 | SPI (SCK/MOSI/MISO/CS)，CS为P6.04 GPIO软件控制 |
| P1.09/P7.09 | OV2640 PWDN / RESET |
| P9.00-01 | UART9 @ 115200 |

## 外设配置速览

| 外设 | 参数 |
|---|---|
| SPI | 主模式, 7.8125 MHz, CPOL=0, CPHA=0, MSB First |
| UART | 115200-8N1 |
| I2C | 主模式, ~100 kHz, 从机地址 0x30 |
| CEU | 640×480, YCbCr422, 8-bit |

## 已知问题

1. **P6.00 引脚复用冲突** — 默认 SCI 功能无法作 GPIO CS，已切至 P6.04
2. **OV2640 寄存器配置未完成** — I2C 已验证，画面配置尚未下发

## 参考资料

- [RA8P1 数据手册](https://www.renesas.com/products/microcontrollers-microprocessors/ra-cortex-m-mcus/ra8p1-1000mhz-cortex-m85-mcu)
- [FSP v6.5.1](https://github.com/renesas/rx-fsp)
- [OV2640 数据手册](https://www.ovt.com/products/ov2640/)
- [SPI 协议规范](docs/SPI_Protocol.md)

---

> **人生有梦，各自精彩** — LifeDream Team © 2026
