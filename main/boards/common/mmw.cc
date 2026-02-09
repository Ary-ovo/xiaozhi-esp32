/*
 * SPDX-FileCopyrightText: 2024 Xiaozhi
 * SPDX-License-Identifier: Apache-2.0
 * Target Device: Migan Technology AT2410 Radar
 */

#include "mmw.h"
#include "esp_log.h"
#include <cstring>
#include <numeric>

static const char *TAG = "AT2410";

// 协议常量定义
static const uint8_t SEND_HEADER = 0x58; // 发送帧头 [cite: 36]
static const uint8_t RECV_HEADER = 0x59; // 响应帧头/上报帧头 [cite: 44]

// 命令码定义
static const uint8_t CMD_ENABLE_REPORT = 0x53; // 开启/取消算法结果自动上报 [cite: 890]
static const uint8_t CMD_REPORT_DATA   = 0x52; // 算法结果自动上报的数据帧命令码 [cite: 899]
static const uint8_t CMD_GET_INFO      = 0x30; // 查询雷达感应信息 [cite: 286]

MmwRadar::MmwRadar(const Config& config) : config_(config) {
}

MmwRadar::~MmwRadar() {
    Stop();
}

esp_err_t MmwRadar::Start() {
    ESP_LOGI(TAG, "Initializing AT2410 Radar...");
    // 1. 配置 UART
    uart_config_t uart_config = {
        .baud_rate = MMW_DEFAULT_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_driver_install(config_.uart_port, MMW_UART_BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(config_.uart_port, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(config_.uart_port, config_.tx_pin, config_.rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    // 2. 启动任务
    is_running_ = true;
    BaseType_t ret = xTaskCreate(UartTaskWrapper, "mmw_task", 4096, this, 11, &task_handle_);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create task");
        return ESP_FAIL;
    }
    // 3. 发送初始化指令：开启自动上报
    // 延时一小会儿等待串口稳定
    vTaskDelay(pdMS_TO_TICKS(100));
    EnableAutoReport();

    ESP_LOGI(TAG, "Radar started on UART%d", config_.uart_port);
    return ESP_OK;
}

void MmwRadar::Stop() {
    is_running_ = false;
    if (task_handle_) {
        vTaskDelete(task_handle_);
        task_handle_ = nullptr;
    }
    uart_driver_delete(config_.uart_port);
}

void MmwRadar::SetStateChangedCallback(std::function<void(MmwState)> callback) {
    callback_ = callback;
}

// 开启算法结果自动上报 
esp_err_t MmwRadar::EnableAutoReport() {
    // 参数: 0x01 (启动自动上报) [cite: 892]
    return SendCommand(CMD_ENABLE_REPORT, {0x01});
}

// 发送指令 [cite: 35, 41]
// Frame: 0x58 + CMD + LEN + DATA + Checksum(Low) + Checksum(High)
esp_err_t MmwRadar::SendCommand(uint8_t cmd, const std::vector<uint8_t>& data) {
    std::vector<uint8_t> frame;
    frame.reserve(5 + data.size());

    uint8_t len = (uint8_t)data.size(); // 参数长度 [cite: 38]

    frame.push_back(SEND_HEADER);
    frame.push_back(cmd);
    frame.push_back(len);
    frame.insert(frame.end(), data.begin(), data.end());

    // 计算校验和: 0x58 + CMD + LEN + DATA [cite: 41]
    uint16_t checksum = SEND_HEADER + cmd + len;
    for (uint8_t b : data) {
        checksum += b;
    }

    // 小端发送校验码 [cite: 33]
    frame.push_back(checksum & 0xFF);
    frame.push_back((checksum >> 8) & 0xFF);

    int written = uart_write_bytes(config_.uart_port, frame.data(), frame.size());
    return (written == frame.size()) ? ESP_OK : ESP_FAIL;
}

void MmwRadar::UartTaskWrapper(void* param) {
    static_cast<MmwRadar*>(param)->UartTask();
}

void MmwRadar::UartTask() {
    uint8_t* temp_buf = (uint8_t*)malloc(MMW_UART_BUF_SIZE);
    
    while (is_running_) {
        int len = uart_read_bytes(config_.uart_port, temp_buf, MMW_UART_BUF_SIZE, pdMS_TO_TICKS(50));
        if (len > 0) {
            ParsePacket(temp_buf, len);
        }
    }
    free(temp_buf);
    vTaskDelete(NULL);
}

// 协议解析器
// 响应帧格式: 0x59 + CMD + LEN + DATA + Checksum [cite: 43]
void MmwRadar::ParsePacket(const uint8_t* data, int len) {
    buffer_.insert(buffer_.end(), data, data + len);

    // 最小帧长: Head(1) + Cmd(1) + Len(1) + Checksum(2) = 5 bytes (当Data为0时)
    while (buffer_.size() >= 5) {
        // 1. 寻找帧头 [cite: 44]
        if (buffer_[0] != RECV_HEADER) {
            buffer_.erase(buffer_.begin());
            continue;
        }

        uint8_t cmd = buffer_[1];
        uint8_t data_len = buffer_[2]; // 参数长度 [cite: 46]

        // 检查缓冲区是否包含完整的一帧
        // 完整长度 = Header(1) + Cmd(1) + Len(1) + Data(data_len) + Checksum(2)
        size_t total_frame_len = 3 + data_len + 2;

        if (buffer_.size() < total_frame_len) {
            // 数据不够，等待下次接收
            break;
        }

        // 2. 校验和验证 [cite: 48]
        // 校验范围: Header + Cmd + Len + Data
        uint16_t calc_sum = 0;
        for (size_t i = 0; i < total_frame_len - 2; i++) {
            calc_sum += buffer_[i];
        }

        // 读取接收到的校验码 (小端)
        uint16_t recv_sum = buffer_[total_frame_len - 2] | (buffer_[total_frame_len - 1] << 8);

        if (calc_sum == recv_sum) {
            // 提取 Data 部分
            std::vector<uint8_t> frame_payload(buffer_.begin() + 3, buffer_.begin() + 3 + data_len);
            
            // 处理不同的命令
            if (cmd == CMD_REPORT_DATA || cmd == CMD_GET_INFO) {
                HandleFrame(frame_payload);
            } else if (cmd == CMD_ENABLE_REPORT) {
                ESP_LOGI(TAG, "Auto Report Enabled Response Received");
            }
            
            // 移除处理完的帧
            buffer_.erase(buffer_.begin(), buffer_.begin() + total_frame_len);
        } else {
            ESP_LOGW(TAG, "Checksum failed: Calc 0x%04X != Recv 0x%04X", calc_sum, recv_sum);
            // 校验失败，移除帧头，尝试重新同步
            buffer_.erase(buffer_.begin());
        }
    }
}

// 处理具体的数据帧
// 数据结构参考 3.2.5 [cite: 333]
void MmwRadar::HandleFrame(const std::vector<uint8_t>& frame) {
    // 确保数据长度足够解析出 is_detected
    // 结构体前两项: frame_id (4 bytes) + is_detected (2 bytes) = 6 bytes
    if (frame.size() < 6) return;

    // is_detected 位于偏移量 4 (第5、6字节) [cite: 335, 337]
    // 小端解析
    uint16_t is_detected = frame[4] | (frame[5] << 8);

    // 解析 is_detected 位域 [cite: 364]
    // Bit 0: 综合判定结果有无触发 (1: 有, 0: 无)
    // Bit 1: 运动触发
    // Bit 2: 存在触发
    bool has_target = (is_detected & 0x01) != 0;

    MmwState new_state = has_target ? MmwState::PRESENCE : MmwState::NO_PRESENCE;

    // 也可以根据 Bit1 (Motion) 和 Bit2 (Presence) 做更细致的区分
    // bool motion = (is_detected & 0x02) != 0;
    // bool presence = (is_detected & 0x04) != 0;

    HandleStateChange(new_state);
    
    // 调试日志：打印距离信息 (move_info range_cm)
    // move_info 结构体起始位置: 4(id) + 2(det) + 1(max) + 1(count) = 8
    // move_info_t { range(2), angle(2), mag(2), snr(2) }
    /*
    if (has_target && frame.size() >= 10) {
         uint16_t range = frame[8] | (frame[9] << 8);
         ESP_LOGD(TAG, "Target Detected. Range: %d cm", range);
    }
    */
}

void MmwRadar::HandleStateChange(MmwState new_state) {
    if (current_state_ != new_state) {
        current_state_ = new_state;
        ESP_LOGI(TAG, "State Change: %s", (new_state == MmwState::PRESENCE) ? "PRESENCE" : "NO_PRESENCE");
        if (callback_) {
            callback_(current_state_);
        }
    }
}