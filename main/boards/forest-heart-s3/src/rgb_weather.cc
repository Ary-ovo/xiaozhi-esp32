#include "rgb_weather.h"
#include <math.h>
#include <stdlib.h>
#include <esp_log.h>
#include "sdkconfig.h"

#include "weather_service.h"

static const char* TAG = "RgbWeather";

RgbWeather::RgbWeather(RgbwStrip* strip) : strip_(strip) {
}

RgbWeather::~RgbWeather() {
    Stop();
}

void RgbWeather::StartWeatherTask(int interval_ms, std::function<void()> cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (weather_timer_ != nullptr) {
        esp_timer_stop(weather_timer_);
        esp_timer_delete(weather_timer_);
        weather_timer_ = nullptr;
    }

    weather_callback_ = cb;

    esp_timer_create_args_t timer_args = {};
    timer_args.callback = [](void* arg) {
        RgbWeather* self = static_cast<RgbWeather*>(arg);
        if (self->weather_callback_) {
            self->weather_callback_();
        }
    };
    timer_args.arg = this;
    timer_args.name = "weather_timer";
    timer_args.dispatch_method = ESP_TIMER_TASK;

    esp_timer_create(&timer_args, &weather_timer_);
    esp_timer_start_periodic(weather_timer_, interval_ms * 1000);
}

void RgbWeather::Stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (weather_timer_ != nullptr) {
        esp_timer_stop(weather_timer_);
        esp_timer_delete(weather_timer_);
        weather_timer_ = nullptr;
        ESP_LOGI(TAG, "Weather animation stopped.");
    }
    weather_callback_ = nullptr;
}

// ==========================================
// 1. 雷雨系 (Thunderstorm)
// ==========================================
void RgbWeather::StartThunderstorm(int interval_ms, float speed_divisor, float density) {
    if (!strip_) return;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        lightning_step_ = 0;
        lightning_timer_ = 0;
    }

    StartWeatherTask(interval_ms, [this, speed_divisor, density]() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // 闪电白光状态机计算 (基于您之前的设计)
        uint8_t raw_global_w = 0; 
        if (lightning_step_ == 0) {
            if ((rand() % 1000) < 10) { 
                lightning_step_ = 1; lightning_timer_ = 0;
            }
        } else {
            lightning_timer_++;
            if (lightning_step_ == 1) { 
                raw_global_w = 120; // 强闪白光 (可替换为您的 ledmaxlight_)
                if (lightning_timer_ > 2) { lightning_step_ = 2; lightning_timer_ = 0; }
            } 
            else if (lightning_step_ == 2) { 
                raw_global_w = 0;   // 黑暗 (darklight_)
                if (lightning_timer_ > 3) { lightning_step_ = 3; lightning_timer_ = 0; }
            }
            else if (lightning_step_ == 3) { 
                raw_global_w = 30;  // 弱闪白光 (ledweaklight_)
                if (lightning_timer_ > 2) { lightning_step_ = 4; lightning_timer_ = 0; }
            }
            else if (lightning_step_ == 4) { 
                raw_global_w = 0;   // 黑暗缓冲 (darklight_)
                if (lightning_timer_ > 4) { lightning_step_ = 0; }
            }
        }

        // 缩放白光亮度
        uint8_t final_w = (raw_global_w * brightness_percent_) / 100;

        uint32_t now = (uint32_t)(esp_timer_get_time() / 1000ULL);
        uint8_t max_leds = strip_->GetMaxLeds();
        
        for (int i = 0; i < max_leds; i++) {
            float phase = (now / speed_divisor) + (i * density);
            float breath = (sin(phase) + 1.0) / 2.0; 
            
            uint8_t raw_r = 0;
            uint8_t raw_g = 5  + (uint8_t)(breath * 15);
            uint8_t raw_b = 30 + (uint8_t)(breath * 80);
            
            uint8_t final_r = (raw_r * brightness_percent_) / 100;
            uint8_t final_g = (raw_g * brightness_percent_) / 100;
            uint8_t final_b = (raw_b * brightness_percent_) / 100;

            RgbwColor current_color = {final_r, final_g, final_b, final_w};
            strip_->SetPixelWithoutRefresh(i, current_color);
        }
        
        led_strip_handle_t raw_strip = strip_->GetHandle();
        if (raw_strip != nullptr) led_strip_refresh(raw_strip); 
    });
}

