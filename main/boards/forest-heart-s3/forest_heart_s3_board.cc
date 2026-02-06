#include "wifi_board.h"
#include "codecs/es8311_audio_codec.h"
#include "display/epd_display.h"
#include "application.h"
#include "i2c_device.h"
#include "config.h"
#include "axp2101.h"
#include <esp_log.h>
#include <driver/i2c_master.h>
#include "driver/gpio.h"
#include <driver/spi_master.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_timer.h>
#include "tca9537.h"

// 引入墨水屏底层驱动头文件
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
#define TCA9537_RST_GPIO    12    // TCA9537 的 RST 复位引脚 (假设是 GPIO 12，请修改!)
#define TCA9537_REG_OUTPUT  0x01  // 输出端口寄存器
#define TCA9537_REG_CONFIG  0x03  // 配置寄存器

// 确保在 config.h 中定义了分辨率，或者在这里硬编码
#ifndef DISPLAY_WIDTH
#define DISPLAY_WIDTH 400
#endif
#ifndef DISPLAY_HEIGHT
#define DISPLAY_HEIGHT 300
#endif

class Pmic : public Axp2101 {
public:
    // Power Init
    Pmic(i2c_master_bus_handle_t i2c_bus, uint8_t addr) : Axp2101(i2c_bus, addr) {
            WriteReg(0x22, 0b110); // PWRON > OFFLEVEL as POWEROFF Source enable
            WriteReg(0x27, 0x10);  // hold 4s to power off
    
            // Disable All DCs but DC1
            WriteReg(0x80, 0x01);
            // Disable All LDOs
            WriteReg(0x90, 0x00);
            WriteReg(0x91, 0x00);
    
            // Set DC1 to 3.3V
            WriteReg(0x82, (3300 - 1500) / 100);
    
            // Set ALDO1 to 3.3V
            WriteReg(0x92, (3300 - 500) / 100);

            WriteReg(0x96, (1500 - 500) / 100);
            WriteReg(0x97, (2800 - 500) / 100);
    
            // Enable ALDO1 BLDO1 BLDO2 
            WriteReg(0x90, 0x31);
            
            WriteReg(0x64, 0x02); // CV charger voltage setting to 4.1V
            WriteReg(0x61, 0x02); // set Main battery precharge current to 50mA
            WriteReg(0x62, 0x08); // set Main battery charger current to 400mA ( 0x08-200mA, 0x09-300mA, 0x0A-400mA )
            WriteReg(0x63, 0x01); // set Main battery term charge current to 25mA
    }
};

class IoExpander : public Tca9537 {
public:
    // 构造函数：在这里完成初始化配置
    IoExpander(i2c_master_bus_handle_t i2c_bus, uint8_t addr) : Tca9537(i2c_bus, addr){
        ESP_LOGI(TAG, "Initializing IoExpander (TCA9537)...");
        gpio_set_direction(GPIO_NUM_12, GPIO_MODE_OUTPUT);
        gpio_set_level(GPIO_NUM_12, 1);
        vTaskDelay(20);
        uint8_t output_val = 0x06;
        uint8_t config_val = 0x01;
        WriteReg(0x01, output_val);
        WriteReg(0x03, config_val);
        ESP_LOGI(TAG, "Init Complete: Speaker ON(P1=H), EPD RST(P2=H), EPD CS(P3=L)");
    }
};

