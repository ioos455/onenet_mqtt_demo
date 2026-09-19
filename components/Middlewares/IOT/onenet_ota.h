#ifndef _ONENET_OAT_
#define _ONENET_OTA_

typedef enum{
    invalid = 0,
    valid   = 1,
} app_partition_valid_t;

//获取当前版本号
const char *get_app_version(void);

//设置当前程序是否合法
void set_app_valid(app_partition_valid_t app_valid);

#endif