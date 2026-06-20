---
name: ov2640-hw-pins
description: OV2640 硬件引脚要求 — RESET 高电平有效，PWDN 低电平休眠
metadata:
  type: reference
---

- **RESET 引脚**: 必须保持 **高电平 (3.3V)**，否则芯片一直处于复位状态。
- **PWDN 引脚**: 必须保持 **低电平 (GND)**，否则芯片进入断电休眠，I2C 无响应。

如果 PID 读不到 0x26，优先检查这两个引脚的硬件电平。