// ==========================================
// 2. 晴天系 (Sunny)
// ==========================================
void RgbWeather::StartSunny(int interval_ms, float speed_divisor, float density) {
    if (!strip_) return;
    StartWeatherTask(interval_ms, [this, speed_divisor, density]() {
        std::lock_guard<std::mutex> lock(mutex_);
        uint32_t now = (uint32_t)(esp_timer_get_time() / 1000ULL);
        uint8_t max_leds = strip_->GetMaxLeds();
        
        for (int i = 0; i < max_leds; i++) {
            float phase = (now / speed_divisor) + (i * density); 
            float breath = (sin(phase) + 1.0) / 2.0; 
            
            uint8_t raw_r = 10 + (uint8_t)(breath * 200); 
            uint8_t raw_g = 2  + (uint8_t)(breath * 70);
            
            uint8_t final_r = (raw_r * brightness_percent_) / 100;
            uint8_t final_g = (raw_g * brightness_percent_) / 100;

            RgbwColor current_color = {final_r, final_g, 0, 0};
            strip_->SetPixelWithoutRefresh(i, current_color);
        }
        
        led_strip_handle_t raw_strip = strip_->GetHandle();
        if (raw_strip != nullptr) led_strip_refresh(raw_strip); 
    });
}

// ==========================================
// 3. 雨天系 (Rainy)
// ==========================================
void RgbWeather::StartRainy(int interval_ms, float speed_divisor, float density) {
    if (!strip_) return;
    StartWeatherTask(interval_ms, [this, speed_divisor, density]() {
        std::lock_guard<std::mutex> lock(mutex_);
        uint32_t now = (uint32_t)(esp_timer_get_time() / 1000ULL);
        uint8_t max_leds = strip_->GetMaxLeds();
        
        for (int i = 0; i < max_leds; i++) {
            float phase = (now / speed_divisor) + (i * density);
            float breath = (sin(phase) + 1.0) / 2.0; 
            
            uint8_t raw_g = 10 + (uint8_t)(breath * 30);
            uint8_t raw_b = 40 + (uint8_t)(breath * 110);

            uint8_t final_g = (raw_g * brightness_percent_) / 100;
            uint8_t final_b = (raw_b * brightness_percent_) / 100;

            RgbwColor current_color = {0, final_g, final_b, 0};
            strip_->SetPixelWithoutRefresh(i, current_color);
        }
        
        led_strip_handle_t raw_strip = strip_->GetHandle();
        if (raw_strip != nullptr) led_strip_refresh(raw_strip); 
    });
}

// ==========================================
// 4. 雪天系 (Snowy)
// ==========================================
void RgbWeather::StartSnowy(int interval_ms, float speed_divisor, float density) {
    if (!strip_) return;
    StartWeatherTask(interval_ms, [this, speed_divisor, density]() {
        std::lock_guard<std::mutex> lock(mutex_);
        uint32_t now = (uint32_t)(esp_timer_get_time() / 1000ULL);
        uint8_t max_leds = strip_->GetMaxLeds();
        
        for (int i = 0; i < max_leds; i++) {
            float phase = (now / speed_divisor) + (i * density);
            float breath = (sin(phase) + 1.0) / 2.0; 
            
            uint8_t raw_g = 5  + (uint8_t)(breath * 10);
            uint8_t raw_b = 20 + (uint8_t)(breath * 40);
            uint8_t raw_w = 10 + (uint8_t)(breath * 40); 

            uint8_t final_g = (raw_g * brightness_percent_) / 100;
            uint8_t final_b = (raw_b * brightness_percent_) / 100;
            uint8_t final_w = (raw_w * brightness_percent_) / 100;

            RgbwColor current_color = {0, final_g, final_b, final_w};
            strip_->SetPixelWithoutRefresh(i, current_color);
        }
        
        led_strip_handle_t raw_strip = strip_->GetHandle();
        if (raw_strip != nullptr) led_strip_refresh(raw_strip); 
    });
}

