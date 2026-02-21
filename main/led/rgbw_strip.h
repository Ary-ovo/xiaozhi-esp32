/*
 * rgbw_strip.h
 * 适配 SK6812 RGBW 灯带的驱动类
 */
#pragma once

#include "driver/gpio.h"
#include "led_strip.h"
#include <vector>
#include <mutex>
#include <functional>
#include "esp_timer.h"
#include "led.h"

// 定义包含白光通道的颜色结构
struct RgbwColor {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t white; // [新增] 第4通道
};

class RgbwStrip : public Led {
public:
    /**
     * @brief 构造函数
     * @param gpio 控制引脚
     * @param max_leds 灯珠数量
     */
    RgbwStrip(gpio_num_t gpio, uint8_t max_leds);
    ~RgbwStrip();

    // 基础控制
    void SetAllColor(RgbwColor color);
    void SetSingleColor(uint8_t index, RgbwColor color);
    void SetBrightness(uint8_t default_brightness, uint8_t low_brightness);
    
    // 动画效果
    void Blink(RgbwColor color, int interval_ms = 500);
    void FadeOut(int interval_ms = 50);
    void Breathe(RgbwColor low, RgbwColor high, int interval_ms = 50);
    void Scroll(RgbwColor low, RgbwColor high, int length = 3, int interval_ms = 100);

    void SetPixelWithoutRefresh(uint8_t index, RgbwColor color);
    void RefreshHardware();

    // 状态机响应
    void OnStateChanged() override;

    uint8_t GetMaxLeds() const { return max_leds_; }
    led_strip_handle_t GetHandle() const;
private:
    // 内部使用的定时任务启动器
    void StartStripTask(int interval_ms, std::function<void()> cb);

    led_strip_handle_t led_strip_ = nullptr;
    uint8_t max_leds_;
    std::vector<RgbwColor> colors_; // 本地缓存当前颜色
    
    std::mutex mutex_;
    esp_timer_handle_t strip_timer_;
    std::function<void()> strip_callback_ = nullptr;

    uint8_t default_brightness_ = 30;
    uint8_t low_brightness_ = 10;
};