#include "hal_data.h"
#include "spi_comm.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

/* SPI callback - required by SCI0 SPI configuration */
void sci_b_spi_callback(spi_callback_args_t *p_args)
{
    FSP_PARAMETER_NOT_USED(p_args);
}

static void uart_send(const char *str)
{
    g_uart9.p_api->write(g_uart9.p_ctrl, (uint8_t *)str, strlen(str));
}

void hal_entry(void)
{
    fsp_err_t err;
    char uart_msg[64];
    uint32_t loop_count = 0;
    uint8_t angle = 0;
    const float DEG2RAD = 3.14159265f / 180.0f;

    /* Open UART9 (115200, 8N1) */
    err = g_uart9.p_api->open(g_uart9.p_ctrl, g_uart9.p_cfg);
    if (FSP_SUCCESS != err) { while (1); }

    /* Initialize SPI (opens g_sci_spi0 as Master) */
    if (SPI_COMM_SUCCESS != spi_comm_init()) { while (1); }

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
        /* 构造正弦数据: x = angle, y = sin(x) 映射到 [0..255] */
        float rad = angle * DEG2RAD;
        uint8_t y = (uint8_t)((sinf(rad) + 1.0f) * 127.5f);

        /* P1.10 LED fast flash ON */
        R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_01_PIN_10, BSP_IO_LEVEL_HIGH);
        R_BSP_SoftwareDelay(50, BSP_DELAY_UNITS_MILLISECONDS);
        /* P1.10 LED fast flash OFF */
        R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_01_PIN_10, BSP_IO_LEVEL_LOW);

        /* 通过 SPI 发送正弦数据帧 (自动处理 CS 时序 + CRC) */
        uint8_t payload[2] = { angle, y };
        spi_comm_send_frame(SPI_CMD_DATA_WRITE, payload, sizeof(payload));

        /* UART9 Print sine data */
        sprintf(uart_msg, "SPI Sine[%lu]: x=%3u y=%3u (sin=%+.2f)\r\n",
                (unsigned long) loop_count++,
                (unsigned int) angle, (unsigned int) y, (double) sinf(rad));
        uart_send(uart_msg);

        angle += 10;  /* 步进 10°, 256 次循环后归零 */

        /* Wait for 1Hz cycle (50ms flash + ~2ms SPI + 948ms = 1000ms) */
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