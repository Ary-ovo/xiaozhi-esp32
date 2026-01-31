#include "wifi_board.h"
#include "codecs/es8311_audio_codec.h"
#include "display/lcd_display.h"
#include "application.h"
#include "i2c_device.h"
#include "config.h"

#include <esp_log.h>
#include <driver/i2c_master.h>
#include <driver/gpio.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_timer.h>

// 引入自定义墨水屏驱动
#include "esp_lcd_wft042.h"

class ForestHeartS3 : public WifiBoard {
 public:
  ForestHeartS3() {
    // 1. 初始化 I2C 总线 (Codec, PMU, IO Expander 共用)
    InitializeI2c();

    // 2. 初始化 TCA9537 扩展芯片并复位墨水屏
    // 注意：这一步必须在 SPI 初始化之前完成，以确保屏幕已上电且 CS 片选处于正确状态
    InitializeTca9537AndResetDisplay();

    // 3. 初始化 SPI 总线
    InitializeSpi();

    // 4. 初始化墨水屏面板驱动
    InitializeEinkDisplay();
  }

  // 析构函数，通常板级对象常驻内存，但为了 C++ 规范保留
  ~ForestHeartS3() override = default;

AudioCodec* GetAudioCodec() override {
    static Es8311AudioCodec audio_codec(
        i2c_bus_,                   // 1. I2C Handle
        I2C_NUM_0,                  // 2. [新增] I2C Port (编译器提示缺这个)
        AUDIO_INPUT_SAMPLE_RATE,    // 3. Input Rate
        AUDIO_OUTPUT_SAMPLE_RATE,   // 4. Output Rate
        AUDIO_I2S_GPIO_MCLK,        // 5. MCLK
        AUDIO_I2S_GPIO_BCLK,        // 6. BCLK
        AUDIO_I2S_GPIO_WS,          // 7. WS
        AUDIO_I2S_GPIO_DOUT,        // 8. DOUT
        AUDIO_I2S_GPIO_DIN,         // 9. DIN
        GPIO_NUM_NC,                // 10. PA Pin
        AUDIO_CODEC_ES8311_ADDR,    // 11. I2C Address
        true,                       // 12. [新增] use_mclk (通常设为 true)
        true                        // 13. [新增] pa_active_high (PA 高电平有效，视硬件而定，通常 true)
    );
    return &audio_codec;
  }

  Display* GetDisplay() override {
    return display_;
  }

 private:
  // ========================================================================
  // 常量定义 (遵循 Google Style: kCamelCase)
  // ========================================================================
  static constexpr const char* kTag = "ForestHeartS3";

  // TCA9537 寄存器映射
  static constexpr uint8_t kTcaInputReg    = 0x00;
  static constexpr uint8_t kTcaOutputReg   = 0x01;
  static constexpr uint8_t kTcaPolarityReg = 0x02;
  static constexpr uint8_t kTcaConfigReg   = 0x03;

  // TCA9537 引脚位掩码 (需根据原理图确认 Bit 位)
  // 假设: Bit 0 = RST, Bit 1 = CS
  static constexpr uint8_t kTcaPinEpdRst = (1 << 0);
  static constexpr uint8_t kTcaPinEpdCs  = (1 << 1);

  // ========================================================================
  // 成员变量 (遵循 Google Style: snake_case_)
  // ========================================================================
  i2c_master_bus_handle_t i2c_bus_ = nullptr;
  i2c_master_dev_handle_t tca9537_handle_ = nullptr;
  Display* display_ = nullptr;

  // ========================================================================
  // 初始化方法
  // ========================================================================

  void InitializeI2c() {
    ESP_LOGI(kTag, "Initializing I2C Bus");
    
    i2c_master_bus_config_t i2c_bus_cfg = {
        .i2c_port = I2C_NUM_0,
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

  void InitializeTca9537AndResetDisplay() {
    ESP_LOGI(kTag, "Initializing TCA9537 & Resetting E-Ink");

    // 1. 硬件复位 TCA9537 芯片本身 (使用 ESP32 GPIO)
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << TCA9537_RST_PIN);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);

    gpio_set_level(TCA9537_RST_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(TCA9537_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(10));

    // 2. 将 TCA9537 设备添加到 I2C 总线
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = IO_EXPENDER_TCA9537_ADDR,
        .scl_speed_hz = 400000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus_, &dev_cfg, &tca9537_handle_));