class ForestHeartS3 : public WifiBoard {
private:
    i2c_master_bus_handle_t i2c_bus_;
    Pmic* pmic_;
    Display* display_ = nullptr; // 使用基类指针，指向 EpdDisplay 实例
    IoExpander* ioexpander_ = nullptr;
    // 初始化 I2C 总线
    void InitializeI2c() {
        i2c_master_bus_config_t i2c_bus_cfg = {
            .i2c_port = I2C_NUM_0, 
            .sda_io_num = I2C_SDA_PIN, // 确保 config.h 定义了这些
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

    void InitializeAxp2101() {
        ESP_LOGI(TAG, "Init AXP2101");
        pmic_ = new Pmic(i2c_bus_, 0x34);
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    void EpdHwPreInit() {
        ESP_LOGI(TAG, "EPD Hardware Config");
        ioexpander_ = new IoExpander(i2c_bus_, 0x49);
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
            .max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT / 8 + 100, // 400x300 1bpp
        };
        ESP_ERROR_CHECK(spi_bus_initialize(EPD_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));
    }

    // 初始化墨水屏并创建 Display 对象
    void InitializeGDEW042T2Display() {
        esp_lcd_panel_io_handle_t panel_io = nullptr;
        esp_lcd_panel_handle_t panel = nullptr;

        ESP_LOGI(TAG, "Install Panel IO");
        
        // CS 引脚设为 -1，因为已经由 TCA9537 锁定为低电平
        const esp_lcd_panel_io_spi_config_t io_config = {
            .cs_gpio_num = -1,
            .dc_gpio_num = PIN_NUM_EPD_DC,
            .spi_mode = 0,
            .pclk_hz = 10 * 1000 * 1000, // EPD 推荐 10MHz
            .trans_queue_depth = 10,
            .lcd_cmd_bits = 8,
            .lcd_param_bits = 8,
        };
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(EPD_SPI_HOST, &io_config, &panel_io));

        ESP_LOGI(TAG, "Install GDEW042T2 Panel Driver");

        // 配置 EPD 特有的 Busy 引脚
        esp_lcd_panel_gdew042t2_config_t vendor_config = {
            .busy_gpio_num = PIN_NUM_EPD_BUSY,
            .busy_active_level = 0, // 0: Low = Busy, 1: High = Busy (请根据数据手册确认，通常是 0)
        };

        const esp_lcd_panel_dev_config_t panel_config = {
            .reset_gpio_num = -1, // 复位已由 TCA9537 处理
            .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
            .bits_per_pixel = 1,  // 1 bit per pixel
            .vendor_config = &vendor_config,
        };
        
        ESP_ERROR_CHECK(esp_lcd_new_panel_gdew042t2(panel_io, &panel_config, &panel));

        // 执行初始化序列 (复位 -> 初始化命令)
        esp_lcd_panel_reset(panel); 
        esp_lcd_panel_init(panel);
        
        // 默认设置为全刷模式，显示更清晰，后续由 EpdDisplay 管理模式切换
        esp_lcd_gdew042t2_set_mode(panel, GDEW042T2_REFRESH_FULL); 

        ESP_LOGI(TAG, "Instantiating EpdDisplay");
        // <--- 修改点 2: 实例化你写的 EpdDisplay --->
        // EpdDisplay 构造函数: (panel_io, panel, width, height)
        display_ = new EpdDisplay(panel_io, panel, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    }

    void I2cDetect() {
        uint8_t address;
        printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\r\n");
        for (int i = 0; i < 128; i += 16) {
            printf("%02x: ", i);
            for (int j = 0; j < 16; j++) {
                fflush(stdout);
                address = i + j;
                esp_err_t ret = i2c_master_probe(i2c_bus_, address, pdMS_TO_TICKS(200));
                if (ret == ESP_OK) {
                    printf("%02x ", address);
                } else if (ret == ESP_ERR_TIMEOUT) {
                    printf("UU ");
                } else {
                    printf("-- ");
                }
            }
            printf("\r\n");
        }
    }

public:
    ForestHeartS3(){
        // 1. 初始化 I2C 总线 (用于 TCA9537)
        InitializeI2c();
        
        InitializeAxp2101();
        I2cDetect();
        // 2. EPD 硬件预处理 (控制 TCA9537 拉低 CS，拉高 RST)
        EpdHwPreInit();

        // 3. SPI 初始化
        InitializeSpi();

        // 4. 墨水屏初始化 & UI 启动
        InitializeGDEW042T2Display();
    }

    virtual AudioCodec* GetAudioCodec() override {
        static Es8311AudioCodec audio_codec(i2c_bus_, I2C_NUM_0, AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_GPIO_MCLK, AUDIO_I2S_GPIO_BCLK, AUDIO_I2S_GPIO_WS, AUDIO_I2S_GPIO_DOUT, AUDIO_I2S_GPIO_DIN,
            GPIO_NUM_NC, AUDIO_CODEC_ES8311_ADDR);
        return &audio_codec;
    }

    virtual Display* GetDisplay() override {
        return display_;
    }
};

DECLARE_BOARD(ForestHeartS3);