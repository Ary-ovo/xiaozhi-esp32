#include "wifi_board.h"
#include "codecs/no_audio_codec.h"
#include "display/lcd_display.h" // 如果你有专门的 EpdDisplay 封装最好，否则沿用 LcdDisplay
#include "application.h"
#include "i2c_device.h"
#include "config.h"

#include <esp_log.h>
#include <driver/i2c_master.h>
#include <driver/ledc.h>
#include <driver/spi_master.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_timer.h>

// 引入墨水屏驱动头文件
#include "esp_lcd_gdew042t2.h"

#define TAG "ForestHeartS3"

#define EPD_SPI_HOST        SPI2_HOST
#define PIN_NUM_EPD_MOSI    10
#define PIN_NUM_EPD_CLK     9
#define PIN_NUM_EPD_DC      7
#define PIN_NUM_EPD_BUSY    6

// --- TCA9537 I2C 扩展引脚定义 ---
#define TCA9537_ADDR        0x49
#define TCA_PORT_EPD_CS     3
#define TCA_PORT_EPD_RES    2

class ForestHeartS3 : public WifiBoard {
private:
    i2c_master_bus_handle_t i2c_bus_;
    LcdDisplay* display_; // 注意：墨水屏刷新慢，建议后续改用适配 EPD 的 Display 实现

    // TCA9537 寄存器写入辅助函数
    void Tca9537WriteReg(uint8_t reg, uint8_t val) {
        uint8_t write_buf[2] = {reg, val};
        i2c_device_config_t dev_cfg = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = TCA9537_ADDR,
            .scl_speed_hz = 400000,
        };
        i2c_master_dev_handle_t dev_handle;
        ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus_, &dev_cfg, &dev_handle));
        ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, write_buf, sizeof(write_buf), -1));
        i2c_master_bus_rm_device(dev_handle);
    }

    // 初始化 I2C 总线
    void InitializeI2c() {
        i2c_master_bus_config_t i2c_bus_cfg = {
            .i2c_port = (i2c_port_t)0, 
            .sda_io_num = I2C_SDA_PIN, 
            .scl_io_num = I2C_SCL_PIN, 
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .intr_priority = 0,
            .trans_queue_depth = 0,
            .flags = {
                .enable_internal_pullup = 1,
            },
        };
        ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_cfg, &i2c_bus_));
    }

    // 墨水屏硬件预处理：通过 I2C 扩展芯片拉低 CS，拉高 RES
    void EpdHwPreInit() {
        ESP_LOGI(TAG, "Initializing TCA9537 for EPD CS/RST...");
        
        // 1. 复位 TCA9537 (GPIO 12)
        gpio_reset_pin((gpio_num_t)12);
        gpio_set_direction((gpio_num_t)12, GPIO_MODE_OUTPUT);
        gpio_set_level((gpio_num_t)12, 0);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level((gpio_num_t)12, 1);
        vTaskDelay(pdMS_TO_TICKS(50));

        // 2. 配置扩展引脚
        // Config Reg (0x03): Set P2(CS) and P3(RES) to Output (0)
        // 假设其他引脚保持输入 (1) -> 1111 0011 = 0xF3
        uint8_t config_val = 0xFF & ~((1 << TCA_PORT_EPD_CS) | (1 << TCA_PORT_EPD_RES));
        Tca9537WriteReg(0x03, config_val);

        // Output Reg (0x01): Set CS=Low(0), RES=High(1)
        uint8_t output_val = (0 << TCA_PORT_EPD_CS) | (1 << TCA_PORT_EPD_RES);
        Tca9537WriteReg(0x01, output_val);
        
        ESP_LOGI(TAG, "EPD CS locked LOW, RST pulled HIGH via I2C");
    }

    // 初始化 SPI 总线 (标准 SPI)
    void InitializeSpi() {
        ESP_LOGI(TAG, "Initialize SPI bus for EPD");

        spi_bus_config_t buscfg = {
            .mosi_io_num = PIN_NUM_EPD_MOSI,
            .miso_io_num = -1,
            .sclk_io_num = PIN_NUM_EPD_CLK,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = 400 * 300 / 8 + 100, // 400x300 1bpp
        };
        ESP_ERROR_CHECK(spi_bus_initialize(EPD_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));
    }

    // 初始化墨水屏
    void InitializeGDEW042T2Display() {
        esp_lcd_panel_io_handle_t panel_io = nullptr;
        esp_lcd_panel_handle_t panel = nullptr;

        ESP_LOGI(TAG, "Install Panel IO");
        
        // CS 引脚设为 -1，因为已经由 TCA9537 锁定为低电平
        const esp_lcd_panel_io_spi_config_t io_config = {
            .cs_gpio_num = -1,
            .dc_gpio_num = PIN_NUM_EPD_DC,
            .spi_mode = 0,
            .pclk_hz = 10 * 1000 * 1000, // EPD 速度较低，推荐 4MHz-10MHz
            .trans_queue_depth = 10,
            .lcd_cmd_bits = 8,
            .lcd_param_bits = 8,
        };
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(EPD_SPI_HOST, &io_config, &panel_io));

        ESP_LOGI(TAG, "Install GDEW042T2 Panel Driver");

        // 配置 EPD 特有的 Busy 引脚
        esp_lcd_panel_gdew042t2_config_t vendor_config = {
            .busy_gpio_num = PIN_NUM_EPD_BUSY,
            .busy_active_level = 0, // Low = Busy
        };

        const esp_lcd_panel_dev_config_t panel_config = {
            .reset_gpio_num = -1, // 复位已由 TCA9537 处理
            .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
            .bits_per_pixel = 1,  // 1 bit per pixel
            .vendor_config = &vendor_config,
        };
        
        ESP_ERROR_CHECK(esp_lcd_new_panel_gdew042t2(panel_io, &panel_config, &panel));

        // 执行初始化序列
        esp_lcd_panel_reset(panel); // 软件逻辑复位
        esp_lcd_panel_init(panel);
        
        // 默认设置为全刷模式，显示更清晰
        esp_lcd_gdew042t2_set_mode(panel, GDEW042T2_REFRESH_FULL); 

        // 实例化 Display 对象
        // 注意：DISPLAY_WIDTH/HEIGHT 需在 config.h 中改为 400/300
        // SpiLcdDisplay 类可能需要修改以适应 1bpp 数据格式，但如果是基于 draw_bitmap 的，基本能用
        display_ = new SpiLcdDisplay(panel_io, panel,
                                    DISPLAY_WIDTH, DISPLAY_HEIGHT, 
                                    0, 0, // Offset 通常为 0
                                    false, false, false); // EPD 通常不需要 Swap XY 或 Mirror，除非物理安装旋转
    }

public:
    ForestHeartS3() {
        // 1. 初始化 I2C 总线
        InitializeI2c();
        
        // 2. 触摸屏初始化 (需要在 I2C 之后)
        // InitializeCst816sTouchPad(); 
        
        // 3. EPD 硬件预处理 (控制 TCA9537)
        EpdHwPreInit();

        // 4. SPI 初始化
        InitializeSpi();

        // 5. 墨水屏初始化
        InitializeGDEW042T2Display();
        
    }

    virtual AudioCodec* GetAudioCodec() override {
        // ... 保持原有音频逻辑 ...
        return nullptr; // 简化示例
    }

    virtual Display* GetDisplay() override {
        return display_;
    }

};

DECLARE_BOARD(ForestHeartS3);