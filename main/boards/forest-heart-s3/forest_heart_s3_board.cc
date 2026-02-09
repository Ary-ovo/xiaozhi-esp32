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

// 引入墨水屏底层驱动头文件
#include "esp_lcd_gdew042t2.h"

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
            WriteReg(0x90, 0x03);
            WriteReg(0x64, 0x02); // CV charger voltage setting to 4.1V
            WriteReg(0x61, 0x02); // set Main battery precharge current to 50mA
            WriteReg(0x62, 0x08); // set Main battery charger current to 400mA ( 0x08-200mA, 0x09-300mA, 0x0A-400mA )
            WriteReg(0x63, 0x01); // set Main battery term charge current to 25mA
    }
};

// Init SDmmc and LVGL file systerm
class Sdmmc : public SdCard {
private:
    static void fs_make_path(char * buffer, const char * path) {
        if(path[0] == '/') {
            snprintf(buffer, 256, "%s%s", MOUNT_POINT, path);
        } else {
            snprintf(buffer, 256, "%s/%s", MOUNT_POINT, path);
        }
    }
    static void * fs_open(lv_fs_drv_t * drv, const char * path, lv_fs_mode_t mode) {
        const char * flags = "";
        if(mode == LV_FS_MODE_WR) flags = "w";
        else if(mode == LV_FS_MODE_RD) flags = "r";
        else if(mode == (LV_FS_MODE_WR | LV_FS_MODE_RD)) flags = "r+";

        char real_path[256];
        fs_make_path(real_path, path);

        FILE * f = fopen(real_path, flags);
        if(f == NULL) return NULL;

        return (void *)f; // 返回文件指针作为句柄
    }
    static lv_fs_res_t fs_close(lv_fs_drv_t * drv, void * file_p) {
        FILE * f = (FILE *)file_p;
        fclose(f);
        return LV_FS_RES_OK;
    }
    /* ==========================================================
    * Callback: 读取文件
    * ========================================================== */
    static lv_fs_res_t fs_read(lv_fs_drv_t * drv, void * file_p, void * buf, uint32_t btr, uint32_t * br) {
        FILE * f = (FILE *)file_p;
        *br = fread(buf, 1, btr, f);
        return LV_FS_RES_OK;
    }
    /* ==========================================================
    * Callback: 写入文件
    * ========================================================== */
    static lv_fs_res_t fs_write(lv_fs_drv_t * drv, void * file_p, const void * buf, uint32_t btw, uint32_t * bw) {
        FILE * f = (FILE *)file_p;
        *bw = fwrite(buf, 1, btw, f);
        return LV_FS_RES_OK;
    }
    /* ==========================================================
    * Callback: 定位 (Seek)
    * ========================================================== */
    static lv_fs_res_t fs_seek(lv_fs_drv_t * drv, void * file_p, uint32_t pos, lv_fs_whence_t whence) {
        FILE * f = (FILE *)file_p;
        int mode;
        switch(whence) {
            case LV_FS_SEEK_SET: mode = SEEK_SET; break;
            case LV_FS_SEEK_CUR: mode = SEEK_CUR; break;
            case LV_FS_SEEK_END: mode = SEEK_END; break;
            default: mode = SEEK_SET;
        }
        fseek(f, pos, mode);
        return LV_FS_RES_OK;
    }
    /* ==========================================================
    * Callback: 获取位置 (Tell)
    * ========================================================== */
    static lv_fs_res_t fs_tell(lv_fs_drv_t * drv, void * file_p, uint32_t * pos_p) {
        FILE * f = (FILE *)file_p;
        *pos_p = ftell(f);
        return LV_FS_RES_OK;
    }
    static void * fs_dir_open(lv_fs_drv_t * drv, const char * path) {
        char real_path[256];
        if (path == NULL || strlen(path) == 0 || strcmp(path, "/") == 0) {
            snprintf(real_path, sizeof(real_path), "%s", MOUNT_POINT);
        } else {
            snprintf(real_path, sizeof(real_path), "%s/%s", MOUNT_POINT, path);
        }

        DIR * d = opendir(real_path);
        if (d == NULL) return NULL;
        return (void *)d;
    }

