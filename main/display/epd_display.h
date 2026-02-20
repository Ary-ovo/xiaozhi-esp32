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
    int last_battery_level_ = 0;


    QueueHandle_t display_queue_ = nullptr;
    int update_counter_ = 0;
    int64_t last_refresh_time_ = 0; // 上一次刷新发送的时间 (us)
    // 定义发送给显示线程的命令
    struct EpdCmd {
        bool force_full_refresh; // 是否强制全刷（可选）
    };
    void* lvgl_buf_ = nullptr;
    void* snapshot_buf_ = nullptr;
    void* draw_buf_ = nullptr;
    volatile bool is_dirty_ = false;

    esp_timer_handle_t refresh_timer_ = nullptr;


    void SetupUI();
    void InitAsync();
    
    // "假"的刷新回调，只做标记
    static void FlushCallback(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map);
    static void DisplayTask(void* arg);

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

    void UpdateWeatherUI();
    void FlushToHardware();
    void TriggerFullRefresh();
    void UpdateDateDisplay();
    
    virtual void SetChatMessage(const char* role, const char* content) override;
    virtual void SetEmotion(const char* emotion) override;
    virtual void SetStatus(const char* status);
    virtual void UpdateStatusBar(bool update_all);
};

#endif // EPD_DISPLAY_H