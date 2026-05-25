#include "ntc.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include <time.h>


static const char *TAG = "NTC";

void ntc_init(void) {
      ESP_LOGI(TAG, "NTC 初始化开始");
      /*设置时间同步模式*/
      esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
      /*设置时间服务器*/
      esp_sntp_setservername(0, "pool.ntp.org");
      esp_sntp_setservername(0, "cn.pool.ntp.org");
      esp_sntp_setservername(0, "ntp.aliyun.com");
      /*启动时间同步*/
      esp_sntp_init();

      /*设置当前时区*/
      setenv("TZ", "CST-8", 1);
      tzset();
}