#include "tca9537.h"
#include <esp_log.h>

#define TAG "Tca9537"

Tca9537::Tca9537(i2c_master_bus_handle_t i2c_bus, uint8_t addr) : I2cDevice(i2c_bus, addr) {
}

void Tca9537::SetOutputPin(uint8_t pin_num, uint8_t level) {
    output_val_ = ReadReg(TCA9537_REG_OUTPUT);
    // 修改本地缓存
    if (level) {
        output_val_ |= (1 << pin_num);  // 置 1 (OR 操作)
    } else {
        output_val_ &= ~(1 << pin_num); // 置 0 (AND NOT 操作)
    }
    // 将完整字节写入芯片
    WriteReg(TCA9537_REG_OUTPUT, output_val_);
}

void Tca9537::SetPinConfiguration(uint8_t pin_num, uint8_t direction) {
    // 从芯片读取当前配置
    config_cache_ = ReadReg(TCA9537_REG_CONFIG);

    // 只修改对应的位
    if (direction == TCA9537_OUTPUT) { 
        // 设为输出：需要把对应位清零 (Bit = 0)
        config_cache_ &= ~(1 << pin_num); 
    } else {             
        // 设为输入：需要把对应位置一 (Bit = 1)
        config_cache_ |= (1 << pin_num);  
    }
    // 把修改后的值写回
    WriteReg(TCA9537_REG_CONFIG, config_cache_);
}

int Tca9537::GetOutputPin(uint8_t pin_num) {
    // Read output_val_ from RAM 
    return (output_val_ >> pin_num) & 0x01;
}

uint8_t Tca9537::GetPinDirection(uint8_t pin_num) {
    if (pin_num > 7) return 0; // 简单的越界保护

    // 读取配置寄存器 (Register 0x03)
    uint8_t config_val = ReadReg(TCA9537_REG_CONFIG);

    // 提取对应位的状态
    // 右移 pin_num 位，然后与 1 进行“与”运算
    return (config_val >> pin_num) & 0x01;
}
