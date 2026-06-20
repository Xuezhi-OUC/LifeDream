# OV2640 摄像头硬件连接

## 平台

- **开发板**: RA8P1-EK (R7KA8P1KFLCAC / Cortex-M85)
- **主频**: 1000 MHz
- **XCLK 晶振**: 24 MHz

---

## 引脚连接

### I2C 控制 (SCCB) — g_i2c_master0 (IIC0)

| MCU 引脚 | 功能 | OV2640 | 备注 |
|----------|------|--------|------|
| P4.09 | SCL | SCL | I2C 时钟，需 4.7kΩ 上拉到 3.3V |
| P4.10 | SDA | SDA | I2C 数据，需 4.7kΩ 上拉到 3.3V |

- 7-bit 地址: `0x30` (ADCCK = LOW)
- 速率: ~98 kHz

### DVP 数据 — g_ceu0 (CEU)

| MCU 引脚 | CEU 信号 | OV2640 |
|----------|----------|--------|
| P4.00 | CEU_D0 | D0 |
| P4.01 | CEU_D1 | D1 |
| P4.05 | CEU_D2 | D2 |
| P4.06 | CEU_D3 | D3 |
| P4.14 | CEU_D4 | D4 |
| P4.15 | CEU_D5 | D5 |
| P7.00 | CEU_D6 | D6 |
| P7.01 | CEU_D7 | D7 |
| P7.02 | CEU_VSYNC | VSYNC |
| P7.03 | CEU_HSYNC | HREF / HSYNC |
| P7.08 | CEU_PCLK | PCLK |

### 控制引脚

| MCU 引脚 | 信号 | OV2640 | 当前电平 | 要求 |
|----------|------|--------|----------|------|
| 不需要，有晶振了 | / | / | — | **已经提供 6~27MHz 时钟** (推荐 24MHz)，否则 SCCB 不响应 |
| P1.09 | OV_PWDN | OV_PWDN | 输出 LOW | LOW = 正常工作 / HIGH = 掉电 |
| P7.09 | OV_RESET | OV_RESET | 输出 LOW | LOW = 复位 / HIGH = 正常 (需先脉冲复位再拉高) |

---

## 上电时序要求

```
1. 3.3V 供电
2. XCLK 时钟输入（自带晶振）
3. OV_PWDN = LOW
4. OV_RESET 脉冲 LOW→HIGH
5. 等待 ≥10ms 稳定
6. SCCB (I2C) 开始读写
```

## OV2640 默认 SCCB 地址

```c
#define OV2640_ADDR     0x30    // 7-bit 地址
// 8-bit 写地址: 0x60, 8-bit 读地址: 0x61
```

## 传感器 ID

| 寄存器 | 地址 | 期望值 |
|--------|------|--------|
| REG_PID | 0x0A | 0x26 |
| REG_VER | 0x0B | 0x42 |