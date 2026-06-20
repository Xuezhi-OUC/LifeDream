/*
 * SPI Communication Wrapper for ESP32 Slave
 *
 * 基于 FSP SCI_B_SPI Master (g_sci_spi0) 封装，
 * 提供同步阻塞式的 SPI 协议帧收发接口。
 * 遵循 docs/SPI_Protocol.md 定义的帧格式。
 */

#ifndef SPI_COMM_H_
#define SPI_COMM_H_

#include <stdint.h>
#include "hal_data.h"

/* SPI 协议常量 */
#define SPI_SOF_BYTE           0xA5
#define SPI_CMD_HEARTBEAT      0x01
#define SPI_CMD_DATA_WRITE     0x02
#define SPI_CMD_DATA_READ      0x03
#define SPI_CMD_STATUS         0x04

#define SPI_MAX_PAYLOAD        255
#define SPI_FRAME_OVERHEAD     4   /* SOF + CMD + LEN + CRC */

/** SPI 通信状态 */
typedef enum
{
    SPI_COMM_SUCCESS = 0,
    SPI_COMM_ERR_INIT,       /* 外设打开失败 */
    SPI_COMM_ERR_NULL_PTR,   /* 空指针或参数非法 */
    SPI_COMM_ERR_BUSY,       /* SPI 传输错误 */
    SPI_COMM_ERR_PARAM,      /* 参数超出范围 */
} spi_comm_status_t;

/**
 * @brief 初始化 SPI 外设 (打开 g_sci_spi0)
 * @return SPI_COMM_SUCCESS 成功，否则失败
 */
spi_comm_status_t spi_comm_init(void);

/**
 * @brief 发送完整协议帧 (SOF + CMD + LEN + PAYLOAD + CRC)
 *        自动处理 CS 片选时序（拉低 -> 传输 -> 等待 -> 拉高）
 * @param cmd      命令字 (如 SPI_CMD_DATA_WRITE)
 * @param payload  数据载荷，len=0 时可为 NULL
 * @param len      载荷字节数 (0 ~ SPI_MAX_PAYLOAD)
 * @return 通信状态
 */
spi_comm_status_t spi_comm_send_frame(uint8_t cmd, const uint8_t *payload, uint32_t len);

/**
 * @brief 原始全双工传输 (低层级)
 *        不处理 CS 片选，由调用者控制
 * @param tx_data  发送缓冲区
 * @param rx_data  接收缓冲区
 * @param len      传输字节数
 * @return 通信状态
 */
spi_comm_status_t spi_comm_transfer(const uint8_t *tx_data, uint8_t *rx_data, uint32_t len);

/**
 * @brief 计算 CRC (XOR 累加校验)
 * @param data  数据缓冲区
 * @param len   字节数
 * @return CRC 值
 */
uint8_t spi_comm_calc_crc(const uint8_t *data, uint32_t len);

/**
 * @brief 关闭 SPI 外设
 */
void spi_comm_deinit(void);

#endif /* SPI_COMM_H_ */
