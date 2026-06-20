#include "hal_data.h"
#include "spi_comm.h"
#include <stdio.h>
#include "ov2640.h"

/* SPI callback - required by SCI0 SPI configuration */
void sci_b_spi_callback(spi_callback_args_t *p_args)
{
    FSP_PARAMETER_NOT_USED(p_args);
}

void hal_entry(void)
{
    fsp_err_t err;

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

    /* IIC 测试 OV2640 连接 + 复位时序 */
    test_ov2640_connection();

    /* 读取 PID 并通过 SPI 发送结果给 ESP32 */
    uint8_t pid_high = 0, pid_low = 0;
    ov2640_read_reg(0x0A, &pid_high);
    ov2640_read_reg(0x0B, &pid_low);
    uint8_t result = (pid_high == 0x26) ? 1 : 0;
    uint8_t payload[4] = { pid_high, pid_low, result, 0 };
    spi_comm_send_frame(SPI_CMD_DATA_WRITE, payload, sizeof(payload));

    /* LED 心跳 */
    while (1)
    {
        R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_01_PIN_10, BSP_IO_LEVEL_HIGH);
        R_BSP_SoftwareDelay(50, BSP_DELAY_UNITS_MILLISECONDS);
        R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_01_PIN_10, BSP_IO_LEVEL_LOW);
        R_BSP_SoftwareDelay(950, BSP_DELAY_UNITS_MILLISECONDS);
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
