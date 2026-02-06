#include "axp2101.h"
#include "board.h"
#include "display.h"

#include <esp_log.h>

#define TAG "Axp2101"

Axp2101::Axp2101(i2c_master_bus_handle_t i2c_bus, uint8_t addr) : I2cDevice(i2c_bus, addr) {
}

int Axp2101::GetBatteryCurrentDirection() {
    return (ReadReg(0x01) & 0b01100000) >> 5;
}

bool Axp2101::IsCharging() {
    return GetBatteryCurrentDirection() == 1;
}

bool Axp2101::IsDischarging() {
    return GetBatteryCurrentDirection() == 2;
}

bool Axp2101::IsChargingDone() {
    uint8_t value = ReadReg(0x01);
    return (value & 0b00000111) == 0b00000100;
}

int Axp2101::GetBatteryLevel() {
    return ReadReg(0xA4);
}

float Axp2101::GetTemperature() {
    return ReadReg(0xA5);
}

void Axp2101::PowerOff() {
    uint8_t value = ReadReg(0x10);
    value = value | 0x01;
    WriteReg(0x10, value);
}

// ==========================================
//                 ALDO1 控制
// ==========================================

// 打开 ALDO1 并设置电压为 3.3V
void Axp2101::PowerOnALDO1() {
    // 1. 设置 ALDO1 电压为 3.3V (寄存器 0x92)
    // 计算公式: (3.3V - 0.5V) / 0.1V = 28 (0x1C)
    WriteReg(0x92, 0x1C);

    // 2. 开启 ALDO1 (寄存器 0x90 Bit 0)
    uint8_t value = ReadReg(0x90);
    value = value | 0x01; // 置位 Bit 0 (二进制 0000 0001)
    WriteReg(0x90, value);
}

// 关闭 ALDO1
void Axp2101::PowerOffALDO1() {
    // 关闭 ALDO1 (寄存器 0x90 Bit 0)
    uint8_t value = ReadReg(0x90);
    value = value & (~0x01); // 清除 Bit 0 (与 1111 1110 进行与运算)
    WriteReg(0x90, value);
}

// ==========================================
//                 ALDO2 控制
// ==========================================

// 打开 ALDO2 并设置电压为 3.3V
void Axp2101::PowerOnALDO2() {
    // 1. 设置 ALDO2 电压为 3.3V (寄存器 0x93)
    WriteReg(0x93, 0x1C);

    // 2. 开启 ALDO2 (寄存器 0x90 Bit 1)
    uint8_t value = ReadReg(0x90);
    value = value | 0x02; // 置位 Bit 1 (二进制 0000 0010)
    WriteReg(0x90, value);
}

// 关闭 ALDO2
void Axp2101::PowerOffALDO2() {
    // 关闭 ALDO2 (寄存器 0x90 Bit 1)
    uint8_t value = ReadReg(0x90);
    value = value & (~0x02); // 清除 Bit 1 (与 1111 1101 进行与运算)
    WriteReg(0x90, value);
}
