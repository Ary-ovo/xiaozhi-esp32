#include "wifi_board.h"
#include "codecs/no_audio_codec.h"
#include "display/lcd_display.h"
#include "application.h"
#include "i2c_device.h"
#include "config.h"

#include <esp_log.h>
#include <driver/i2c_master.h>
#include <driver/ledc.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_timer.h>

#define TAG "ForestHeartS3"

class ForestHeartS3 : public WifiBoard {
private:
    i2c_master_bus_handle_t i2c_bus_;
    LcdDisplay* display_;
    esp_timer_handle_t touchpad_timer_;

    void InitializeI2c() {
        // Initialize I2C peripheral
        i2c_master_bus_config_t i2c_bus_cfg = {
            .i2c_port = (i2c_port_t)1,
            .sda_io_num = TP_PIN_NUM_TP_SDA,
            .scl_io_num = TP_PIN_NUM_TP_SCL,
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

    void BspLcdBlSet(int brightness_percent)
    {
        if (brightness_percent > 100) {
            brightness_percent = 100;
        }
        if (brightness_percent < 0) {
            brightness_percent = 0;
        }

        ESP_LOGI(TAG, "Setting LCD backlight: %d%%", brightness_percent);
        uint32_t duty_cycle = (1023 * brightness_percent) / 100; // LEDC resolution set to 10bits, thus: 100% = 1023
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty_cycle);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    }

    void InitializeSpi() {
        ESP_LOGI(TAG, "Initialize QSPI bus");

        const spi_bus_config_t bus_config = TAIJIPI_ST77916_PANEL_BUS_QSPI_CONFIG(QSPI_PIN_NUM_LCD_PCLK,
                                                                        QSPI_PIN_NUM_LCD_DATA0,
                                                                        QSPI_PIN_NUM_LCD_DATA1,
                                                                        QSPI_PIN_NUM_LCD_DATA2,
                                                                        QSPI_PIN_NUM_LCD_DATA3,
                                                                        QSPI_LCD_H_RES * 80 * sizeof(uint16_t));
        ESP_ERROR_CHECK(spi_bus_initialize(QSPI_LCD_HOST, &bus_config, SPI_DMA_CH_AUTO));
    }

    void Initializest77916Display() {

        esp_lcd_panel_io_handle_t panel_io = nullptr;
        esp_lcd_panel_handle_t panel = nullptr;

        ESP_LOGI(TAG, "Install panel IO");
        
        const esp_lcd_panel_io_spi_config_t io_config = ST77916_PANEL_IO_QSPI_CONFIG(QSPI_PIN_NUM_LCD_CS, NULL, NULL);
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)QSPI_LCD_HOST, &io_config, &panel_io));

        ESP_LOGI(TAG, "Install ST77916 panel driver");
        
        st77916_vendor_config_t vendor_config = {
            .init_cmds = lcd_init_cmds, // 如果使用自定义初始化命令，请取消注释这些行
            .init_cmds_size = sizeof(lcd_init_cmds) / sizeof(st77916_lcd_init_cmd_t),
            .flags = {
                .use_qspi_interface = 1,
            },
        };
        const esp_lcd_panel_dev_config_t panel_config = {
            .reset_gpio_num = QSPI_PIN_NUM_LCD_RST,
            .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,     // Implemented by LCD command `36h`
            .bits_per_pixel = QSPI_LCD_BIT_PER_PIXEL,    // Implemented by LCD command `3Ah` (16/18)
            .vendor_config = &vendor_config,
        };
        ESP_ERROR_CHECK(esp_lcd_new_panel_st77916(panel_io, &panel_config, &panel));

        esp_lcd_panel_reset(panel);
        esp_lcd_panel_init(panel);
        esp_lcd_panel_disp_on_off(panel, true);
        esp_lcd_panel_swap_xy(panel, DISPLAY_SWAP_XY);
        esp_lcd_panel_mirror(panel, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y);

        display_ = new SpiLcdDisplay(panel_io, panel,
                                    DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y, DISPLAY_SWAP_XY);
    }

    void InitializeMute() {
        gpio_reset_pin(AUDIO_MUTE_PIN);
        /* Set the GPIO as a push/pull output */
        gpio_set_direction(AUDIO_MUTE_PIN, GPIO_MODE_OUTPUT);
        gpio_set_level(AUDIO_MUTE_PIN, 1);
    }

public:
    ForestHeartS3() {
        InitializeI2c();
        InitializeCst816sTouchPad();
        InitializeSpi();
        Initializest77916Display();
        InitializeMute();
        GetBacklight()->RestoreBrightness();
    }

    virtual AudioCodec* GetAudioCodec() override {
#ifdef CONFIG_TAIJIPAI_I2S_TYPE_STD
        static NoAudioCodecSimplex audio_codec(
            AUDIO_INPUT_SAMPLE_RATE,
            AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_GPIO_BCLK,
            AUDIO_I2S_GPIO_WS,
            AUDIO_I2S_GPIO_DOUT,
            #ifdef CONFIG_I2S_USE_2SLOT
            I2S_STD_SLOT_BOTH,
            #endif
            AUDIO_MIC_SCK_PIN,
            AUDIO_MIC_WS_PIN,
	        #ifdef CONFIG_I2S_USE_2SLOT
            AUDIO_MIC_SD_PIN,
            I2S_STD_SLOT_LEFT
            #else
            AUDIO_MIC_SD_PIN
            #endif
        );
#else
        static NoAudioCodecSimplexPdm audio_codec(
            AUDIO_INPUT_SAMPLE_RATE,
            AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_GPIO_BCLK,
            AUDIO_I2S_GPIO_WS,
            AUDIO_I2S_GPIO_DOUT,
            #ifdef CONFIG_I2S_USE_2SLOT
            I2S_STD_SLOT_BOTH,
            #endif 
            AUDIO_MIC_WS_PIN,
            AUDIO_MIC_SD_PIN
        );
#endif
        return &audio_codec;
    }

    virtual Display* GetDisplay() override {
        return display_;
    }
    
    virtual Backlight* GetBacklight() override {
        static PwmBacklight backlight(DISPLAY_BACKLIGHT_PIN, DISPLAY_BACKLIGHT_OUTPUT_INVERT);
        return &backlight;
    }

    Cst816s* GetTouchpad() {
        return cst816s_;
    }
};

DECLARE_BOARD(ForestHeartS3);
