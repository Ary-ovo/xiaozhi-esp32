#include "sdmmc.h"
#include <esp_log.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include "lvgl.h"
#include <string>
// 必须包含这两个头文件才能定义 mount_config_t
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"

static const char* TAG = "SdCard";

SdCard::SdCard(const char* mount_point, 
               gpio_num_t clk, gpio_num_t cmd, gpio_num_t d0, 
               gpio_num_t d1, gpio_num_t d2, gpio_num_t d3,
               bool format_if_mount_failed)
    : mount_point_(mount_point) {

    ESP_LOGI(TAG, "Initializing SDMMC...");

    esp_err_t ret;

    // 1. 挂载配置
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = format_if_mount_failed,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    // 2. 主机配置 (默认使用 SDMMC 外设)
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    // host.max_freq_khz = SDMMC_FREQ_HIGHSPEED; // 如果 PCB 走线好，可以开 40MHz

    // 3. 卡槽配置 (关键：使用传入的引脚参数)
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    
    // 自动判断位宽：如果 d1 是 NC，说明是 1-bit 模式，否则是 4-bit
    if (d1 == GPIO_NUM_NC) {
        slot_config.width = 1;
        ESP_LOGI(TAG, "Configuring in 1-bit mode");
    } else {
        slot_config.width = 4;
        ESP_LOGI(TAG, "Configuring in 4-bit mode");
    }

    // 设置 GPIO Matrix
    slot_config.clk = clk;
    slot_config.cmd = cmd;
    slot_config.d0  = d0;
    
    // 只有 4-bit 模式才配置 d1-d3
    if (slot_config.width == 4) {
        slot_config.d1 = d1;
        slot_config.d2 = d2;
        slot_config.d3 = d3;
    }

    // 启用内部上拉 (仍然建议硬件接 10k 上拉)
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    // 4. 执行挂载
    ret = esp_vfs_fat_sdmmc_mount(mount_point_.c_str(), &host, &slot_config, &mount_config, &card_);

    if (ret != ESP_OK) {
        LogError("Failed to mount filesystem", ret);
        is_mounted_ = false;
        return;
    }

    is_mounted_ = true;
    ESP_LOGI(TAG, "SD Card mounted at %s", mount_point_.c_str());
    sdmmc_card_print_info(stdout, card_);
}

SdCard::~SdCard() {
    if (is_mounted_) {
        esp_vfs_fat_sdcard_unmount(mount_point_.c_str(), card_);
        ESP_LOGI(TAG, "Card unmounted");
        is_mounted_ = false;
        card_ = nullptr;
    }
}

// ============================================================================
// 功能函数
// ============================================================================

bool SdCard::IsMounted() const {
    return is_mounted_;
}

void SdCard::LogError(const char* msg, esp_err_t err) {
    if (err == ESP_FAIL) {
        ESP_LOGE(TAG, "%s. Mount failed.", msg);
    } else {
        ESP_LOGE(TAG, "%s. Error: %s", msg, esp_err_to_name(err));
    }
}

bool SdCard::WriteFile(const std::string& path, const std::string& data) {
    if (!is_mounted_) return false;
    std::string full_path = mount_point_ + "/" + path;
    
    FILE* f = fopen(full_path.c_str(), "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing: %s", full_path.c_str());
        return false;
    }
    fprintf(f, "%s", data.c_str());
    fclose(f);
    return true;
}

std::string SdCard::ReadFile(const std::string& path) {
    if (!is_mounted_) return "";
    std::string full_path = mount_point_ + "/" + path;
    
    std::ifstream file(full_path);
    if (!file.is_open()) {
        ESP_LOGE(TAG, "Failed to open file for reading: %s", full_path.c_str());
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool SdCard::FileExists(const std::string& path) {
    if (!is_mounted_) return false;
    struct stat st;
    std::string full_path = mount_point_ + "/" + path;
    return stat(full_path.c_str(), &st) == 0;
}