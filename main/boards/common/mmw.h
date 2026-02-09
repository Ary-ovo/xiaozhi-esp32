/*
 * SPDX-FileCopyrightText: 2024 Xiaozhi
 * SPDX-License-Identifier: Apache-2.0
 * Target Device: Migan Technology AT2410 Radar
 */

#pragma once

#include "driver/uart.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <functional>
#include <vector>
#include <cstdint>

// AT2410 默认波特率通常为 256000 (根据常见的雷达模组配置，如手册未明确指定默认值，需根据实际情况调整，此处暂设为 115200 或 256000)
// 注意：请确认实际模组波特率
#define MMW_DEFAULT_BAUD_RATE 115200 
#define MMW_UART_BUF_SIZE     512

/**
 * @brief AT2410 雷达状态枚举
 * 基于 is_detected 字段解析 
 */
enum class MmwState {
    NO_PRESENCE = 0, // 无人
    PRESENCE = 1     // 有人 (运动或微动/存在)
};

class MmwRadar {
public:
    struct Config {
        uart_port_t uart_port;
        int tx_pin;
        int rx_pin;
        int irq_pin; // OUT
    };

    /**
     * @brief 构造函数
     * @param config UART配置参数
     */
    MmwRadar(const Config& config);

    /**
     * @brief 析构函数
     */
    ~MmwRadar();

    /**
     * @brief 初始化 UART 并启动接收任务
     * @return ESP_OK 成功
     */
    esp_err_t Start();

    /**
     * @brief 停止任务并释放资源
     */
    void Stop();

    /**
     * @brief 设置状态变化回调函数
     */
    void SetStateChangedCallback(std::function<void(MmwState)> callback);

    /**
     * @brief 获取当前状态
     */
    MmwState GetState() const { return current_state_; }

    /**
     * @brief 发送开启自动上报指令 
     */
    esp_err_t EnableAutoReport();

private:
    Config config_;
    TaskHandle_t task_handle_ = nullptr;
    bool is_running_ = false;
    MmwState current_state_ = MmwState::NO_PRESENCE;
    std::function<void(MmwState)> callback_ = nullptr;

    // 数据接收缓存
    std::vector<uint8_t> buffer_;

    /**
     * @brief FreeRTOS 任务封装
     */
    static void UartTaskWrapper(void* param);
    void UartTask();

    /**
     * @brief 解析数据包
     * @param data 新接收的数据
     * @param len 数据长度
     */
    void ParsePacket(const uint8_t* data, int len);

    /**
     * @brief 处理完整的协议帧
     * @param frame 除去校验位的完整数据帧
     */
    void HandleFrame(const std::vector<uint8_t>& frame);

    /**
     * @brief 发送指令底层函数
     * 遵循发送帧格式：0x58 + CMD + LEN + DATA + Checksum [cite: 35]
     */
    esp_err_t SendCommand(uint8_t cmd, const std::vector<uint8_t>& data);
    
    /**
     * @brief 状态机处理状态变化
     */
    void HandleStateChange(MmwState new_state);
};