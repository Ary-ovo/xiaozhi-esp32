#include "wifi_board.h"
#include "codecs/es8311_audio_codec.h"
#include "display/epd_display.h"
#include "application.h"
#include "i2c_device.h"
#include "config.h"
#include "axp2101.h"
#include "sdmmc.h"
#include <esp_log.h>
#include "driver/gpio.h"
#include "tca9537.h"
#include "power_save_timer.h"
#include "mmw.h"
#include "esp_lcd_gdew042t2.h"
#include "led/rgbw_strip.h"

#define TAG "ForestHeartS3"

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
            // Enable ALDO1 ALDO2 Disable Other LDO
            WriteReg(0x90, 0x01);

            WriteReg(0x64, 0x02); // CV charger voltage setting to 4.1V
            WriteReg(0x61, 0x02); // set Main battery precharge current to 50mA
            WriteReg(0x62, 0x08); // set Main battery charger current to 400mA ( 0x08-200mA, 0x09-300mA, 0x0A-400mA )
            WriteReg(0x63, 0x01); // set Main battery term charge current to 25mA
    }
};

// Init SDmmc and LVGL file systerm
class Sdmmc : public SdCard {
private:

public:
    // Init SDmmc 
    Sdmmc(const char* mount_point, 
        gpio_num_t clk, 
        gpio_num_t cmd, 
        gpio_num_t d0, 
        gpio_num_t d1 = GPIO_NUM_NC,
        gpio_num_t d2 = GPIO_NUM_NC, 
        gpio_num_t d3 = GPIO_NUM_NC,
        bool format_if_mount_failed = false) : 
    SdCard(mount_point, clk, cmd, d0, d1, d2, d3, format_if_mount_failed){
    }
};

