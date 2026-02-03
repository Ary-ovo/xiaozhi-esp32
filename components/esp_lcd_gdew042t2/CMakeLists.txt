idf_component_register(
    SRCS "esp_lcd_gdew042t2.c"
    INCLUDE_DIRS "include"    
    REQUIRES esp_lcd driver freertos log esp_common
)