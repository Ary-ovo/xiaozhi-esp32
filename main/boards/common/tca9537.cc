#include "tca9537.h"
#include <esp_log.h>

#define TAG "Tca9537"

Tca9537::Tca9537(i2c_master_bus_handle_t i2c_bus, uint8_t addr) : I2cDevice(i2c_bus, addr) {
}

void Tca9537::SetOutputPort(uint8_t value) {
    WriteReg(TCA9537_REG_OUTPUT, value);
}

void Tca9537::SetConfiguration(uint8_t dir_mask) {
    WriteReg(TCA9537_REG_CONFIG, dir_mask);
}

uint8_t Tca9537::GetOutputPort() {
    return ReadReg(TCA9537_REG_OUTPUT);
}

void Tca9537::SetPinLevel(uint8_t pin_mask, bool level) {
    uint8_t val = GetOutputPort();
    if (level) {
        val |= pin_mask;
    } else {
        val &= ~pin_mask;
    }
    SetOutputPort(val);
}