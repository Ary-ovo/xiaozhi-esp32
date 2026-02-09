#ifndef TCA9537_H
#define TCA9537_H

#include "i2c_device.h"

// 寄存器定义
#define TCA9537_REG_INPUT       0x00
#define TCA9537_REG_OUTPUT      0x01
#define TCA9537_REG_POLARITY    0x02
#define TCA9537_REG_CONFIG      0x03

// Pin Mask Code
#define P0    0
#define P1    1
#define P2    2
#define P3    3

#define TCA9537_OUTPUT  0
#define TCA9537_INPUT   1

class Tca9537 : public I2cDevice {
protected:
    uint8_t output_val_ = 0;
    uint8_t config_cache_ = 0;
public:
    // 构造函数
    Tca9537(i2c_master_bus_handle_t i2c_bus, uint8_t addr);

    // 直接写输出寄存器 (0x01)
    void SetOutputPin(uint8_t pin_num, uint8_t level);
    
    // 直接写配置寄存器 (0x03)
    void SetPinConfiguration(uint8_t pin_num, uint8_t direction);

    // 获取Pin当前输出状态
    int GetOutputPin(uint8_t pin_num);

    // 获取Pin当前配置状态
    uint8_t GetPinDirection(uint8_t pin_num);
};

#endif // TCA9537_