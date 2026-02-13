#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <driver/i2c_master.h>

// Audio Config
#define AUDIO_CODEC_ES8311_ADDR     ES8311_CODEC_DEFAULT_ADDR

#define AUDIO_INPUT_SAMPLE_RATE     16000
#define AUDIO_OUTPUT_SAMPLE_RATE    16000

#define AUDIO_I2S_GPIO_MCLK         GPIO_NUM_4
#define AUDIO_I2S_GPIO_WS           GPIO_NUM_2
#define AUDIO_I2S_GPIO_BCLK         GPIO_NUM_5
#define AUDIO_I2S_GPIO_DIN          GPIO_NUM_3
#define AUDIO_I2S_GPIO_DOUT         GPIO_NUM_1

// I2C Bus Config
#define I2C_PORT             I2C_NUM_0
#define I2C_SDA_PIN          GPIO_NUM_39 
#define I2C_SCL_PIN          GPIO_NUM_40       

// Function Button Config
#define BOOT_BUTTON_GPIO        GPIO_NUM_0

#define ENCODER_A               GPIO_NUM_14
#define ENCODER_B               GPIO_NUM_38
#define ENCODER_S               GPIO_NUM_13

// E-INK Config
#define EPD_SPI_HOST            SPI2_HOST
#define DISPLAY_SPI_SCK_PIN     GPIO_NUM_9
#define DISPLAY_SPI_MOSI_PIN    GPIO_NUM_10
#define DISPLAY_DC_PIN          GPIO_NUM_7
#define DISPLAY_SPI_CS_PIN      -1
#define DISPLAY_BUSY_PIN        GPIO_NUM_6
#define DISPLAY_RESET_PIN       -1

#define DISPLAY_WIDTH   400
#define DISPLAY_HEIGHT  300

// PMU Config
#define AXP2101_I2C_ADDR   0x34
#define AXP2101_I2C_SDA    GPIO_NUM_39     
#define AXP2101_I2C_SCL    GPIO_NUM_40
#define AXP2101_I2C_IRQ    GPIO_NUM_11

// SD Card Config
#define LVGL_FS_LETTER   'S'
#define MOUNT_POINT      "/sdcard"
#define SD_MMC_CLK       GPIO_NUM_18
#define SD_MMC_CMD       GPIO_NUM_19
#define SD_MMC_D0        GPIO_NUM_17
#define SD_MMC_D1        GPIO_NUM_16
#define SD_MMC_D2        GPIO_NUM_21
#define SD_MMC_D3        GPIO_NUM_20

#define ICONS_PATH       "S:/icons/"  // 假设 S: 是你的 SD 卡挂载点
#define FONTS_PATH       "S:/fonts/"  // 假设 S: 是你的 SD 卡挂载点

// MMW Config
#define MMW_UART_TX   GPIO_NUM_41
#define MMW_UART_RX   GPIO_NUM_46
#define MMW_UART_INT  GPIO_NUM_8

// SK6812RGW Config
#define SK6812RGBW_DATA_PIN     GPIO_NUM_45
#define SK6812RGBW_NUM          4

// TCA9537 Config
#define TCA9537_ADDR            0x49
#define TCA9537_RST_GPIO        12    // TCA9537的RST复位引脚
#define TCA9537_REG_OUTPUT      0x01  // 输出端口寄存器
#define TCA9537_REG_CONFIG      0x03  // 配置寄存器
#define TCA9537_RST_PIN         GPIO_NUM_12
#define TCA9537_INT_PIN         GPIO_NUM_42

// P1
#define IO_EXP_PIN_SPK   1  // P1: 喇叭 (1=开, 0=关)
// P2
#define IO_EXP_PIN_RST   2  // P2: EPD复位 (0=复位, 1=工作)
// P3
#define IO_EXP_PIN_CS    3  // P3: EPD片选 (0=选中, 1=取消)

#endif // _BOARD_CONFIG_H_