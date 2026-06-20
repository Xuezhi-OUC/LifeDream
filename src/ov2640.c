#include "ov2640.h"
#include "ov2640_settings.h"  // 引入庞大的配置数组
// =========================================================================
// 1. 定义全局标志位，用于在中断和主程序间传递状态
// =========================================================================
volatile bool g_i2c_tx_complete = false;
volatile bool g_i2c_rx_complete = false;

// =========================================================================
// 2. IIC 底层中断回调函数 (名字必须与 FSP Properties 里填写的 Callback 名字一致)
// =========================================================================
void iic_callback(i2c_master_callback_args_t * p_args)
{
    if (I2C_MASTER_EVENT_TX_COMPLETE == p_args->event)
    {
        g_i2c_tx_complete = true;
    }
    else if (I2C_MASTER_EVENT_RX_COMPLETE == p_args->event)
    {
        g_i2c_rx_complete = true;
    }
    else if (I2C_MASTER_EVENT_ABORTED == p_args->event)
    {
        /* I2C error — set both flags to prevent infinite hang in polling loops */
        g_i2c_tx_complete = true;
        g_i2c_rx_complete = true;
    }
}

// =========================================================================
// 3. 基础通信函数封装
// =========================================================================

// 单字节写入 (向指定寄存器写入1个字节数据)
fsp_err_t ov2640_write_reg(uint8_t reg_addr, uint8_t data)
{
    fsp_err_t err;
    uint8_t tx_buffer[2] = {reg_addr, data};

    g_i2c_tx_complete = false;
    err = R_IIC_MASTER_Write(&g_i2c_master0_ctrl, tx_buffer, 2, false);
    if (FSP_SUCCESS != err) return err;

    while (!g_i2c_tx_complete) { }

    return FSP_SUCCESS;
}

// 单字节读取 (从指定寄存器读取1个字节数据)
 fsp_err_t ov2640_read_reg(uint8_t reg_addr, uint8_t * p_data)
{
    fsp_err_t err;

    // 第一步：写入想要读取的寄存器地址
    g_i2c_tx_complete = false;
    err = R_IIC_MASTER_Write(&g_i2c_master0_ctrl, &reg_addr, 1, false);
    if (FSP_SUCCESS != err) return err;

    while (!g_i2c_tx_complete) { }

    // 稍微延时，给传感器内部逻辑一点准备时间
    R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MICROSECONDS);

    // 第二步：读取数据
    g_i2c_rx_complete = false;
    err = R_IIC_MASTER_Read(&g_i2c_master0_ctrl, p_data, 1, false);
    if (FSP_SUCCESS != err) return err;

    while (!g_i2c_rx_complete) { }

    return FSP_SUCCESS;
}

// =========================================================================
// 4. 点亮测试函数
// =========================================================================
void test_ov2640_connection(void)
{
    uint8_t pid_high = 0;
    uint8_t pid_low  = 0;
    fsp_err_t err;

    // 0. 硬件引脚初始化：确保 OV2640 退出复位和掉电模式
    /* P1.09 = OV_PWDN, 输出 LOW → 正常工作 (HIGH = 掉电) */
    R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_01_PIN_09, BSP_IO_LEVEL_LOW);
    /* P7.09 = OV_RESET, 先拉低进入复位 */
    R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_07_PIN_09, BSP_IO_LEVEL_LOW);
    R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
    /* 再拉高释放复位 */
    R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_07_PIN_09, BSP_IO_LEVEL_HIGH);
    R_BSP_SoftwareDelay(20, BSP_DELAY_UNITS_MILLISECONDS);

    // 1. 初始化并打开 IIC 硬件模块
    err = R_IIC_MASTER_Open(&g_i2c_master0_ctrl, &g_i2c_master0_cfg);
    if (FSP_SUCCESS != err) {
        while(1);
    }

    // 2. 切换到 Sensor Bank (OV2640 必须步骤：向 0xFF 寄存器写入 0x01)
    ov2640_write_reg(0xFF, 0x01);

    R_BSP_SoftwareDelay(5, BSP_DELAY_UNITS_MILLISECONDS);

    // 3. 读取 PID 寄存器
    ov2640_read_reg(0x0A, &pid_high);
    ov2640_read_reg(0x0B, &pid_low);

    // 4. 验证结果
    if (pid_high == 0x26)
    {
        // 测试成功！可以在这行代码打个断点
        __asm("nop");
    }
    else
    {
        // 测试失败！读回来的值不对，也在这行打个断点
        __asm("nop");
    }
}
// =========================================================================
// 4.5 OLED 连接测试
// =========================================================================
void test_oled_connection(void)
{
    fsp_err_t err;
    uint8_t test_data[2] = {0x00, 0x00};

    err = R_IIC_MASTER_Open(&g_i2c_master0_ctrl, &g_i2c_master0_cfg);
    if (FSP_SUCCESS != err) { while(1); }

    g_i2c_tx_complete = false;
    err = R_IIC_MASTER_Write(&g_i2c_master0_ctrl, test_data, 2, false);

    if (FSP_SUCCESS == err) {
        while (!g_i2c_tx_complete) { }
        __asm("nop");
    }
    else {
        __asm("nop");
    }
}

// =========================================================================
// 5. 数组连续写入引擎
// =========================================================================
fsp_err_t ov2640_write_array(const uint8_t reg_array[][2])
{
    uint32_t i = 0;
    fsp_err_t err;

    // 遍历数组，直到遇到结尾标志 {0, 0}
    while ((reg_array[i][0] != 0) || (reg_array[i][1] != 0))
    {
        g_i2c_tx_complete = false;

        // 每次发送 2 个字节：[0]是地址，[1]是数据
        err = R_IIC_MASTER_Write(&g_i2c_master0_ctrl, (uint8_t *)reg_array[i], 2, false);
        if (FSP_SUCCESS != err) return err;

        // 死等底层的硬件发送完成中断
        while (!g_i2c_tx_complete) { }

        // 每写完一个寄存器，延时 1 毫秒，防止 I2C 塞车或传感器处理不过来
        R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);

        i++;
    }

    return FSP_SUCCESS;
}

// =========================================================================
// 6. 完整的 OV2640 初始化流程
// =========================================================================
fsp_err_t ov2640_init(void)
{
    fsp_err_t err;

    // 1. 发送基础初始化参数 (时钟、基础增益、白平衡等)
    err = ov2640_write_array(ov2640_settings_cif);
    if (FSP_SUCCESS != err) return err;

    // 2. 发送分辨率配置 (这里以 CIF 分辨率 352x288 为例，占用 RAM 较小，适合调试)
    err = ov2640_write_array(ov2640_settings_to_cif);
    if (FSP_SUCCESS != err) return err;

    // 3. 发送色彩格式配置 (配置为 RGB565 格式，这是最常用且瑞萨 CEU 支持最好的格式)
    err = ov2640_write_array(ov2640_settings_rgb565);
    if (FSP_SUCCESS != err) return err;

    return FSP_SUCCESS;
}
