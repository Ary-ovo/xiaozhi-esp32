#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <driver/i2c_master.h>

// Audio Config
#define AUDIO_INPUT_SAMPLE_RATE  24000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000

#define AUDIO_I2S_GPIO_MCLK GPIO_NUM_4
#define AUDIO_I2S_GPIO_WS   GPIO_NUM_2
#define AUDIO_I2S_GPIO_BCLK GPIO_NUM_5
#define AUDIO_I2S_GPIO_DIN  GPIO_NUM_3
#define AUDIO_I2S_GPIO_DOUT GPIO_NUM_1

#define I2C_SDA_PIN          GPIO_NUM_39 
#define I2C_SCL_PIN          GPIO_NUM_40       

#define AUDIO_CODEC_ES8311_ADDR     ES8311_CODEC_DEFAULT_ADDR
#define IO_EXPENDER_TCA9537_ADDR    0x49

// Function Button Config
#define BOOT_BUTTON_GPIO        GPIO_NUM_0

#define ENCODER_A               GPIO_NUM_14
#define ENCODER_B               GPIO_NUM_38
#define ENCODER_S               GPIO_NUM_13

// E-INK Config
#define DISPLAY_SPI_SCK_PIN     GPIO_NUM_9
#define DISPLAY_SPI_MOSI_PIN    GPIO_NUM_10
#define DISPLAY_DC_PIN          GPIO_NUM_7
#define DISPLAY_SPI_CS_PIN      -1
#define DISPLAY_BUSY_PIN        GPIO_NUM_6

#define DISPLAY_WIDTH   400
#define DISPLAY_HEIGHT  300

// PMU Config
#define AXP2101_I2C_SDA    GPIO_NUM_39     
#define AXP2101_I2C_SCL    GPIO_NUM_40
#define AXP2101_I2C_IRQ    GPIO_NUM_11

// SD Card Config
#define SD_MMC_CLK   GPIO_NUM_18
#define SD_MMC_CMD   GPIO_NUM_19
#define SD_MMC_D0    GPIO_NUM_17
#define SD_MMC_D1    GPIO_NUM_16
#define SD_MMC_D2    GPIO_NUM_21
#define SD_MMC_D3    GPIO_NUM_20

// MMW Config
#define MMW_UART_TX   GPIO_NUM_41
#define MMW_UART_RX   GPIO_NUM_46
#define MMW_UART_INT  GPIO_NUM_8

// SK6812RGW Config

#define SK6812RGBW_DATA_PIN     GPIO_NUM_45

// TCA9537 Config
#define TCA9537_RST_PIN         GPIO_NUM_12
#define TCA9537_INT_PIN         GPIO_NUM_42

#endif // _BOARD_CONFIG_H_