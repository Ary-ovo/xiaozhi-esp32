#include "epd_display.h"
#include "assets/lang_config.h"
#include "lvgl_theme.h"
#include "lvgl_font.h"
#include "esp_lcd_gdew042t2.h"
#include "esp_heap_caps.h" 

#include <string>
#include <algorithm>
#include <cstring>
#include <esp_log.h>
#include <esp_err.h>
#include <esp_lvgl_port.h>
#include <font_awesome.h>

// #ifdef CONFIG_BOARD_TYPE_FOREST_HEART_S3
//     #include "gui_guider.h"
//     lv_ui guider_ui;
// #endif // CONFIG_BOARD_TYPE_FOREST_HEART_S3

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

// ============================================================================
// Helper: Set 1bpp pixel with bounds checking
// ============================================================================
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

// ============================================================================
// 1. Flush Callback (RGB565 -> 1bpp Conversion)
// ============================================================================
void EpdDisplay::FlushCallback(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map)
{
    EpdDisplay* self = (EpdDisplay*)lv_display_get_user_data(disp);
    if (!self) {
        lv_display_flush_ready(disp);
        return;
    }

    int32_t w = area->x2 - area->x1 + 1;
    int32_t h = area->y2 - area->y1 + 1;

    // SOURCE: RGB565 (16-bit) because menuconfig is set to 16-bit
    uint16_t* src_pixels = (uint16_t*)px_map;

    // We iterate through the partial update area provided by LVGL
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            // Read RGB565 color (0x0000 - 0xFFFF)
            uint16_t color = src_pixels[y * w + x];
            
            // Simple Binarization: Check brightness
            // > 0x8000 (approx 50% gray) is White, else Black
            bool is_white = (color > 0x8000); 

            // Write to our full-screen 1bpp buffer in PSRAM
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

// ============================================================================
// 2. Hardware Flush (PSRAM -> Internal RAM DMA)
// ============================================================================
void EpdDisplay::FlushToHardware()
{
    if (!is_dirty_) return;

    ESP_LOGD(TAG, "Flushing to E-Ink (M01)...");

    // Step A: Copy from PSRAM shadow buffer to Internal DMA buffer
    {
        DisplayLockGuard lock(this);
        // Copy 1bpp data. 
        // snapshot_buf_ has extra padding so this memcpy is strictly safe.
        memcpy(snapshot_buf_, lvgl_buf_, (width_ * height_) / 8);
        is_dirty_ = false;
    }

    // Step B: Set Refresh Mode (Switching Logic for M01)
    if (update_counter_ >= EPD_FULL_REFRESH_EVERY_X_FRAMES) {
         // --- 全刷模式 (Full Refresh) ---
         // false = 关闭局刷 = 使用全刷 (OTP LUTs, 慢但清晰)
         esp_lcd_gdew042t2_set_mode(panel_, GDEW042T2_REFRESH_FULL);
         
         update_counter_ = 0;
    } else {
         // --- 局刷模式 (Partial Refresh) ---
         // true = 开启局刷 (Custom LUTs, 快但有残影)
         esp_lcd_gdew042t2_set_mode(panel_, GDEW042T2_REFRESH_PARTIAL);
         
         // 注意：建议在这里增加计数器，否则全刷逻辑可能永远触发不了
         // update_counter_++; 
    }

    // Step C: Send the Internal RAM buffer via DMA
    // 标准 API，不需要改动
    esp_lcd_panel_draw_bitmap(panel_, 0, 0, width_, height_, snapshot_buf_);
}

// // Helper Functions
// void EpdDisplay::TriggerFullRefresh() {
//     update_counter_ = EPD_FULL_REFRESH_EVERY_X_FRAMES + 1;
//     is_dirty_ = true;
// }

// void EpdDisplay::SetChatMessage(const char* role, const char* content) {
//     DisplayLockGuard lock(this);
//     if (!chat_message_label_) return;
//     lv_label_set_text(chat_message_label_, content);
//     update_counter_++;
//     if (role && strcmp(role, "system") == 0) TriggerFullRefresh();
// }

// void EpdDisplay::SetEmotion(const char* emotion) {
//     DisplayLockGuard lock(this);
//     if (!emotion_label_) return;
//     const char* icon = font_awesome_get_utf8(emotion);
//     lv_label_set_text(emotion_label_, icon ? icon : FONT_AWESOME_NEUTRAL);
// }

// void EpdDisplay::SetTheme(Theme* theme) {
//     DisplayLockGuard lock(this);
//     auto lvgl_theme = static_cast<LvglTheme*>(theme);
//     if (emotion_label_ && lvgl_theme) {
//          lv_obj_set_style_text_font(emotion_label_, lvgl_theme->large_icon_font()->font(), 0);
//     }
// }

// void EpdDisplay::SetPowerSaveMode(bool enabled) {
//     DisplayLockGuard lock(this);
    
//     // 如果没有初始化 panel_handle_，直接返回
//     if (!panel_) return;

//     if (enabled) {
//         ESP_LOGI(TAG, "EPD Enter Sleep");
//         // 墨水屏唤醒后通常需要一次全刷来清除残影或重置电压状态
//         TriggerFullRefresh();
//         FlushToHardware();
//         esp_lcd_panel_disp_on_off(panel_, false);
        
//     } else {
//         ESP_LOGI(TAG, "EPD Exit Sleep");
//         esp_lcd_panel_disp_on_off(panel_, true);
//         // 墨水屏唤醒后通常需要一次全刷来清除残影或重置电压状态
//         TriggerFullRefresh();
//         FlushToHardware();
//     }
// }

// LOCKING: Must be blocking to prevent race conditions
bool EpdDisplay::Lock(int timeout_ms) {
    if (timeout_ms == 0) timeout_ms = 1000;
    return lvgl_port_lock(timeout_ms);
}

void EpdDisplay::Unlock() { 
    lvgl_port_unlock(); 
}

// ============================================================================
// 5. Constructor & Destructor
// ============================================================================
EpdDisplay::EpdDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel,
                       int width, int height)
    : panel_io_(panel_io), panel_(panel), width_(width), height_(height)
{
    // 1. Theme Init
    auto text_font = std::make_shared<LvglBuiltInFont>(&BUILTIN_TEXT_FONT);
    auto icon_font = std::make_shared<LvglBuiltInFont>(&BUILTIN_ICON_FONT);
    auto large_icon_font = std::make_shared<LvglBuiltInFont>(&font_awesome_30_1);
    auto theme = new LvglTheme("epd");
    theme->set_text_font(text_font);
    theme->set_icon_font(icon_font);
    theme->set_large_icon_font(large_icon_font);
    LvglThemeManager::GetInstance().RegisterTheme("epd", theme);
    current_theme_ = theme;

    // 2. LVGL Port Init
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
    
    // ============================================================
    // [CRITICAL FIX] Memory Allocation Strategy
    // ============================================================
    
    // 1. Calculate Sizes
    uint32_t full_1bpp_size = (width * height) / 8; // ~15KB
    
    // LVGL Partial Buffer (RGB565). 
    // IMPORTANT: We must allocate size for 16-bit pixels (2 bytes), 
    // otherwise LVGL writes past the end and crashes Wi-Fi.
    uint32_t partial_height = height / 10;
    uint32_t draw_buf_size = width * partial_height * sizeof(uint16_t); // ~24KB

    // 2. Allocate Buffers
    // Use PSRAM for the large drawing buffers to save internal RAM
    // Add 4KB padding to act as a "safety airbag" against overflows
    draw_buf_ = heap_caps_malloc(draw_buf_size, MALLOC_CAP_SPIRAM);
    lvgl_buf_ = heap_caps_malloc(full_1bpp_size, MALLOC_CAP_SPIRAM);
    
    // Use INTERNAL RAM for the hardware snapshot.
    // [CRITICAL]: We OVER-ALLOCATE (32KB instead of 15KB) to guarantee
    // that even if a stray pointer writes past the end, it hits our padding,
    // not the Wi-Fi stack.
    uint32_t safe_snapshot_size = 32 * 1024; 
    snapshot_buf_ = heap_caps_calloc(1, safe_snapshot_size, MALLOC_CAP_SPIRAM);

    if (!lvgl_buf_ || !snapshot_buf_ || !draw_buf_) {
        ESP_LOGE(TAG, "Failed to allocate buffers!");
        lvgl_port_unlock();
        return;
    }
    
    // Clear buffers
    memset(lvgl_buf_, 0xFF, full_1bpp_size);
    // snapshot_buf_ is already zeroed by calloc, but we set it white (0xFF)
    memset(snapshot_buf_, 0xFF, safe_snapshot_size);
    
    // 4. Configure Display
    // [CRITICAL]: We MUST tell LVGL we are RGB565 to match menuconfig.
    lv_display_set_color_format(display_, LV_COLOR_FORMAT_RGB565);
    lv_display_set_user_data(display_, this);
    lv_display_set_flush_cb(display_, FlushCallback);

    // Set buffers. Note: We use Partial Mode.
    lv_display_set_buffers(display_, draw_buf_, NULL, draw_buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);

    esp_lcd_gdew042t2_set_mode(panel_, GDEW042T2_REFRESH_PARTIAL);
    lvgl_port_unlock();

    SetupUI();

    // 5. Start Refresh Timer
    const esp_timer_create_args_t timer_args = {
        .callback = [](void *arg) { static_cast<EpdDisplay*>(arg)->FlushToHardware(); },
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "epd_refresh"
    };
    esp_timer_create(&timer_args, &refresh_timer_);
    esp_timer_start_periodic(refresh_timer_, 200000);
}