    static lv_fs_res_t fs_dir_read(lv_fs_drv_t * drv, void * dir_p, char * fn, uint32_t fn_len) {
        DIR * d = (DIR *)dir_p;
        struct dirent * entry;

        entry = readdir(d);
        if (entry) {
            strncpy(fn, entry->d_name, fn_len);
            if (fn_len > 0) fn[fn_len - 1] = '\0';
        } else {
            if (fn_len > 0) fn[0] = '\0';
        }
        return LV_FS_RES_OK;
    }

    static lv_fs_res_t fs_dir_close(lv_fs_drv_t * drv, void * dir_p) {
        closedir((DIR *)dir_p);
        return LV_FS_RES_OK;
    }
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
        ESP_LOGI(TAG, "Mounting LVGL File Systerm....");
        lv_port_fs_init();
    }
    void lv_port_fs_init(void) {
        static lv_fs_drv_t fs_drv;
        lv_fs_drv_init(&fs_drv);
        fs_drv.letter = LVGL_FS_LETTER;
        fs_drv.open_cb = fs_open;
        fs_drv.close_cb = fs_close;
        fs_drv.read_cb = fs_read;
        fs_drv.write_cb = fs_write;
        fs_drv.seek_cb = fs_seek;
        fs_drv.tell_cb = fs_tell;
        fs_drv.dir_open_cb = fs_dir_open;
        fs_drv.dir_read_cb = fs_dir_read;
        fs_drv.dir_close_cb = fs_dir_close;

        lv_fs_drv_register(&fs_drv);
        ESP_LOGI(TAG, "LVGL File System registered for '%c:' -> '/sdcard'\n", LVGL_FS_LETTER);
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
        uint8_t output_state = 0;
        WriteReg(0x01, 0x00); // 先写 Output Reg = 0x00，确保一配置成输出就是低电平
        output_state = 0x00;
        // 配置 Config Reg: 0x00 (全为输出)
        WriteReg(0x03, 0x00);
        // Excute Epd Hardware Reset
        EpdSetCs();
        EpdReset();
        SetOutputPin(P0, 1);
    }
    void EpdReset(void){
        uint8_t output_val = ReadReg(0x01);
        output_val &= ~(1 << IO_EXP_PIN_RST);
        WriteReg(0x01, output_val);
        vTaskDelay(pdMS_TO_TICKS(20));
        output_val |= (1 << IO_EXP_PIN_RST);
        WriteReg(0x01, output_val);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    void EpdSetCs(void)
    {
        uint8_t output_val = ReadReg(0x01);
        output_val |= (1 << IO_EXP_PIN_CS);
        WriteReg(0x01, output_val);
        vTaskDelay(pdMS_TO_TICKS(20));
        output_val &= ~(1 << IO_EXP_PIN_CS);
        WriteReg(0x01, output_val);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
};

class ForestHeartS3 : public WifiBoard {
private:
    i2c_master_bus_handle_t i2c_bus_;
    Pmic* pmic_;
    Display* display_ = nullptr;
    IoExpander* ioexpander_ = nullptr;
    Sdmmc* sdmmc_ = nullptr;
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
        bool card_state = false;
        sdmmc_ = new Sdmmc(MOUNT_POINT, SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0, SD_MMC_D1, SD_MMC_D2, SD_MMC_D3, card_state);
        if (card_state)
        {
            ESP_LOGE(TAG, "SDmmc Init Fail");
        }else ESP_LOGI(TAG, "SDmmc Init Success");
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

public:
    ForestHeartS3(){
        size_t internal_total = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
        size_t spiram_total = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
        ESP_LOGI(TAG, " Internal RAM Total: %d bytes (%.2f KB)", internal_total, internal_total / 1024.0);
        ESP_LOGI(TAG, " External RAM Total: %d bytes (%.2f MB)", spiram_total, spiram_total / 1024.0 / 1024.0);
        // 初始化 I2C 总线 (用于 TCA9537)
        InitializeI2c();
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
    }

    virtual AudioCodec* GetAudioCodec() override {
        static Es8311AudioCodec audio_codec(i2c_bus_, I2C_PORT, AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_GPIO_MCLK, AUDIO_I2S_GPIO_BCLK, AUDIO_I2S_GPIO_WS, AUDIO_I2S_GPIO_DOUT, AUDIO_I2S_GPIO_DIN,
            GPIO_NUM_NC, AUDIO_CODEC_ES8311_ADDR);
        return &audio_codec;
    }

    virtual Display* GetDisplay() override {
        return display_;
    }
};

DECLARE_BOARD(ForestHeartS3);