class IoExpander : public Tca9537 {
public:
    // Init TCA9537
    IoExpander(i2c_master_bus_handle_t i2c_bus, uint8_t addr) : Tca9537(i2c_bus, addr) {
        ESP_LOGI(TAG, "Start IoExpander Sequence...");
        // 硬件复位引脚操作 (复位 TCA9537 芯片本身)
        gpio_set_direction(TCA9537_RST_PIN, GPIO_MODE_OUTPUT);
        gpio_set_level(TCA9537_RST_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(20));
        gpio_set_level(TCA9537_RST_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(20)); // 等待芯片苏醒
        // 定义状态变量
        WriteReg(TCA9537_REG_OUTPUT, 0x00); // 先写 Output Reg = 0x00，确保一配置成输出就是低电平
        // 配置 Config Reg: 0x00 (全为输出)
        WriteReg(TCA9537_REG_CONFIG, 0x00);
        // Excute Epd Hardware Reset
        EpdSetCs();
        EpdReset();
        SetOutputPin(P0, 1);
        SetOutputPin(IO_EXP_PIN_SPK, 0);
    }
    void EpdReset(void){
        uint8_t output_val = ReadReg(TCA9537_REG_OUTPUT);
        output_val &= ~(1 << IO_EXP_PIN_RST);
        WriteReg(TCA9537_REG_OUTPUT, output_val);
        vTaskDelay(pdMS_TO_TICKS(20));
        output_val |= (1 << IO_EXP_PIN_RST);
        WriteReg(TCA9537_REG_OUTPUT, output_val);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    void EpdSetCs(void)
    {
        uint8_t output_val = ReadReg(TCA9537_REG_OUTPUT);
        output_val |= (1 << IO_EXP_PIN_CS);
        WriteReg(TCA9537_REG_OUTPUT, output_val);
        vTaskDelay(pdMS_TO_TICKS(20));
        output_val &= ~(1 << IO_EXP_PIN_CS);
        WriteReg(TCA9537_REG_OUTPUT, output_val);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
};

class PresenceSensor : public MmwRadar {
public:
    // 构造函数即完成初始化和配置
    PresenceSensor(uart_port_t uart_num, int tx_pin, int rx_pin) : MmwRadar({uart_num, tx_pin, rx_pin}) {
        // 1. 设置回调函数（当检测到有人/无人变化时做什么）
        this->SetStateChangedCallback([this](MmwState state) {
            this->OnStateChanged(state);
        });
        // 2. 启动底层线程 (调用基类的 Start)
        Start(); 
        // 3. 发送配置指令 (例如开启自动上报)
        EnableAutoReport();
        ESP_LOGI("RADAR", "Radar Thread Started & Configured");
    }
private:
    // 处理状态变化的逻辑
    void OnStateChanged(MmwState state) {
        if (state == MmwState::PRESENCE) {
            ESP_LOGW("RADAR", "检测到有人！(User Presence Detected)");
            // 在这里可以通知小智的主逻辑，例如：
            // application_->WakeUp(); 
            // audio_->PlaySound("hello.mp3");
        } else {
            ESP_LOGI("RADAR", "无人。(No Presence)");
            // application_->GoToSleep();
        }
    }
};

class CustomAudioCodec : public Es8311AudioCodec {
private:
    IoExpander* ioexpander_;

public:
    CustomAudioCodec(i2c_master_bus_handle_t i2c_bus, IoExpander* ioexpander) 
        : Es8311AudioCodec(i2c_bus, I2C_PORT, AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_GPIO_MCLK, AUDIO_I2S_GPIO_BCLK, AUDIO_I2S_GPIO_WS, AUDIO_I2S_GPIO_DOUT, AUDIO_I2S_GPIO_DIN,
            GPIO_NUM_NC, AUDIO_CODEC_ES8311_ADDR), 
          ioexpander_(ioexpander) {
    }

    virtual void EnableOutput(bool enable) override {
        Es8311AudioCodec::EnableOutput(enable);
        if (enable) {
            ioexpander_->SetOutputPin(IO_EXP_PIN_SPK, 1);
        } else {
            ioexpander_->SetOutputPin(IO_EXP_PIN_SPK, 0);
        }
    }
};


class ForestHeartS3 : public WifiBoard {
private:
    i2c_master_bus_handle_t i2c_bus_;
    Pmic* pmic_;
    Display* display_ = nullptr;
    IoExpander* ioexpander_ = nullptr;
    Sdmmc* sdmmc_ = nullptr;
    PresenceSensor* radar_ = nullptr;
    RgbwStrip* rgbwstrip_ = nullptr;
    PowerSaveTimer* power_save_timer_;

    // 初始化 I2C 总线
    void InitializeI2c() {
        i2c_master_bus_config_t i2c_bus_cfg = {
            .i2c_port = I2C_PORT, 
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
    // Init AXP2101
    void InitializeAxp2101() {
        ESP_LOGI(TAG, "Init AXP2101");
        pmic_ = new Pmic(i2c_bus_, AXP2101_I2C_ADDR);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    // Enable EPD Pin Presetting
    void EpdHwPreInit() {
        ESP_LOGI(TAG, "EPD Hardware Config");
        ioexpander_ = new IoExpander(i2c_bus_, TCA9537_ADDR);
    }
    // 初始化 SPI 总线 (标准 SPI)
    void InitializeSpi() {
        ESP_LOGI(TAG, "Initialize SPI bus for EPD");

        spi_bus_config_t buscfg = {
            .mosi_io_num = DISPLAY_SPI_MOSI_PIN,
            .miso_io_num = -1,
            .sclk_io_num = DISPLAY_SPI_SCK_PIN,
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
        const esp_lcd_panel_io_spi_config_t io_config = {
            .cs_gpio_num = DISPLAY_SPI_CS_PIN,
            .dc_gpio_num = DISPLAY_DC_PIN,
            .spi_mode = 0,
            .pclk_hz = 10 * 1000 * 1000,
            .trans_queue_depth = 10,
            .lcd_cmd_bits = 8,
            .lcd_param_bits = 8,
        };
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(EPD_SPI_HOST, &io_config, &panel_io));
        ESP_LOGI(TAG, "Install GDEW042T2 Panel Driver");
        esp_lcd_panel_gdew042t2_config_t vendor_config = {
            .busy_gpio_num = DISPLAY_BUSY_PIN,
            .busy_active_level = 0, // Low = Busy
        };
        const esp_lcd_panel_dev_config_t panel_config = {
            .reset_gpio_num = DISPLAY_RESET_PIN, 
            .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
            .bits_per_pixel = 16,
            .vendor_config = &vendor_config,
        };
        ESP_ERROR_CHECK(esp_lcd_new_panel_gdew042t2(panel_io, &panel_config, &panel));
        esp_lcd_panel_reset(panel);
        ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
        esp_lcd_gdew042t2_set_mode(panel, GDEW042T2_REFRESH_FULL);
        display_ = new EpdDisplay(panel_io, panel, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    }
    // Init sdmmc
    void Sdmmc_Init(){
        bool auto_format = false;
        sdmmc_ = new Sdmmc(MOUNT_POINT, SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0, SD_MMC_D1, SD_MMC_D2, SD_MMC_D3, auto_format);
        if (sdmmc_->IsMounted()) {
            ESP_LOGI(TAG, "SDmmc Init Success");
        } else {
            ESP_LOGE(TAG, "SDmmc Init Fail");
        }
    }


    
    // Scan i2c devices
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
    // Init PresenSensor Unit
    void Init_PresenSensor(void)
    {
        radar_ = new PresenceSensor(UART_NUM_1, MMW_UART_RX, MMW_UART_TX);
    }

    void Init_RgbwStrip(void){
        rgbwstrip_ = new RgbwStrip(SK6812RGBW_DATA_PIN, SK6812RGBW_NUM);
    }

public:
    ForestHeartS3(){
        size_t internal_total = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
        size_t spiram_total = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
        ESP_LOGI(TAG, " Internal RAM Total: %d bytes (%.2f KB)", internal_total, internal_total / 1024.0);
        ESP_LOGI(TAG, " External RAM Total: %d bytes (%.2f MB)", spiram_total, spiram_total / 1024.0 / 1024.0);
      
        InitializeI2c(); // 初始化 I2C 总线 (用于 TCA9537)
        // Init SDcard
        Sdmmc_Init();
        // Set up All Device Power
        InitializeAxp2101();
        // EPD 硬件预处理 (控制 TCA9537 拉低 CS，拉高 RST)
        EpdHwPreInit();
        // Check I2c Device
        I2cDetect();
        // SPI 初始化
        InitializeSpi();
        // 墨水屏初始化 & UI 启动
        InitializeGDEW042T2Display();
        // // Init MicroWave Radar
        // Init_PresenSensor();

        Init_RgbwStrip();
    }
    
    virtual AudioCodec* GetAudioCodec() override {
        static CustomAudioCodec audio_codec(
            i2c_bus_, 
            ioexpander_);
        return &audio_codec;
    }
    virtual Display* GetDisplay() override {
        return display_;
    }
    virtual bool GetBatteryLevel(int &level, bool& charging, bool& discharging) override {
        static bool last_discharging = false;
        charging = pmic_->IsCharging();
        discharging = pmic_->IsDischarging();
        if (discharging != last_discharging) {
            power_save_timer_->SetEnabled(discharging);
            last_discharging = discharging;
        }
        level = pmic_->GetBatteryLevel();
        return true;
    }
};

DECLARE_BOARD(ForestHeartS3);