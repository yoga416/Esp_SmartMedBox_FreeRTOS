#ifndef _APP_WEATHER_H_
#define _APP_WEATHER_H_



// 心知天气 API 配置
#define WEATHER_API_KEY "SMppJL-t2_vNrnQ5C"
#define WEATHER_URL     // 将 location=taiyuan 改为 location=ip
#define SENIVERSE_URL "http://api.seniverse.com/v3/weather/now.json?key=SMppJL-t2_vNrnQ5C&location=ip&language=zh-Hans&unit=c"

/**
 * @brief 启动天气同步任务
 */
void app_weather_start_task(void);

#endif /* _APP_WEATHER_H_ */