// ==========================================
// 5. 雾霾沙尘系 (Foggy) - 全局呼吸，无需 density
// ==========================================
void RgbWeather::StartFoggy(int interval_ms, float speed_divisor) {
    if (!strip_) return;
    StartWeatherTask(interval_ms, [this, speed_divisor]() {
        std::lock_guard<std::mutex> lock(mutex_);
        uint32_t now = (uint32_t)(esp_timer_get_time() / 1000ULL);
        uint8_t max_leds = strip_->GetMaxLeds();
        
        float phase = (now / speed_divisor); 
        float breath = (sin(phase) + 1.0) / 2.0; 
        
        uint8_t raw_r = 20 + (uint8_t)(breath * 20); 
        uint8_t raw_g = 15 + (uint8_t)(breath * 15);
        uint8_t raw_b = 5  + (uint8_t)(breath * 5);
        uint8_t raw_w = 10 + (uint8_t)(breath * 20); 

        uint8_t final_r = (raw_r * brightness_percent_) / 100;
        uint8_t final_g = (raw_g * brightness_percent_) / 100;
        uint8_t final_b = (raw_b * brightness_percent_) / 100;
        uint8_t final_w = (raw_w * brightness_percent_) / 100;
        RgbwColor current_color = {final_r, final_g, final_b, final_w};

        for (int i = 0; i < max_leds; i++) {
            strip_->SetPixelWithoutRefresh(i, current_color);
        }
        
        led_strip_handle_t raw_strip = strip_->GetHandle();
        if (raw_strip != nullptr) led_strip_refresh(raw_strip); 
    });
}

// ==========================================
// 6. 阴天多云系 (Cloudy) - 全局呼吸，无需 density
// ==========================================
void RgbWeather::StartCloudy(int interval_ms, float speed_divisor) {
    if (!strip_) return;
    StartWeatherTask(interval_ms, [this, speed_divisor]() {
        std::lock_guard<std::mutex> lock(mutex_);
        uint32_t now = (uint32_t)(esp_timer_get_time() / 1000ULL);
        uint8_t max_leds = strip_->GetMaxLeds();
        
        float phase = (now / speed_divisor); 
        float breath = (sin(phase) + 1.0) / 2.0; 
        
        uint8_t raw_r = 5  + (uint8_t)(breath * 5);
        uint8_t raw_g = 10 + (uint8_t)(breath * 10);
        uint8_t raw_b = 20 + (uint8_t)(breath * 20);
        uint8_t raw_w = 5  + (uint8_t)(breath * 10); 

        uint8_t final_r = (raw_r * brightness_percent_) / 100;
        uint8_t final_g = (raw_g * brightness_percent_) / 100;
        uint8_t final_b = (raw_b * brightness_percent_) / 100;
        uint8_t final_w = (raw_w * brightness_percent_) / 100;
        RgbwColor current_color = {final_r, final_g, final_b, final_w};

        for (int i = 0; i < max_leds; i++) {
            strip_->SetPixelWithoutRefresh(i, current_color);
        }
        
        led_strip_handle_t raw_strip = strip_->GetHandle();
        if (raw_strip != nullptr) led_strip_refresh(raw_strip); 
    });
}


void RgbWeather::StartNightMoon(int interval_ms, float speed_divisor, float density) {
    if (!strip_) return;
    StartWeatherTask(interval_ms, [this, speed_divisor, density]() {
        std::lock_guard<std::mutex> lock(mutex_);
        uint32_t now = (uint32_t)(esp_timer_get_time() / 1000ULL);
        uint8_t max_leds = strip_->GetMaxLeds();
        
        for (int i = 0; i < max_leds; i++) {
            uint8_t raw_r, raw_g, raw_b, raw_w;
            
            if (i == 0) {
                raw_r = 60; raw_g = 40; raw_b = 10; raw_w = 10;
            } else {
                float phase = (now / speed_divisor) + (i * 13.7f * density); 
                float base_breath = (sin(phase) + 1.0) / 2.0; 
                float breath = pow(base_breath, 3.0); 

                raw_r = 2 + (uint8_t)(breath * 15);
                raw_g = 0;
                raw_b = 10 + (uint8_t)(breath * 50);
                
                raw_w = (uint8_t)(breath * 20); 
            }

            uint8_t final_r = (raw_r * brightness_percent_) / 100;
            uint8_t final_g = (raw_g * brightness_percent_) / 100;
            uint8_t final_b = (raw_b * brightness_percent_) / 100;
            uint8_t final_w = (raw_w * brightness_percent_) / 100;

            RgbwColor current_color = {final_r, final_g, final_b, final_w};
            strip_->SetPixelWithoutRefresh(i, current_color);
        }
        
        led_strip_handle_t raw_strip = strip_->GetHandle();
        if (raw_strip != nullptr) led_strip_refresh(raw_strip); 
    });
}

void RgbWeather::SetBrightness(uint8_t percent) {
    std::lock_guard<std::mutex> lock(mutex_);
    brightness_percent_ = (percent > 100) ? 100 : percent; 
}
