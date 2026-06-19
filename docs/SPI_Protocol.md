# SPI 主从通信协议规范 v1.0

## 1. 物理层

| 参数 | 值 |
|------|-----|
| SPI 模式 | **Mode 0** (CPOL=0, CPHA=0) |
| 位序 | MSB First |
| 数据宽度 | 8-bit |
| 时钟频率 | **~7.8 MHz** (Master: RA8P1) |
| CS 片选 | **低电平有效**，由 Master GPIO 控制 |

### 引脚连接

| Master (RA8P1) | Slave | 说明 |
|----------------|-------|------|
| SCK (P601) | SCK | SPI 时钟 |
| MOSI (P602) | MOSI | Master 输出，Slave 输入 |
| MISO (P603) | MISO | Slave 输出，Master 输入 |
| CS (P6.00) | CS | 片选信号，低有效 |

> CS 引脚可按需调整，当前使用 **P6.00** (已配置为 GPIO 输出 LOW)。

---

## 2. 数据帧格式

每次通信以一帧为单位，帧结构如下：

```
+--------+--------+--------+--------------------+--------+
|  SOF   |  CMD   |  LEN   |      PAYLOAD      |  CRC   |
|  1 B   |  1 B   |  1 B   |    0 ~ 255 B      |  1 B   |
+--------+--------+--------+--------------------+--------+
```

### 字段说明

| 字段 | 长度 | 说明 |
|------|------|------|
| **SOF** | 1 B | 帧起始标志，固定 `0xA5` |
| **CMD** | 1 B | 命令字（见第 3 节） |
| **LEN** | 1 B | PAYLOAD 字节数，范围 `0x00` ~ `0xFF` (0~255) |
| **PAYLOAD** | 0~255 B | 数据载荷 |
| **CRC** | 1 B | 校验和（见第 5 节） |

### 传输时序（CS 控制）

```
         +---+
CS ------+   +---------------------------------------------
         |   |
         +---+---++---++---++---++---++---++---++---+---
SCK ----+    +--++--++--++--++--++--++--++--++--++--+--...
         |  B0  | B1 | B2 | B3 | B4 | B5 | ...| BN | CRC
MOSI ----+-----+----+----+----+----+----+----+----+----...
         | SOF |CMD |LEN |          PAYLOAD          |CRC
MISO ----+.....+....+....+...........................+....
```

1. Master 拉低 **CS**，启动传输
2. Master 连续发送 `N+4` 个字节（SOF + CMD + LEN + PAYLOAD + CRC）
3. 每发送 1 字节，同时从 MISO 接收 1 字节（全双工）
4. Master 拉高 **CS**，结束传输

> Slave 在 MISO 上返回的数据：非 PAYLOAD 阶段返回 `0x00`；若 CMD 为读取命令，在对应 PAYLOAD 位置返回数据。

---

## 3. 命令定义

| CMD | 名称 | 方向 | PAYLOAD | 说明 |
|-----|------|------|---------|------|
| `0x01` | HEARTBEAT | Master → Slave | 无 (LEN=0) | 心跳探测，Slave 应回复状态 |
| `0x02` | DATA_WRITE | Master → Slave | 任意数据 | Master 发送数据给 Slave |
| `0x03` | DATA_READ | Master → Slave | 请求字节数 `[N]` | 请求 Slave 返回 N 字节数据 |
| `0x04` | STATUS | 双向 | 状态码 (1 B) | 状态查询 / 应答 |

### 3.1 HEARTBEAT (0x01)

```
Master → Slave:
  SOF=0xA5, CMD=0x01, LEN=0x00, CRC=0xA4
```

Slave 正常时应正确接收并校验 CRC。Master 可通过 CRC 是否正确来判断 Slave 是否在线。

### 3.2 DATA_WRITE (0x02)

示例：Master 发送 5 字节数据 `0x10, 0x20, 0x30, 0x40, 0x50`

```
  SOF=0xA5, CMD=0x02, LEN=0x05,
  PAYLOAD=0x10, 0x20, 0x30, 0x40, 0x50,
  CRC=0xA5^0x02^0x05^0x10^0x20^0x30^0x40^0x50 = 0xFA
```

### 3.3 DATA_READ (0x03)

Master 请求读取 N 字节。PAYLOAD 为单字节，表示请求的字节数。

示例：请求 8 字节数据

```
  SOF=0xA5, CMD=0x03, LEN=0x01,
  PAYLOAD=0x08,
  CRC=0xA5^0x03^0x01^0x08 = 0xA5
```

Slave 应在接收到 CMD 为 0x03 后，在后续 PAYLOAD 位置填充响应数据。

### 3.4 STATUS (0x04)

状态查询或主动上报。

| 状态码 | 含义 |
|--------|------|
| `0x00` | 空闲 / OK |
| `0x01` | 忙碌 |
| `0x02` | 错误 |
| `0x03` | 缓冲区满 |
| `0xFF` | 未就绪 |

示例：Master 查询状态

