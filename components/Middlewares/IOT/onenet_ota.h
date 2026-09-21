#ifndef _ONENET_OAT_
#define _ONENET_OTA_

#include "esp_err.h"

typedef enum{
    invalid = 0,
    valid   = 1,
} app_partition_valid_t;

//获取当前版本号
const char *get_app_version(void);

//设置当前程序是否合法
void set_app_valid(app_partition_valid_t app_valid);

//函数上报版本号,使用http协议请求应答
esp_err_t onenet_upload_version(void);

#endif