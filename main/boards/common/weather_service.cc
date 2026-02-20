#include "weather_service.h"
#include "board.h"
#include <esp_log.h>
#include <cJSON.h>
#include <cstring>
#include <zlib.h> // 必须引入
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "WeatherService";

void WeatherService::SetLocation(const std::string& location_query) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (location_query_ != location_query) {
        location_query_ = location_query;
        current_location_id_ = ""; // 清空旧 ID，强制下次更新时重新获取
        ESP_LOGI(TAG, "Location changed to: %s", location_query_.c_str());
    }
}

bool WeatherService::FetchLocationId() {
    ESP_LOGI(TAG, "Fetching Location ID for: %s", location_query_.c_str());

    // 注意：Geo API 的域名是 geoapi.qweather.com
    std::string geo_url = "https://geoapi.qweather.com/v2/city/lookup?location=" + 
                          location_query_ + "&key=" + QWEATHER_KEY;
    
    std::string geo_resp = HttpGet(geo_url);
    if (geo_resp.empty()) {
        ESP_LOGE(TAG, "Geo API request failed");
        return false;
    }

    cJSON* root = cJSON_Parse(geo_resp.c_str());
    if (!root) {
        ESP_LOGE(TAG, "Geo JSON Parse failed");
        return false;
    }

    bool success = false;
    cJSON* code = cJSON_GetObjectItem(root, "code");
    
    // 检查状态码是否为 200
    if (code && cJSON_IsString(code) && strcmp(code->valuestring, "200") == 0) {
        cJSON* location_array = cJSON_GetObjectItem(root, "location");
        
        // 获取数组中的第一个匹配结果
        if (cJSON_IsArray(location_array) && cJSON_GetArraySize(location_array) > 0) {
            cJSON* first_item = cJSON_GetArrayItem(location_array, 0);
            
            cJSON* id = cJSON_GetObjectItem(first_item, "id");
            cJSON* name = cJSON_GetObjectItem(first_item, "name");
            cJSON* adm1 = cJSON_GetObjectItem(first_item, "adm1"); // 省份

            if (id && cJSON_IsString(id)) {
                current_location_id_ = id->valuestring;
                success = true;
                
                ESP_LOGI(TAG, "Geo Success: %s, %s -> ID: %s", 
                         (name ? name->valuestring : "Unknown"), 
                         (adm1 ? adm1->valuestring : ""),
                         current_location_id_.c_str());
            }
        }
    } else {
        ESP_LOGW(TAG, "Geo API returned code: %s", code ? code->valuestring : "null");
    }

    cJSON_Delete(root);
    return success;
}

std::string GzipUncompress(const std::string& data) {
    if (data.empty()) return "";

    z_stream zs;
    memset(&zs, 0, sizeof(zs));

    // 15+16 启用 gzip 头检测
    if (inflateInit2(&zs, 15 + 16) != Z_OK) {
        ESP_LOGE(TAG, "inflateInit2 failed");
        return "";
    }

    zs.next_in = (Bytef*)data.data();
    zs.avail_in = data.size();

    int ret;
    char outbuffer[2048]; // 解压缓冲区
    std::string outstring;

    // 循环解压
    do {
        zs.next_out = (Bytef*)outbuffer;
        zs.avail_out = sizeof(outbuffer);

        ret = inflate(&zs, Z_NO_FLUSH);

        if (outstring.size() < zs.total_out) {
            outstring.append(outbuffer, zs.total_out - outstring.size());
        }

    } while (ret == Z_OK);

    inflateEnd(&zs);

    if (ret != Z_STREAM_END) {
        ESP_LOGE(TAG, "Zlib error: %d", ret);
        return "";
    }

    return outstring;
}

void WeatherService::UpdateWeatherAsync() {
    auto& board = Board::GetInstance();
    if (!board.GetNetwork()) return;
    // 增加堆栈到 12KB，解压非常消耗堆栈
    xTaskCreate(TaskWrapper, "weather_once", 12 * 1024, this, 5, NULL);
}

void WeatherService::TaskWrapper(void* arg) {
    WeatherService* self = static_cast<WeatherService*>(arg);
    if (self) self->FetchWeatherTask();
    vTaskDelete(NULL);
}

WeatherData WeatherService::GetWeatherData() {
    std::lock_guard<std::mutex> lock(mutex_);
    return cached_data_;
}

std::string WeatherService::HttpGet(const std::string& url) {
    auto& board = Board::GetInstance();
    auto network = board.GetNetwork();
    if (!network) return "";

    auto http = network->CreateHttp(0);
    if (!http) return "";

    ESP_LOGI(TAG, "GET URL: %s", url.c_str());

    // 1. 设置 Accept-Encoding: gzip (既然服务器强制发，我们就主动认领)
    http->SetHeader("Accept-Encoding", "gzip");

    if (!http->Open("GET", url.c_str())) {
        ESP_LOGE(TAG, "HTTP Open failed");
        return "";
    }

    // 2. 读取二进制数据
    char buffer[1024]; 
    std::string response;
    int read_len = 0;
    while ((read_len = http->Read(buffer, sizeof(buffer))) > 0) {
        // 注意：这是二进制追加
        response.append(buffer, read_len);
    }
    http->Close();

    ESP_LOGI(TAG, "Download Size: %d bytes", response.size());

    if (response.empty()) return "";

    // 3. 检测 GZIP 头 (0x1F 0x8B)
    if (response.size() > 2 && (uint8_t)response[0] == 0x1F && (uint8_t)response[1] == 0x8B) {
        ESP_LOGI(TAG, "GZIP detected. Decompressing...");
        std::string plain_text = GzipUncompress(response);
        ESP_LOGI(TAG, "Decompressed Size: %d bytes", plain_text.size());
        return plain_text;
    }

    // 如果不是 GZIP，直接返回
    return response;
}

