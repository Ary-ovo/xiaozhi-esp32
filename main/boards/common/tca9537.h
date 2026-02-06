#ifndef TCA9537_H
#define TCA9537_H

#include "i2c_device.h"

// 寄存器定义
#define TCA9537_REG_INPUT       0x00
#define TCA9537_REG_OUTPUT      0x01
#define TCA9537_REG_POLARITY    0x02
#define TCA9537_REG_CONFIG      0x03

// 根据你的原理图定义的掩码
// P1: Speaker, P2: EPD RST, P3: EPD CS
#define MASK_SPEAKER    (1 << 1)
#define MASK_EPD_RST    (1 << 2)
#define MASK_EPD_CS     (1 << 3)

class Tca9537 : public I2cDevice {
public:
    // 构造函数
    Tca9537(i2c_master_bus_handle_t i2c_bus, uint8_t addr);

    // 直接写输出寄存器 (0x01)
    void SetOutputPort(uint8_t value);
    
    // 直接写配置寄存器 (0x03)
    void SetConfiguration(uint8_t dir_mask);

    // 获取当前输出状态
    uint8_t GetOutputPort();

    // 辅助功能：单独设置某一位
    void SetPinLevel(uint8_t pin_mask, bool level);
};

#endif // TCA9537_H