#include "hal_data.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

/* SPI callback - required by SCI0 SPI configuration */
void sci_b_spi_callback(spi_callback_args_t *p_args)
{
    FSP_PARAMETER_NOT_USED(p_args);
}

void hal_entry(void)
{
    fsp_err_t err;
    uint8_t uart_msg[64];
    uint8_t spi_tx_data[6];
    uint8_t spi_rx_data[6] = {0};
    uint32_t loop_count = 0;
    uint8_t angle = 0;
    const float DEG2RAD = 3.14159265f / 180.0f;

    /* Open UART9 (115200, 8N1) */
    err = g_uart9.p_api->open(g_uart9.p_ctrl, g_uart9.p_cfg);
    if (FSP_SUCCESS != err) { while (1); }

    /* Open SPI0 (Master, ~7.8MHz, 8-bit) */
    err = g_sci_spi0.p_api->open(g_sci_spi0.p_ctrl, g_sci_spi0.p_cfg);
    if (FSP_SUCCESS != err) { while (1); }

    /* Wake up 2nd core if this is first core and we are inside a multicore project. */
#if (0 == _RA_CORE) && (1 == BSP_MULTICORE_PROJECT) && !BSP_TZ_NONSECURE_BUILD

#if BSP_TZ_SECURE_BUILD
    /* Take semaphore so 2nd core can clear it */
    R_BSP_IpcSemaphoreTake(&g_core_start_semaphore);
#endif

    R_BSP_SecondaryCoreStart();

#if BSP_TZ_SECURE_BUILD
    /* Wait for 2nd core to start and clear semaphore */
    while(FSP_ERR_IN_USE == R_BSP_IpcSemaphoreTake(&g_core_start_semaphore))
    {
        ;
    }
#endif
#endif

#if (1 == _RA_CORE) && (1 == BSP_MULTICORE_PROJECT) && BSP_TZ_SECURE_BUILD
    /* Signal to 1st core that 2nd core has started */
    R_BSP_IpcSemaphoreGive(&g_core_start_semaphore);
#endif

#if BSP_TZ_SECURE_BUILD
    /* Enter non-secure code */
    R_BSP_NonSecureEnter();
#endif

    while (1)
    {
        /* 构造正弦信号帧: SOF + CMD + LEN(2) + x(angle) + y(sin) + CRC */
        float rad = angle * DEG2RAD;
        uint8_t y = (uint8_t)((sinf(rad) + 1.0f) * 127.5f);
        spi_tx_data[0] = 0xA5;                                /* SOF */
        spi_tx_data[1] = 0x02;                                /* CMD_DATA_WRITE */
        spi_tx_data[2] = 0x02;                                /* LEN = 2 */
        spi_tx_data[3] = angle;                               /* x */
        spi_tx_data[4] = y;                                   /* y = sin(x) */
        spi_tx_data[5] = 0xA5 ^ 0x02 ^ 0x02 ^ angle ^ y;     /* CRC */

        /* P1.10 LED fast flash ON */
        R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_01_PIN_10, BSP_IO_LEVEL_HIGH);
        R_BSP_SoftwareDelay(50, BSP_DELAY_UNITS_MILLISECONDS);
        /* P1.10 LED fast flash OFF */
        R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_01_PIN_10, BSP_IO_LEVEL_LOW);

        /* CS 拉低，通知 Slave 开始传输 */
        R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_06_PIN_04, BSP_IO_LEVEL_LOW);

        /* SPI0 发送数据: 使用 writeRead 替代 write */
        /* SCI SPI write() 在 p_dest=NULL 且无 DMA 时不会写 TDR 启动传输 */
        err = g_sci_spi0.p_api->writeRead(g_sci_spi0.p_ctrl, spi_tx_data, spi_rx_data, sizeof(spi_tx_data), SPI_BIT_WIDTH_8_BITS);
        if (FSP_SUCCESS != err) { }

        /* 等待 SPI 传输完成 (6B @ 7.8MHz ≈ 6μs, 保险用 1ms) */
        R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);

        /* CS 拉高，通知 Slave 帧结束 */
        R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_06_PIN_04, BSP_IO_LEVEL_HIGH);

        /* 释放后等一小段让 Slave 完成处理 */
        R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);

        /* UART9 Print sine data */
        sprintf((char*) uart_msg, "SPI Sine[%lu]: x=%3u y=%3u (sin=%+.2f)\r\n",
                (unsigned long) loop_count++,
                (unsigned int) angle, (unsigned int) y, (double) sinf(rad));
        g_uart9.p_api->write(g_uart9.p_ctrl, uart_msg, strlen((char*) uart_msg));

        angle += 10;  /* 步进 10°, 256 次循环后归零 */

        /* Wait for 1Hz cycle (50ms flash + 2ms CS + 948ms = 1000ms) */
        R_BSP_SoftwareDelay(948, BSP_DELAY_UNITS_MILLISECONDS);
    }
}

#if BSP_TZ_SECURE_BUILD

FSP_CPP_HEADER
BSP_CMSE_NONSECURE_ENTRY void template_nonsecure_callable ();

/* Trustzone Secure Projects require at least one nonsecure callable function in order to build (Remove this if it is not required to build). */
BSP_CMSE_NONSECURE_ENTRY void template_nonsecure_callable ()
{

}
FSP_CPP_FOOTER

#endif