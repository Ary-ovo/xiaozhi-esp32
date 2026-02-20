#include "epd_display.h"
#include "assets/lang_config.h"
#include "lvgl_theme.h"
#include "lvgl_font.h"
#include "esp_lcd_gdew042t2.h"
#include "esp_heap_caps.h" 
#include "board.h"
#include "application.h"
#include <string>
#include "../config.h"
#include <algorithm>
#include <cstring>
#include <esp_log.h>
#include <esp_err.h>
#include <esp_lvgl_port.h>
#include <font_awesome.h>
#include "weather_service.h"
#include "weather_service.h"
#include "assets/lang_config.h"
#include <time.h>
#include <sys/time.h>

#ifndef CONFIG_BOARD_TYPE_FOREST_HEART_S3
    #error PLEASE CHECK YOU ARE FOREST HEART BOARD
#endif 

#include "gui_guider.h"
#include "custom.h"

lv_ui* guider_ui;

#define TAG "EpdDisplay"

LV_FONT_DECLARE(BUILTIN_TEXT_FONT);
LV_FONT_DECLARE(BUILTIN_ICON_FONT);
LV_FONT_DECLARE(font_awesome_30_1);

/**
 * @brief 初始化内存（ESP32 上电已自动初始化堆，此处留空即可）
 */
void lv_mem_init(void)
{
    // Noting to do
}

/**
 * @brief 反初始化内存
 */
void lv_mem_deinit(void)
{
    // Noting to do
}

/**
 * @brief 核心内存分配函数
 * @param size 需要分配的字节数
 * @return 指针，如果失败返回 NULL
 */
void * lv_malloc_core(size_t size)
{
    // 从 PSRAM (SPIRAM) 分配RAM
    return heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
}

/**
 * @brief 核心内存释放函数
 * @param p 要释放的指针
 */
void lv_free_core(void * p)
{
    heap_caps_free(p);
}

/**
 * @brief 核心内存重分配函数
 * @param p 旧指针
 * @param new_size 新大小
 * @return 新指针
 */
void * lv_realloc_core(void * p, size_t new_size)
{
    return heap_caps_realloc(p, new_size, MALLOC_CAP_SPIRAM);
}

void lv_mem_monitor_core(lv_mem_monitor_t * mon_p) {
    if(mon_p) lv_memzero(mon_p, sizeof(lv_mem_monitor_t));
}

lv_result_t lv_mem_test_core(void) {
    return LV_RESULT_OK;
}


static inline void set_pixel_1bpp_safe(uint8_t* buffer, int x, int y, int width, int height, bool is_white) {
    if (x < 0 || x >= width || y < 0 || y >= height) return;

    // 1bpp compact layout: 8 pixels per byte
    int stride = width / 8;
    int idx = y * stride + (x / 8);
    int bit = 7 - (x % 8); // MSB first
    
    if (is_white) {
        buffer[idx] |= (1 << bit);
    } else {
        buffer[idx] &= ~(1 << bit);
    }
}

void EpdDisplay::FlushCallback(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map)
{
    EpdDisplay* self = (EpdDisplay*)lv_display_get_user_data(disp);
    if (!self) {
        lv_display_flush_ready(disp);
        return;
    }
    int32_t w = area->x2 - area->x1 + 1;
    int32_t h = area->y2 - area->y1 + 1;
    uint16_t* src_pixels = (uint16_t*)px_map;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            uint16_t color = src_pixels[y * w + x];
            bool is_white = (color > 0x8000); 
            set_pixel_1bpp_safe((uint8_t*)self->lvgl_buf_, 
                                area->x1 + x, 
                                area->y1 + y, 
                                self->width_, 
                                self->height_, 
                                is_white);
        }
    }
    self->is_dirty_ = true;
    lv_display_flush_ready(disp);
}

void EpdDisplay::FlushToHardware()
{
    // 1. 基础检查
    if (!is_dirty_) return;
    if (display_queue_ == nullptr) return;

    int64_t now = esp_timer_get_time();
    if (now - last_refresh_time_ < 1500 * 1000) { 
        return; 
    }
    last_refresh_time_ = now;
    {
        DisplayLockGuard lock(this);
        memcpy(snapshot_buf_, lvgl_buf_, (width_ * height_) / 8);
        is_dirty_ = false;
    }

    EpdCmd cmd;
    cmd.force_full_refresh = false; 
    if (update_counter_ >= EPD_FULL_REFRESH_EVERY_X_FRAMES) {
        cmd.force_full_refresh = true;
    }
    xQueueOverwrite(display_queue_, &cmd);
}

