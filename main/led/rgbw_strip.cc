/*
 * rgbw_strip.cc
 */
#include "rgbw_strip.h"
#include "application.h"
#include <esp_log.h>

#define TAG "RgbwStrip"

RgbwStrip::RgbwStrip(gpio_num_t gpio, uint8_t max_leds) : max_leds_(max_leds) {
    assert(gpio != GPIO_NUM_NC);

    colors_.resize(max_leds_);

    led_strip_config_t strip_config = {};
    strip_config.strip_gpio_num = gpio;
    strip_config.max_leds = max_leds_;
    strip_config.led_model = LED_MODEL_SK6812; 
    strip_config.color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRBW;
    strip_config.flags.invert_out = false;

    led_strip_rmt_config_t rmt_config = {};
    rmt_config.resolution_hz = 10 * 1000 * 1000;
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip_));
    
    led_strip_clear(led_strip_);

    esp_timer_create_args_t strip_timer_args = {
        .callback = [](void *arg) {
            auto strip = static_cast<RgbwStrip*>(arg);
            std::lock_guard<std::mutex> lock(strip->mutex_);
            if (strip->strip_callback_ != nullptr) {
                strip->strip_callback_();
            }
        },
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "rgbw_strip_timer",
        .skip_unhandled_events = false,
    };
    ESP_ERROR_CHECK(esp_timer_create(&strip_timer_args, &strip_timer_));
    
    ESP_LOGI(TAG, "Initialized SK6812 RGBW Strip on GPIO %d", gpio);
}

RgbwStrip::~RgbwStrip() {
    esp_timer_stop(strip_timer_);
    esp_timer_delete(strip_timer_);
    if (led_strip_ != nullptr) {
        led_strip_del(led_strip_);
    }
}

void RgbwStrip::SetAllColor(RgbwColor color) {
    std::lock_guard<std::mutex> lock(mutex_);
    esp_timer_stop(strip_timer_);
    
    for (int i = 0; i < max_leds_; i++) {
        colors_[i] = color;
        led_strip_set_pixel_rgbw(led_strip_, i, color.red, color.green, color.blue, color.white);
    }
    led_strip_refresh(led_strip_);
}

void RgbwStrip::SetSingleColor(uint8_t index, RgbwColor color) {
    std::lock_guard<std::mutex> lock(mutex_);
    esp_timer_stop(strip_timer_);
    
    if (index < max_leds_) {
        colors_[index] = color;
        led_strip_set_pixel_rgbw(led_strip_, index, color.red, color.green, color.blue, color.white);
        led_strip_refresh(led_strip_);
    }
}

void RgbwStrip::Blink(RgbwColor color, int interval_ms) {
    for (int i = 0; i < max_leds_; i++) {
        colors_[i] = color;
    }
    
    StartStripTask(interval_ms, [this]() {
        static bool on = true;
        if (on) {
            for (int i = 0; i < max_leds_; i++) {
                led_strip_set_pixel_rgbw(led_strip_, i, colors_[i].red, colors_[i].green, colors_[i].blue, colors_[i].white);
            }
            led_strip_refresh(led_strip_);
        } else {
            led_strip_clear(led_strip_);
        }
        on = !on;
    });
}

void RgbwStrip::FadeOut(int interval_ms) {
    StartStripTask(interval_ms, [this]() {
        bool all_off = true;
        for (int i = 0; i < max_leds_; i++) {
            colors_[i].red /= 2;
            colors_[i].green /= 2;
            colors_[i].blue /= 2;
            colors_[i].white /= 2;

            if (colors_[i].red > 0 || colors_[i].green > 0 || colors_[i].blue > 0 || colors_[i].white > 0) {
                all_off = false;
            }
            led_strip_set_pixel_rgbw(led_strip_, i, colors_[i].red, colors_[i].green, colors_[i].blue, colors_[i].white);
        }
        
        if (all_off) {
            led_strip_clear(led_strip_);
            esp_timer_stop(strip_timer_);
        } else {
            led_strip_refresh(led_strip_);
        }
    });
}

