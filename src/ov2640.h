#ifndef OV2640_H_
#define OV2640_H_

#include "hal_data.h"

// 内部基础操作函数声明 (消除警告)
fsp_err_t ov2640_write_reg(uint8_t reg_addr, uint8_t data);
fsp_err_t ov2640_read_reg(uint8_t reg_addr, uint8_t * p_data);
fsp_err_t ov2640_write_array(const uint8_t reg_array[][2]);

// 外部调用接口声明
void test_ov2640_connection(void);
void test_oled_connection(void);
fsp_err_t ov2640_init(void);

#endif /* OV2640_H_ */