// // Helper Functions
void EpdDisplay::TriggerFullRefresh() {
    update_counter_ = EPD_FULL_REFRESH_EVERY_X_FRAMES + 1;
    is_dirty_ = true;
}

// LOCKING: Must be blocking to prevent race conditions
bool EpdDisplay::Lock(int timeout_ms) {
    if (timeout_ms == 0) timeout_ms = 1000;
    return lvgl_port_lock(timeout_ms);
}

void EpdDisplay::Unlock() { 
    lvgl_port_unlock(); 
}

EpdDisplay::EpdDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel,
                       int width, int height)
    : panel_io_(panel_io), panel_(panel), width_(width), height_(height)
{
    auto text_font = std::make_shared<LvglBuiltInFont>(&BUILTIN_TEXT_FONT);
    auto icon_font = std::make_shared<LvglBuiltInFont>(&BUILTIN_ICON_FONT);
    auto large_icon_font = std::make_shared<LvglBuiltInFont>(&font_awesome_30_1);
    auto theme = new LvglTheme("epd");
    theme->set_text_font(text_font);
    theme->set_icon_font(icon_font);
    theme->set_large_icon_font(large_icon_font);
    LvglThemeManager::GetInstance().RegisterTheme("epd", theme);
    current_theme_ = theme;

    ESP_LOGI(TAG, "Initialize LVGL Port");
    lvgl_port_cfg_t port_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    port_cfg.task_priority = 4;
    port_cfg.task_stack = 8192; 
    port_cfg.timer_period_ms = 5;
#if CONFIG_SOC_CPU_CORES_NUM > 1
    port_cfg.task_affinity = 1;
#endif
    if (lvgl_port_init(&port_cfg) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init LVGL port");
        return;
    }
    if (!lvgl_port_lock(0)) return;

    display_ = lv_display_create(width, height);
    
    uint32_t full_1bpp_size = (width * height) / 8; // ~15KB
    uint32_t partial_height = height / 10;
    uint32_t draw_buf_size = width * partial_height * sizeof(uint16_t); // ~24KB

    draw_buf_ = heap_caps_malloc(draw_buf_size, MALLOC_CAP_SPIRAM);
    lvgl_buf_ = heap_caps_malloc(full_1bpp_size, MALLOC_CAP_SPIRAM);

    uint32_t safe_snapshot_size = 32 * 1024; 
    snapshot_buf_ = heap_caps_calloc(1, safe_snapshot_size, MALLOC_CAP_SPIRAM);

    if (!lvgl_buf_ || !snapshot_buf_ || !draw_buf_) {
        ESP_LOGE(TAG, "Failed to allocate buffers!");
        lvgl_port_unlock();
        return;
    }
    lv_display_set_color_format(display_, LV_COLOR_FORMAT_RGB565);
    lv_display_set_user_data(display_, this);
    lv_display_set_flush_cb(display_, FlushCallback);
    lv_display_set_buffers(display_, draw_buf_, NULL, draw_buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lvgl_port_unlock();
    SetupUI();
    display_queue_ = xQueueCreate(1, sizeof(EpdCmd));
    InitAsync();
    const esp_timer_create_args_t timer_args = {
        .callback = [](void *arg) { 
            static_cast<EpdDisplay*>(arg)->FlushToHardware(); 
        },
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "epd_refresh"
    };
    esp_timer_create(&timer_args, &refresh_timer_);
    esp_timer_start_periodic(refresh_timer_, 200 * 1000);
}

EpdDisplay::~EpdDisplay() {
    if (refresh_timer_) { esp_timer_stop(refresh_timer_); esp_timer_delete(refresh_timer_); }

    if(guider_ui->screen) lv_obj_delete(guider_ui->screen);
    if(guider_ui->screen_1) lv_obj_delete(guider_ui->screen_1);
    if(guider_ui) free(guider_ui);

    if (display_) lv_display_delete(display_);

    if (lvgl_buf_) free(lvgl_buf_);
    if (snapshot_buf_) free(snapshot_buf_);
    if (draw_buf_) free(draw_buf_);

    if (panel_) esp_lcd_panel_del(panel_);
    if (panel_io_) esp_lcd_panel_io_del(panel_io_);
}


void EpdDisplay::UpdateDateDisplay() {
    char buf_wday[16];
    char buf_date[16];
    char buf_mon[16];
    char buf_day[8];
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo); // 将时间戳转换为本地时间结构体

    strftime(buf_wday, sizeof(buf_wday), "%A", &timeinfo);
    strftime(buf_date, sizeof(buf_date), "%Y-%m-%d", &timeinfo);
    strftime(buf_mon, sizeof(buf_mon), "%B", &timeinfo);
    strftime(buf_day, sizeof(buf_day), "%d", &timeinfo);
    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
    ESP_LOGI(TAG, "当前系统时间: %s", strftime_buf);
    DisplayLockGuard lock(this);
    lv_label_set_text(guider_ui->screen_label_wday, buf_wday);
    lv_label_set_text(guider_ui->screen_label_date, buf_date);
    lv_label_set_text(guider_ui->screen_label_mon, buf_mon);
    lv_label_set_text(guider_ui->screen_label_day, buf_day);
}

