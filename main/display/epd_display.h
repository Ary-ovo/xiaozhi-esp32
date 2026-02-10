#ifndef EPD_DISPLAY_H
#define EPD_DISPLAY_H

#include "lvgl_display.h"
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_timer.h>
#include <memory>

// 定义全刷新的阈值 (每更新几次文字执行一次全刷去残影)
#define EPD_FULL_REFRESH_EVERY_X_FRAMES 10

class EpdDisplay : public LvglDisplay {
private:
    esp_lcd_panel_io_handle_t panel_io_ = nullptr;
    esp_lcd_panel_handle_t panel_ = nullptr;
    lv_display_t* display_ = nullptr;
    
    int width_ = 0;
    int height_ = 0;

    // ==========================================
    // 双缓冲机制成员
    // ==========================================
    // Buffer 1: LVGL 绘图区 (一级缓存)
    void* lvgl_buf_ = nullptr;
    
    // Buffer 2: 硬件发送快照区 (二级缓存)
    void* snapshot_buf_ = nullptr;

    // Buffer 3: 【新增】LVGL 内部绘图缓存 (Partial)
    void* draw_buf_ = nullptr;
    
    // 脏标记：记录是否有新的内容需要刷新
    volatile bool is_dirty_ = false;

    // 定时器句柄
    esp_timer_handle_t refresh_timer_ = nullptr;

    // 刷新计数器 (用于触发定期全刷)
    int update_counter_ = 0;

    // ==========================================
    // UI 对象成员 (修复 top_bar_ 未定义错误)
    // ==========================================
    lv_obj_t* container_ = nullptr;
    lv_obj_t* top_bar_ = nullptr;       // 顶部状态栏
    lv_obj_t* chat_message_label_ = nullptr; // 底部文字区域
    lv_obj_t* emotion_label_ = nullptr; // 中间表情

    // 状态栏图标
    lv_obj_t* network_label_ = nullptr;
    lv_obj_t* battery_label_ = nullptr;
    lv_obj_t* mute_label_ = nullptr;

    // ==========================================
    // 内部函数
    // ==========================================
    void SetupUI();

    // 真正的硬件刷新函数 (由定时器触发)
    void FlushToHardware();

    // 触发全刷清理残影的辅助函数
    void TriggerFullRefresh();

    // "假"的刷新回调，只做标记
    static void FlushCallback(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map);

    virtual bool Lock(int timeout_ms = 0) override;
    virtual void Unlock() override;

public:
    /**
     * @brief 构造函数
     * @param panel_io 硬件 IO 句柄
     * @param panel 硬件 Panel 句柄
     * @param width 宽度 (400)
     * @param height 高度 (300)
     */
    EpdDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel, int width, int height);
    
    ~EpdDisplay();

    virtual void SetChatMessage(const char* role, const char* content) override;
    virtual void SetEmotion(const char* emotion) override;
    virtual void SetTheme(Theme* theme) override;
    virtual void SetPowerSaveMode(bool enabled);
};

#endif // EPD_DISPLAY_H