void WeatherService::FetchWeatherTask() {
    // 1. 延时防抢跑
    vTaskDelay(pdMS_TO_TICKS(3000));

    WeatherData new_data;
    bool weather_success = false;

    if (current_location_id_.empty()) {
        if (!FetchLocationId()) {
            ESP_LOGE(TAG, "Failed to resolve location, aborting weather update.");
            return; // 拿不到 ID 就直接退出，不查天气了
        }
        // 成功拿到 ID 后，给服务器一点喘息时间
        vTaskDelay(pdMS_TO_TICKS(1000)); 
    }

    // 将 ID 取出来备用，避免与原来代码中的宏起冲突
    std::string loc_id = current_location_id_;

    // 2. Weather Now
    std::string weather_url = "https://devapi.qweather.com/v7/weather/now?location=" + 
                              loc_id + "&key=" + QWEATHER_KEY + "&lang=zh";
    std::string weather_resp = HttpGet(weather_url);
    if (!weather_resp.empty()) {
        cJSON* root = cJSON_Parse(weather_resp.c_str());
        if (root) {
            cJSON* now = cJSON_GetObjectItem(root, "now");
            if (now) {
                cJSON* temp = cJSON_GetObjectItem(now, "temp");
                cJSON* feels = cJSON_GetObjectItem(now, "feelsLike");
                cJSON* text = cJSON_GetObjectItem(now, "text");
                cJSON* icon = cJSON_GetObjectItem(now, "icon");

                if (temp) new_data.temp = temp->valuestring;
                if (feels) new_data.feels_like = feels->valuestring;
                if (text) new_data.text = text->valuestring;
                if (icon) new_data.icon_code = icon->valuestring;
                weather_success = true;
            }
            cJSON_Delete(root);
        }
    }
    
    vTaskDelay(pdMS_TO_TICKS(1000));

    // 3. Air Now
    std::string air_url = "https://devapi.qweather.com/v7/air/now?location=" + 
                          loc_id + "&key=" + QWEATHER_KEY + "&lang=zh";
    std::string air_resp = HttpGet(air_url);
    if (!air_resp.empty()) {
        cJSON* root = cJSON_Parse(air_resp.c_str());
        if (root) {
            cJSON* now = cJSON_GetObjectItem(root, "now");
            if (now) {
                cJSON* category = cJSON_GetObjectItem(now, "category");
                if (category) new_data.aqi_category = category->valuestring;
            }
            cJSON_Delete(root);
        }
    }
    if (new_data.aqi_category.empty()) new_data.aqi_category = "-";

    vTaskDelay(pdMS_TO_TICKS(1000));

    // ==========================================================
    // 4. Forecast 7d (获取7天预报)
    // ==========================================================
    std::string forecast_url = "https://devapi.qweather.com/v7/weather/7d?location=" + 
                               loc_id + "&key=" + QWEATHER_KEY + "&lang=zh";
    std::string forecast_resp = HttpGet(forecast_url);
    if (!forecast_resp.empty()) {
        cJSON* root = cJSON_Parse(forecast_resp.c_str());
        if (root) {
            cJSON* daily_array = cJSON_GetObjectItem(root, "daily");
            if (cJSON_IsArray(daily_array)) {
                int count = cJSON_GetArraySize(daily_array);
                // 【关键修改】这里改为 i < 7，提取 7 天的数据
                for (int i = 0; i < count && i < 7; i++) {
                    cJSON* item = cJSON_GetArrayItem(daily_array, i);
                    DailyForecast day;
                    
                    cJSON* icon = cJSON_GetObjectItem(item, "iconDay");
                    cJSON* tmax = cJSON_GetObjectItem(item, "tempMax");
                    cJSON* tmin = cJSON_GetObjectItem(item, "tempMin");

                    if (icon) day.icon_day = icon->valuestring;
                    else day.icon_day = "999"; 

                    if (tmax) day.temp_max = tmax->valuestring;
                    if (tmin) day.temp_min = tmin->valuestring;
                    
                    new_data.daily.push_back(day);
                }
            }
            cJSON_Delete(root);
        }
    } else {
        ESP_LOGE(TAG, "Failed to get 7d Forecast");
    }

    // 5. Update
    if (weather_success) {
        new_data.is_valid = true;
        std::lock_guard<std::mutex> lock(mutex_);
        cached_data_ = new_data;
        // 日志会打印出 Days: 7
        ESP_LOGI(TAG, "Weather Updated! Temp: %s, AQI: %s, Days: %d", 
                 new_data.feels_like.c_str(), new_data.aqi_category.c_str(), new_data.daily.size());
    } else {
        ESP_LOGE(TAG, "Update Failed");
    }
}