EpdDisplay::~EpdDisplay() {
    if (refresh_timer_) { esp_timer_stop(refresh_timer_); esp_timer_delete(refresh_timer_); }
    if (display_) lv_display_delete(display_);
    
    if (lvgl_buf_) free(lvgl_buf_);
    if (snapshot_buf_) free(snapshot_buf_);
    if (draw_buf_) free(draw_buf_);

    if (container_) lv_obj_del(container_);
    if (panel_) esp_lcd_panel_del(panel_);
    if (panel_io_) esp_lcd_panel_io_del(panel_io_);
}

// SetupUI
void EpdDisplay::SetupUI() {
    DisplayLockGuard lock(this);
    // setup_scr_screen(&guider_ui);
    // auto lvgl_theme = static_cast<LvglTheme*>(current_theme_);
    // auto text_font = lvgl_theme->text_font()->font();
    // auto icon_font = lvgl_theme->icon_font()->font();
    // auto large_icon_font = lvgl_theme->large_icon_font()->font();

    // auto screen = lv_screen_active();
    // lv_obj_set_style_text_font(screen, text_font, 0);
    // lv_obj_set_style_text_color(screen, lv_color_black(), 0);

    // container_ = lv_obj_create(screen);
    // lv_obj_set_size(container_, width_, height_);
    // lv_obj_set_style_pad_all(container_, 0, 0);
    // lv_obj_set_style_border_width(container_, 0, 0);
    // lv_obj_set_flex_flow(container_, LV_FLEX_FLOW_COLUMN);
    // lv_obj_set_style_bg_opa(container_, LV_OPA_TRANSP, 0);

    // top_bar_ = lv_obj_create(container_);
    // lv_obj_set_size(top_bar_, width_, 30);
    // lv_obj_set_style_pad_hor(top_bar_, 10, 0);
    // lv_obj_set_style_border_side(top_bar_, LV_BORDER_SIDE_BOTTOM, 0);
    // lv_obj_set_style_border_width(top_bar_, 2, 0);
    // lv_obj_set_style_border_color(top_bar_, lv_color_black(), 0);
    // lv_obj_set_flex_flow(top_bar_, LV_FLEX_FLOW_ROW);
    // lv_obj_set_flex_align(top_bar_, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // network_label_ = lv_label_create(top_bar_);
    // lv_obj_set_style_text_font(network_label_, icon_font, 0);
    // lv_label_set_text(network_label_, LV_SYMBOL_WIFI); 
    // lv_obj_set_style_text_color(network_label_, lv_color_black(), 0);

    // lv_obj_t* right_box = lv_obj_create(top_bar_);
    // lv_obj_set_size(right_box, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    // lv_obj_set_flex_flow(right_box, LV_FLEX_FLOW_ROW);
    // lv_obj_set_style_pad_gap(right_box, 10, 0);

    // mute_label_ = lv_label_create(right_box);
    // lv_label_set_text(mute_label_, ""); 
    
    // battery_label_ = lv_label_create(right_box);
    // lv_obj_set_style_text_font(battery_label_, icon_font, 0);
    // lv_label_set_text(battery_label_, LV_SYMBOL_BATTERY_FULL);

    // lv_obj_t* emotion_box = lv_obj_create(container_);
    // lv_obj_set_width(emotion_box, width_);
    // lv_obj_set_flex_grow(emotion_box, 1);
    // lv_obj_set_flex_flow(emotion_box, LV_FLEX_FLOW_COLUMN);
    // lv_obj_set_flex_align(emotion_box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // emotion_label_ = lv_label_create(emotion_box);
    // lv_obj_set_style_text_font(emotion_label_, large_icon_font, 0);
    // lv_label_set_text(emotion_label_, FONT_AWESOME_MICROCHIP_AI);
    // lv_obj_set_style_text_color(emotion_label_, lv_color_black(), 0);

    // lv_obj_t* msg_box = lv_obj_create(container_);
    // lv_obj_set_size(msg_box, width_, 80);
    // lv_obj_set_style_pad_all(msg_box, 10, 0);
    // lv_obj_set_style_border_side(msg_box, LV_BORDER_SIDE_TOP, 0);
    // lv_obj_set_style_border_width(msg_box, 2, 0);
    // lv_obj_set_style_border_color(msg_box, lv_color_black(), 0);

    // chat_message_label_ = lv_label_create(msg_box);
    // lv_obj_set_width(chat_message_label_, width_ - 20);
    // lv_label_set_long_mode(chat_message_label_, LV_LABEL_LONG_WRAP);
    // lv_obj_set_style_text_align(chat_message_label_, LV_TEXT_ALIGN_CENTER, 0);
    // lv_label_set_text(chat_message_label_, "Xiaozhi E-Paper Ready");

}
