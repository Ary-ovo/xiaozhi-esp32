#ifndef SDMMC_H
#define SDMMC_H

#include <string>
#include "driver/gpio.h"
#include "driver/sdmmc_host.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include <dirent.h>

class SdCard {
public:
    /**
     * @brief 构造函数：完全参数化初始化 (类似 AudioCodec 风格)
     * * @param mount_point 挂载点 (例如 "/sdcard")
     * @param clk  时钟引脚
     * @param cmd  指令引脚
     * @param d0   数据引脚 0
     * @param d1   数据引脚 1 (如果使用 1线模式，传 GPIO_NUM_NC)
     * @param d2   数据引脚 2 (如果使用 1线模式，传 GPIO_NUM_NC)
     * @param d3   数据引脚 3 (如果使用 1线模式，传 GPIO_NUM_NC)
     * @param format_if_mount_failed 如果挂载失败是否格式化 (默认 false)
     */
    SdCard(const char* mount_point, 
           gpio_num_t clk, 
           gpio_num_t cmd, 
           gpio_num_t d0, 
           gpio_num_t d1 = GPIO_NUM_NC, 
           gpio_num_t d2 = GPIO_NUM_NC, 
           gpio_num_t d3 = GPIO_NUM_NC,
           bool format_if_mount_failed = false);

    virtual ~SdCard();

    // ============================================================
    // 公有接口
    // ============================================================
    bool IsMounted() const;

    // 文件操作接口
    bool WriteFile(const std::string& path, const std::string& data);
    std::string ReadFile(const std::string& path);
    bool FileExists(const std::string& path);
private:
    std::string mount_point_;
    sdmmc_card_t* card_ = nullptr;
    bool is_mounted_ = false;
    
    // 内部使用的辅助函数
    void LogError(const char* msg, esp_err_t err);
};

#endif // SDMMC_H