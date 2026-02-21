/*
 * rgb_weather.h
 * 独立的天气灯效控制类，基于 RgbwStrip 驱动
 */
#pragma once

#include "led/rgbw_strip.h"
#include "esp_timer.h"
#include <mutex>
#include <functional>

class RgbWeather {
public:
    /**
     * @brief 构造函数
     * @param strip 指向开发板 RgbwStrip 实例的指针
     */
    RgbWeather(RgbwStrip* strip);
    ~RgbWeather();

    void SetBrightness(uint8_t percent);

    // --- 天气动画效果 ---
    // 1. 雷雨 (speed_divisor越小波浪越快，density越大波浪越密)
    void StartThunderstorm(int interval_ms = 20, float speed_divisor = 150.0f, float density = 1.2f);
    // 2. 晴天/白天 (光影流转)
    void StartSunny(int interval_ms = 30, float speed_divisor = 1000.0f, float density = 0.5f);
    // 3. 雨天 (小雨到暴雨，调速即可)
    void StartRainy(int interval_ms = 20, float speed_divisor = 200.0f, float density = 1.0f);
    // 4. 雪天 (慢速飘落)
    void StartSnowy(int interval_ms = 30, float speed_divisor = 800.0f, float density = 0.6f);
    // 5. 雾霾沙尘 (全局呼吸，无波浪，传入速度即可)
    void StartFoggy(int interval_ms = 40, float speed_divisor = 1500.0f);
    // 6. 阴天多云 (全局云层翻涌)
    void StartCloudy(int interval_ms = 40, float speed_divisor = 1200.0f);
    // 7. 月相夜空 (星河流动)
    void StartNightMoon(int interval_ms = 30, float speed_divisor = 2000.0f, float density = 0.2f);

    void Stop();

private:
    // 内部使用的定时任务启动器 (保持与 RgbwStrip 相同的底层风格)
    void StartWeatherTask(int interval_ms, std::function<void()> cb);

    RgbwStrip* strip_ = nullptr;
    
    std::mutex mutex_;
    esp_timer_handle_t weather_timer_ = nullptr;
    std::function<void()> weather_callback_ = nullptr;

    // --- 雷雨状态机专用变量 ---
    int lightning_step_ = 0;
    int lightning_timer_ = 0;

    uint8_t ledmaxlight_ = 30;
    uint8_t ledweaklight_ = 15;
    uint8_t darklight_ = 0;

    uint8_t brightness_percent_ = 60;
};