```
Master → Slave:
  SOF=0xA5, CMD=0x04, LEN=0x01, PAYLOAD=0x00, CRC=0xA0
```

---

## 4. Slave 应答规范

Slave 在全双工模式下**必须**在 MISO 上始终保持输出，不能出现高阻态。建议 Slave 的行为：

```
+------------------+----------------------------------+
| Master 发送阶段  | Slave MISO 输出                  |
+------------------+----------------------------------+
| SOF 字节         | 0x00                             |
| CMD 字节         | 0x00 (或上一个响应的最后字节)     |
| LEN 字节         | 0x00                             |
| PAYLOAD 字节     | 0x00 (写命令)；响应数据 (读命令)  |
| CRC 字节         | 0x00                             |
+------------------+----------------------------------+
```

对于 **DATA_READ (0x03)**：Slave 应在收到完整的 CMD + LEN 后，在后续的 PAYLOAD 时钟周期内逐个字节填充响应数据。Master 发送的 PAYLOAD 内容为请求长度，Slave 可忽略其值，仅按 `LEN` 字段决定返回字节数。

---

## 5. CRC 校验算法

使用 **XOR 累加校验**，算法如下：

```c
uint8_t spi_calc_crc(const uint8_t *data, uint32_t len)
{
    uint8_t crc = 0;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= data[i];
    }
    return crc;
}
```

CRC 范围：`SOF + CMD + LEN + PAYLOAD` 所有字节，不包含 CRC 自身。

---

## 6. Slave 处理流程伪代码

```
SPI 中断接收 / DMA 接收:

1. 等待 CS 拉低
2. 接收第 1 字节 → 检查是否为 SOF (0xA5)
   - 不是 0xA5 → 丢弃，等待下一个字节
   - 是 0xA5 → 继续
3. 接收第 2 字节 → 保存为 CMD
4. 接收第 3 字节 → 保存为 LEN
5. 接收 LEN 个字节 → 保存为 PAYLOAD
6. 接收最后 1 字节 → 保存为 CRC_RX
7. 等待 CS 拉高（帧结束）

8. 计算 CRC: CRC_CAL = XOR(SOF, CMD, LEN, PAYLOAD[0..LEN-1])
9. 比较 CRC_CAL == CRC_RX
   - 不匹配 → 丢弃该帧
   - 匹配 → 根据 CMD 执行操作
10. 准备下一次接收
```

### DATA_READ 特殊处理

当收到 `CMD=0x03` 时，Slave 需在当前帧的 PAYLOAD 阶段就返回数据：

```
在接收 SOF, CMD, LEN 后:
  - 确定要返回的长度 N = LEN (或取 PAYLOAD[0])
  - 在后续 N 个字节的 MISO 上依次发送响应数据
```

> 由于 SPI 是全双工，这意味着 Slave 必须在**接收的同时发送**。建议 Slave 使用双缓冲或 DMA 来实现。

---

## 7. Master 当前实现参考

当前 Master (RA8P1) 以 **1Hz** 频率循环执行：

```
loop:
  1. LED (P1.10) 快闪 50ms
  2. CS 拉低 (P6.00 → LOW)              → Slave 准备接收
  3. SPI0 writeRead 发送完整协议帧        → 7 字节, 阻塞等待完成
  4. 等待 1ms (确保传输结束, 3B@7.8MHz=3μs)
  5. CS 拉高 (P6.00 → HIGH)              → 通知 Slave 帧结束
  6. 等待 1ms (Slave 处理完毕)
  7. UART9 打印帧信息
  8. 等待 948ms
```

> **注意**：RA8P1 的 SCI SPI 外设在主模式下**不会自动生成 CS**，必须由 GPIO 手动控制。每帧传输前后都必须拉低/拉高 CS，Slave 依赖 CS 上升沿判断帧结束。
>
> **关键**：必须使用 `writeRead()` 而不是 `write()`。Renesas SCI SPI 驱动的 `write()` 在 `p_dest=NULL` 且无 DMA 时**不会将第一字节写入 TDR**，因此不会产生 SCK 时钟。`writeRead()` 会同时使能收发路径，正确启动传输。

帧内容示例（7 字节完整协议帧）：

```
A5 02 03 AA BB CC 79    →  SOF + DATA_WRITE + LEN=3 + PAYLOAD(AA BB CC) + CRC

各字节:  A5=SOF, 02=CMD_DATA_WRITE, 03=LEN, AA/BB/CC=PAYLOAD, 79=CRC(XOR)
CRC 计算:  A5 ^ 02 ^ 03 ^ AA ^ BB ^ CC = 79
```

---

## 8. 附录：CRC 计算参考表

### 快速验证

```
输入: A5 01 00          → CRC = A5 ^ 01 ^ 00 = A4
输入: A5 02 03 AA BB CC → CRC = A5 ^ 02 ^ 03 ^ AA ^ BB ^ CC = 6B
输入: A5 03 01 08       → CRC = A5 ^ 03 ^ 01 ^ 08 = A5
输入: A5 04 01 00       → CRC = A5 ^ 04 ^ 01 ^ 00 = A0
```