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

bool Axp2101::IsBatteryConnected() {
    uint8_t status_val = ReadReg(0x00);
    return (status_val & 0x08) != 0;
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

void Axp2101::PowerOnALDO1() {
    WriteReg(0x92, 0x1C);
    uint8_t value = ReadReg(0x90);
    value = value | 0x01;
    WriteReg(0x90, value);
}

void Axp2101::PowerOffALDO1() {
    uint8_t value = ReadReg(0x90);
    value = value & (~0x01);
    WriteReg(0x90, value);
}

void Axp2101::PowerOnALDO2() {
    WriteReg(0x93, 0x1C);
    uint8_t value = ReadReg(0x90);
    value = value | 0x02;
    WriteReg(0x90, value);
}

void Axp2101::PowerOffALDO2() {
    uint8_t value = ReadReg(0x90);
    value = value & (~0x02);
    WriteReg(0x90, value);
}