    // 3. 配置 TCA9537 引脚方向
    // 目标: 将 EPD_RST 和 EPD_CS 设为输出 (0)，其他保持输入 (1)
    // 寄存器值: 0xFF & ~(RST | CS)
    uint8_t config_val = 0xFF & ~(kTcaPinEpdRst | kTcaPinEpdCs);
    uint8_t write_buf[2] = {kTcaConfigReg, config_val};
    ESP_ERROR_CHECK(i2c_master_transmit(tca9537_handle_, write_buf, sizeof(write_buf), -1));

    // 4. 执行屏幕复位序列 (RST Low -> Wait -> RST High)
    // 同时必须将 CS 拉低 (选中)，因为 SPI 驱动不控制扩展芯片上的 CS
    
    // Step A: RST = Low (复位), CS = Low (选中)
    uint8_t output_val = 0xFF & ~kTcaPinEpdRst; 
    output_val &= ~kTcaPinEpdCs; 
    
    write_buf[0] = kTcaOutputReg;
    write_buf[1] = output_val;
    ESP_ERROR_CHECK(i2c_master_transmit(tca9537_handle_, write_buf, sizeof(write_buf), -1));
    
    vTaskDelay(pdMS_TO_TICKS(20)); // 保持复位状态

    // Step B: RST = High (释放复位), CS = Low (保持选中)
    output_val |= kTcaPinEpdRst; 
    
    write_buf[1] = output_val;
    ESP_ERROR_CHECK(i2c_master_transmit(tca9537_handle_, write_buf, sizeof(write_buf), -1));
    
    vTaskDelay(pdMS_TO_TICKS(50)); // 等待屏幕内部控制器唤醒
  }

  void InitializeSpi() {
    ESP_LOGI(kTag, "Initializing Standard SPI Bus");

    // WFT042CZ15 400x300, 1bpp
    constexpr size_t kBufferPixelSize = DISPLAY_WIDTH * DISPLAY_HEIGHT;
    constexpr size_t kMaxTransferSz = kBufferPixelSize / 8 + 100;

    // 使用宏定义配置 (需确保宏 WFT042_PANEL_BUS_SPI_CONFIG 已定义)
    const spi_bus_config_t bus_config = WFT042_PANEL_BUS_SPI_CONFIG(
        DISPLAY_SPI_SCK_PIN,
        DISPLAY_SPI_MOSI_PIN,
        kMaxTransferSz
    );

    ESP_ERROR_CHECK(spi_bus_initialize(DISPLAY_SPI_PORT, &bus_config, SPI_DMA_CH_AUTO));
  }

  void InitializeEinkDisplay() {
    esp_lcd_panel_io_handle_t panel_io = nullptr;
    esp_lcd_panel_handle_t panel = nullptr;

    ESP_LOGI(kTag, "Installing WFT042 Panel IO");

    // 1. 配置 Panel IO
    // 注意: CS 引脚设为 NC，因为我们已通过 TCA9537 手动将其拉低
    const esp_lcd_panel_io_spi_config_t io_config = WFT042_PANEL_IO_SPI_CONFIG(
        GPIO_NUM_NC, 
        DISPLAY_DC_PIN, 
        nullptr, 
        nullptr
    );

    // 强制类型转换使用 static_cast
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(
        static_cast<esp_lcd_spi_bus_handle_t>(DISPLAY_SPI_PORT), 
        &io_config, 
        &panel_io));

    ESP_LOGI(kTag, "Installing WFT042 Panel Driver");

    // 2. Vendor 配置 (Busy Pin)
    // WFT042 Busy 为 Low Active (0)
    wft042_vendor_config_t vendor_config = {
        .busy_gpio_num = DISPLAY_BUSY_PIN,
        .busy_level = 0, 
    };

    // 3. 面板配置
    // Reset 填 NC，因为已经在 InitializeTca9537AndResetDisplay 中处理
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = GPIO_NUM_NC,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 1,
        .vendor_config = &vendor_config,
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_wft042(panel_io, &panel_config, &panel));

    // 4. 软件初始化
    // 避免调用 esp_lcd_panel_reset(panel)，因为硬件复位已完成
    esp_lcd_panel_init(panel);
    esp_lcd_panel_disp_on_off(panel, true);

    // 5. 创建显示对象包装器
    // 假设 SpiLcdDisplay 能够处理 1-bit 缓冲区
    display_ = new SpiLcdDisplay(
        panel_io, 
        panel,
        DISPLAY_WIDTH, 
        DISPLAY_HEIGHT, 
        0,    // Offset X
        0,    // Offset Y
        false,// Mirror X
        false,// Mirror Y
        false // Swap XY
    );
  }
};

DECLARE_BOARD(ForestHeartS3);