void RgbwStrip::Breathe(RgbwColor low, RgbwColor high, int interval_ms) {
    StartStripTask(interval_ms, [this, low, high]() {
        static bool increase = true;
        static RgbwColor color = low;
        
        auto update_channel = [](uint8_t &current, uint8_t target, bool inc) {
            if (inc) {
                if (current < target) current++;
            } else {
                if (current > target) current--;
            }
        };

        update_channel(color.red,   increase ? high.red : low.red,     increase);
        update_channel(color.green, increase ? high.green : low.green, increase);
        update_channel(color.blue,  increase ? high.blue : low.blue,   increase);
        update_channel(color.white, increase ? high.white : low.white, increase);

        bool reached_target = true;
        if (increase) {
            if (color.red < high.red || color.green < high.green || color.blue < high.blue || color.white < high.white) reached_target = false;
        } else {
            if (color.red > low.red || color.green > low.green || color.blue > low.blue || color.white > low.white) reached_target = false;
        }

        if (reached_target) {
            increase = !increase;
        }

        for (int i = 0; i < max_leds_; i++) {
            led_strip_set_pixel_rgbw(led_strip_, i, color.red, color.green, color.blue, color.white);
        }
        led_strip_refresh(led_strip_);
    });
}

void RgbwStrip::Scroll(RgbwColor low, RgbwColor high, int length, int interval_ms) {

    for (int i = 0; i < max_leds_; i++) {
        colors_[i] = low;
    }
    
    StartStripTask(interval_ms, [this, low, high, length]() {
        static int offset = 0;
        
        for (int i = 0; i < max_leds_; i++) {
            colors_[i] = low;
        }
        for (int j = 0; j < length; j++) {
            int i = (offset + j) % max_leds_;
            colors_[i] = high;
        }
        
        for (int i = 0; i < max_leds_; i++) {
            led_strip_set_pixel_rgbw(led_strip_, i, colors_[i].red, colors_[i].green, colors_[i].blue, colors_[i].white);
        }
        led_strip_refresh(led_strip_);
        offset = (offset + 1) % max_leds_;
    });
}

void RgbwStrip::StartStripTask(int interval_ms, std::function<void()> cb) {
    if (led_strip_ == nullptr) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    esp_timer_stop(strip_timer_);
    
    strip_callback_ = cb;
    esp_timer_start_periodic(strip_timer_, interval_ms * 1000);
}

void RgbwStrip::SetBrightness(uint8_t default_brightness, uint8_t low_brightness) {
    default_brightness_ = default_brightness;
    low_brightness_ = low_brightness;
    OnStateChanged();
}

void RgbwStrip::OnStateChanged() {
    auto& app = Application::GetInstance();
    auto device_state = app.GetDeviceState();
    
    RgbwColor off = {0, 0, 0, 0};
    RgbwColor pure_white_high = {0, 0, 0, default_brightness_};
    RgbwColor pure_white_low  = {0, 0, 0, low_brightness_};
    
    RgbwColor blue_high = {0, 0, default_brightness_, 0};
    RgbwColor blue_low  = {0, 0, low_brightness_, 0};

    RgbwColor green_high = {0, default_brightness_, 0, 0};
    
    RgbwColor red_high = {default_brightness_, 0, 0, 0};

    switch (device_state) {
        case kDeviceStateStarting:
            Scroll(blue_low, blue_high, 3, 100);
            break;

        case kDeviceStateWifiConfiguring:
            Breathe(blue_low, blue_high, 20); 
            break;

        case kDeviceStateIdle:
            FadeOut(50);
            break;

        case kDeviceStateConnecting:
            Scroll(pure_white_low, pure_white_high, 3, 100);
            break;

        case kDeviceStateListening:
        case kDeviceStateAudioTesting:
            SetAllColor(red_high);
            break;

        case kDeviceStateSpeaking:
            SetAllColor(green_high);
            break;

        case kDeviceStateUpgrading:
            Blink(green_high, 100);
            break;

        case kDeviceStateActivating:
            Blink(green_high, 500);
            break;

        default:
            ESP_LOGW(TAG, "Unknown device state: %d", device_state);
            break;
    }
}