void EpdDisplay::InitAsync() {
    xTaskCreate(DisplayTask, "epd_task", 4U * 1024, this, 1, NULL);
};

void EpdDisplay::DisplayTask(void* arg) {
    EpdDisplay* self = (EpdDisplay*)arg;
    EpdCmd cmd;
    ESP_LOGI(TAG, "Creating Epd Wait Thread, You won't let the audio wait for the Epd busy");
    while (true) {
        // 1. 阻塞等待命令。没有命令时，线程休眠，不占用 CPU。
        if (xQueueReceive(self->display_queue_, &cmd, portMAX_DELAY)) {
            ESP_LOGD(TAG, "E-Ink Task: Start Refresh...");
            if (self->update_counter_ >= EPD_FULL_REFRESH_EVERY_X_FRAMES || cmd.force_full_refresh) {
                 esp_lcd_gdew042t2_set_mode(self->panel_, GDEW042T2_REFRESH_FULL);
                 self->update_counter_ = 0;
            } else {
                 esp_lcd_gdew042t2_set_mode(self->panel_, GDEW042T2_REFRESH_PARTIAL);
                 self->update_counter_++; 
            }
            esp_lcd_panel_draw_bitmap(self->panel_, 0, 0, self->width_, self->height_, self->snapshot_buf_);
            ESP_LOGI(TAG, "E-Ink Task: FLUSH DONE");
        }
    }
}

// SetupUI
void EpdDisplay::SetupUI() {
    DisplayLockGuard lock(this);

    auto lvgl_theme = static_cast<LvglTheme*>(current_theme_);
    auto icon_font = lvgl_theme->icon_font()->font();

    if (guider_ui != NULL) return;
    guider_ui = (lv_ui *)malloc(sizeof(lv_ui));
    if (guider_ui == NULL) {
        LV_LOG_ERROR("UI Malloc Failed!");
        return;
    }
    custom_init(guider_ui);
    // 检查一下所有字体是否都加载了
    if (lv_font_zaozigongfangxinranti_92 && lv_font_MFYueHei_18) {
        setup_ui(guider_ui);
    } else {
        ESP_LOGE("App", "字体加载不全，跳过 UI 初始化以防止崩溃");
    }
    lv_obj_set_style_text_font(guider_ui->battery_label, icon_font, 0);
    lv_label_set_text(guider_ui->battery_label, FONT_AWESOME_BATTERY_FULL);
}

static char last_status_str[64] = {0};

