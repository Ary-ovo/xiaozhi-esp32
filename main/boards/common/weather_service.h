#ifndef WEATHER_SERVICE_H
#define WEATHER_SERVICE_H

#include <string>
#include <mutex>
#include <vector>

// ========================================================
// 和风天气配置 (请修改为您自己的)
// ========================================================
// 注册地址: https://console.qweather.com/
#define QWEATHER_KEY      "4182ff2353b3450480aabdca8413b2a4"  
// 城市 ID (例如: 北京=101010100, 上海=101020100)
// 查询地址: https://github.com/qwd/LocationList
#define QWEATHER_LOCATION "101010100" 

struct DailyForecast {
    std::string icon_day;
    std::string temp_max;
    std::string temp_min;
};

struct WeatherData {
    std::string temp = "--";
    std::string feels_like = "--";
    std::string text = "未知";
    std::string icon_code = "999";
    std::string aqi_category = "--";
    std::vector<DailyForecast> daily;
    bool is_valid = false;
};

class WeatherService {
public:
    static WeatherService& GetInstance() {
        static WeatherService instance;
        return instance;
    }

    void UpdateWeatherAsync();
    WeatherData GetWeatherData();

    // ==========================================================
    // 【新增】设置要查询的地点 (支持拼音、英文、经纬度)
    // ==========================================================
    void SetLocation(const std::string& location_query);

private:
    WeatherService() = default;
    
    void FetchWeatherTask();
    std::string HttpGet(const std::string& url);
    static void TaskWrapper(void* arg);

    // ==========================================================
    // 【新增】调用 Geo API 获取真正的 Location ID
    // ==========================================================
    bool FetchLocationId();

    std::mutex mutex_;
    WeatherData cached_data_;

    // 新增的状态变量
    std::string location_query_ = "nanning"; // 默认查询词 (拼音最安全，不带中文避免URL编码问题)
    std::string current_location_id_ = "";   // 存放解析出来的 ID
};

#endif // WEATHER_SERVICE_H
