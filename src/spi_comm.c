/*
 * SPI Communication Wrapper for ESP32 Slave
 *
 * 封装 FSP SCI_B_SPI Master 驱动 (g_sci_spi0)，
 * 提供基于 SPI_Protocol.md v1.0 的协议帧收发接口。
 * CS 由 GPIO (P6.04) 手动控制，使用 writeRead 全双工模式。
 */

#include "spi_comm.h"
#include <string.h>

/* CS 片选引脚 */
#define SPI_CS_PORT       BSP_IO_PORT_06_PIN_04

/* 传输时序参数 (ms) */
#define SPI_FRAME_DELAY_MS     1   /* 传输完成后等待 CS 拉高 */
#define SPI_POST_CS_DELAY_MS   1   /* CS 拉高后等待 Slave 处理 */


/* ---- 内部辅助 ---- */

static void cs_select(void)
{
    R_IOPORT_PinWrite(&g_ioport_ctrl, SPI_CS_PORT, BSP_IO_LEVEL_LOW);
}

static void cs_deselect(void)
{
    R_IOPORT_PinWrite(&g_ioport_ctrl, SPI_CS_PORT, BSP_IO_LEVEL_HIGH);
}


/* ---- 公有接口 ---- */

spi_comm_status_t spi_comm_init(void)
{
    fsp_err_t err = g_sci_spi0.p_api->open(g_sci_spi0.p_ctrl, g_sci_spi0.p_cfg);
    return (FSP_SUCCESS == err) ? SPI_COMM_SUCCESS : SPI_COMM_ERR_INIT;
}

spi_comm_status_t spi_comm_send_frame(uint8_t cmd, const uint8_t *payload, uint32_t len)
{
    uint32_t frame_len;
    uint8_t frame[SPI_MAX_PAYLOAD + SPI_FRAME_OVERHEAD];
    uint8_t rx_buf[SPI_MAX_PAYLOAD + SPI_FRAME_OVERHEAD] = {0};
    fsp_err_t err;

    if (len > SPI_MAX_PAYLOAD)
    {
        return SPI_COMM_ERR_PARAM;
    }

    if (len > 0 && NULL == payload)
    {
        return SPI_COMM_ERR_NULL_PTR;
    }

    /* 构建协议帧 */
    frame_len = SPI_FRAME_OVERHEAD + len;   /* SOF + CMD + LEN + PAYLOAD + CRC */
    frame[0] = SPI_SOF_BYTE;
    frame[1] = cmd;
    frame[2] = (uint8_t)len;

    if (payload && len > 0)
    {
        memcpy(&frame[3], payload, len);
    }

    /* CRC = XOR(SOF, CMD, LEN, payload[0..len-1]) */
    frame[frame_len - 1] = spi_comm_calc_crc(frame, frame_len - 1);

    /* CS 拉低，通知 Slave 开始传输 */
    cs_select();

    /* 全双工发送 (writeRead 确保 SCK 一直输出) */
    err = g_sci_spi0.p_api->writeRead(g_sci_spi0.p_ctrl,
                                       frame, rx_buf,
                                       frame_len,
                                       SPI_BIT_WIDTH_8_BITS);
    if (FSP_SUCCESS != err)
    {
        cs_deselect();
        return SPI_COMM_ERR_BUSY;
    }

    /* 等待传输完成 (保险延时) */
    R_BSP_SoftwareDelay(SPI_FRAME_DELAY_MS, BSP_DELAY_UNITS_MILLISECONDS);

    /* CS 拉高，通知 Slave 帧结束 */
    cs_deselect();

    /* 等待 Slave 完成处理 */
    R_BSP_SoftwareDelay(SPI_POST_CS_DELAY_MS, BSP_DELAY_UNITS_MILLISECONDS);

    return SPI_COMM_SUCCESS;
}

spi_comm_status_t spi_comm_transfer(const uint8_t *tx_data, uint8_t *rx_data, uint32_t len)
{
    if (NULL == tx_data || NULL == rx_data || 0 == len)
    {
        return SPI_COMM_ERR_NULL_PTR;
    }

    fsp_err_t err = g_sci_spi0.p_api->writeRead(g_sci_spi0.p_ctrl,
                                                  (uint8_t *)tx_data, rx_data,
                                                  len,
                                                  SPI_BIT_WIDTH_8_BITS);
    return (FSP_SUCCESS == err) ? SPI_COMM_SUCCESS : SPI_COMM_ERR_BUSY;
}

uint8_t spi_comm_calc_crc(const uint8_t *data, uint32_t len)
{
    uint8_t crc = 0;
    for (uint32_t i = 0; i < len; i++)
    {
        crc ^= data[i];
    }
    return crc;
}

void spi_comm_deinit(void)
{
    g_sci_spi0.p_api->close(g_sci_spi0.p_ctrl);
}