void EpdDisplay::SetStatus(const char* status) {
    DisplayLockGuard lock(this);
    if (strcmp(status, Lang::Strings::ACTIVATION) == 0 ||          
        strcmp(status, Lang::Strings::CHECKING_NEW_VERSION) == 0 || 
        strcmp(status, Lang::Strings::LOADING_PROTOCOL) == 0 || 
        strcmp(status, Lang::Strings::REGISTERING_NETWORK) == 0 || 
        strcmp(status, Lang::Strings::DETECTING_MODULE) == 0 || 
        strcmp(status, Lang::Strings::CHECKING_NEW_VERSION) == 0 || 
        strcmp(status, Lang::Strings::LOADING_PROTOCOL) == 0 || 
        strcmp(status, Lang::Strings::CONNECTING) == 0) {    
        strncpy(last_status_str, status, sizeof(last_status_str) - 1);
        return; 
    }
    if (strcmp(status, last_status_str) != 0) {
        bool need_full_refresh = false;
        if (strcmp(status, Lang::Strings::STANDBY) == 0) {
            if (guider_ui != nullptr && guider_ui->screen != nullptr) {
                if (lv_scr_act() != guider_ui->screen) {
                    lv_scr_load(guider_ui->screen);
                    need_full_refresh = true; 
                }
            } else {
                need_full_refresh = true;
            }
        }
        else if (strcmp(status, Lang::Strings::LISTENING) == 0) {
            if (guider_ui != nullptr && guider_ui->screen_1 != nullptr) {
                if (lv_scr_act() != guider_ui->screen_1) {
                    lv_scr_load(guider_ui->screen_1);
                    need_full_refresh = true; 
                }
            }
            update_counter_++;
            if (strcmp(last_status_str, Lang::Strings::SPEAKING) != 0) {
                need_full_refresh = true;
            }
        }
        if (need_full_refresh) {
            this->TriggerFullRefresh(); 
        }
        if (need_full_refresh) {
             lv_refr_now(display_);
        }
        strncpy(last_status_str, status, sizeof(last_status_str) - 1);
    }
}
void EpdDisplay::SetChatMessage(const char* role, const char* content)
{
    if (role != nullptr && strcmp(role, "user") == 0) {
        return;
    }

    DisplayLockGuard lock(this);
    if (guider_ui == nullptr || guider_ui->screen_1 == nullptr) {
        return;
    }
    if (guider_ui->screen_1_chat_msg) {
        lv_label_set_text(guider_ui->screen_1_chat_msg, content);
    }
}

static char last_emotion_str[64] = {0};

void EpdDisplay::SetEmotion(const char* emotion) {
    ESP_LOGI(TAG, "SetEmotion: %s", emotion);
    DisplayLockGuard lock(this);

    if (strncmp(emotion, last_emotion_str, sizeof(last_emotion_str)) == 0) {
        ESP_LOGD(TAG, "Emotion is same as before (%s), skipping.", emotion);
        return;
    }

    if (guider_ui == nullptr) {
        ESP_LOGW("EpdDisplay", "SetEmotion ignored: guider_ui is NULL");
        return;
    }
    if (guider_ui->screen_1_emo_img == nullptr) {
        ESP_LOGW("EpdDisplay", "SetEmotion ignored: screen_1_emo_img is NULL");
        return;
    }

    strncpy(last_emotion_str, emotion, sizeof(last_emotion_str) - 1);

    char file_path[64];
    snprintf(file_path, sizeof(file_path), "S:/icons/%s.bin", emotion);
    lv_image_set_src(guider_ui->screen_1_emo_img, file_path);
    
    this->TriggerFullRefresh();

    if (lv_scr_act() == guider_ui->screen_1) {
        if (display_) {
            lv_refr_now(display_);
        }
    }
}

void EpdDisplay::UpdateStatusBar(bool update_all) {
    auto& app = Application::GetInstance();
    auto& board = Board::GetInstance();
    // Update time
    if (app.GetDeviceState() == kDeviceStateIdle) {
        if (last_status_update_time_ + std::chrono::seconds(10) < std::chrono::system_clock::now()) {
            time_t now = time(NULL);
            struct tm* tm = localtime(&now);
            if (tm->tm_year >= 2025 - 1900) {
                char time_str[16];
                strftime(time_str, sizeof(time_str), "%H:%M", tm);
            } else {
                ESP_LOGW(TAG, "System time is not set, tm_year: %d", tm->tm_year);
            }
        }
    }
    // Update battery icon
    esp_pm_lock_acquire(pm_lock_);
    int battery_level;
    bool charging, discharging;
    const char* icon = nullptr;
    if (board.GetBatteryLevel(battery_level, charging, discharging)) {
        if (battery_level < 0) {
            icon = FONT_AWESOME_BATTERY_SLASH;
        } 
        else if (charging) {
            icon = FONT_AWESOME_BATTERY_BOLT;
        } 
        else {
            const char* levels[] = {
                FONT_AWESOME_BATTERY_EMPTY,             // 0-19%
                FONT_AWESOME_BATTERY_QUARTER,           // 20-39%
                FONT_AWESOME_BATTERY_HALF,              // 40-59%
                FONT_AWESOME_BATTERY_THREE_QUARTERS,    // 60-79%
                FONT_AWESOME_BATTERY_FULL,              // 80-99%
                FONT_AWESOME_BATTERY_FULL               // 100%
            };
            int index = battery_level / 20;
            if (index > 5) index = 5; 
            if (index < 0) index = 0; 
            icon = levels[index];
        }
        DisplayLockGuard lock(this);
        if (guider_ui->battery_label != nullptr && 
           (battery_icon_ != icon || last_battery_level_ != battery_level)) {
            
            ESP_LOGI(TAG, "Battery value %d", battery_level);
            battery_icon_ = icon;
            last_battery_level_ = battery_level;
            lv_label_set_text(guider_ui->battery_label, battery_icon_);
            if (battery_level < 0) {
                lv_label_set_text(guider_ui->battery_value, "--%");
            } else {
                lv_label_set_text_fmt(guider_ui->battery_value, "%d%%", battery_level);
            }
        }
    }
    // Update network icon every 10 seconds
    static int seconds_counter = 0;
    if (update_all || seconds_counter++ % 10 == 0) {
        auto device_state = Application::GetInstance().GetDeviceState();
        static const std::vector<DeviceState> allowed_states = {
            kDeviceStateIdle,
        };
        if (std::find(allowed_states.begin(), allowed_states.end(), device_state) != allowed_states.end()) {
            icon = board.GetNetworkStateIcon();
            if (network_label_ != nullptr && icon != nullptr && network_icon_ != icon) {
                DisplayLockGuard lock(this);
                network_icon_ = icon;
                lv_obj_add_flag(guider_ui->battery_value, LV_OBJ_FLAG_HIDDEN);
                lv_obj_remove_flag(guider_ui->network_state, LV_OBJ_FLAG_HIDDEN);
                lv_label_set_text(network_label_, network_icon_);
            }
        }
        else lv_obj_add_flag(guider_ui->network_state, LV_OBJ_FLAG_HIDDEN);
    }

    esp_pm_lock_release(pm_lock_);

    // Update weather every 1 hour (check cache)
    static int weather_counter = 0;
    if (update_all || weather_counter++ % 1800 == 0) {
        UpdateWeatherUI();
    }
    // Update date per hour
    static int date_counter = 0;
    if (update_all || date_counter++ % 3600 == 0) {
        UpdateDateDisplay();
    }
}

void EpdDisplay::UpdateWeatherUI()
{
    DisplayLockGuard lock(this);
    auto weather = WeatherService::GetInstance().GetWeatherData();
    if (weather.is_valid && guider_ui->screen_label_now_weather_label && !weather.daily.empty()) {

        lv_obj_t* day_img_objs[] = {
        guider_ui->screen_img_weather_day1,
        guider_ui->screen_img_weather_day2,
        guider_ui->screen_img_weather_day3,
        guider_ui->screen_img_weather_day4,
        guider_ui->screen_img_weather_day5,
        guider_ui->screen_img_weather_day6,
        guider_ui->screen_img_weather_day7
        };
        char path_buffer[64];
        for (int i = 0; i < 7; i++) {
            lv_obj_t* target_img = day_img_objs[i];
            if (target_img != nullptr) {
                if (i < weather.daily.size()) {
                    std::string icon_code = weather.daily[i].icon_day;
                    snprintf(path_buffer, sizeof(path_buffer), "%s%s.bin", ICONS_PATH, icon_code.c_str());
                    lv_obj_clear_flag(target_img, LV_OBJ_FLAG_HIDDEN);
                    lv_image_set_src(target_img, path_buffer);
                    
                } else {
                    lv_obj_add_flag(target_img, LV_OBJ_FLAG_HIDDEN);
                }
            }
            char weather_str[128]; 
            snprintf(weather_str, sizeof(weather_str), 
                    "%s / 空气质量等级 %s / 体感温度 %s°C", 
                    weather.text.c_str(),
                    weather.aqi_category.c_str(),
                    weather.feels_like.c_str()
            );
            lv_label_set_text(guider_ui->screen_label_now_weather_label, weather_str);
            if (guider_ui->screen_img_now_weather_icon) {
                char path_buf[64];
                snprintf(path_buf, sizeof(path_buf), "S:/icons/%s.bin", weather.icon_code.c_str());
                lv_image_set_src(guider_ui->screen_img_now_weather_icon, path_buf);
            }
        }
    }
}
