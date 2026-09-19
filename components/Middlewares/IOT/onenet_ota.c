#include "esp_ota_ops.h"
#include "onenet_ota.h"

#include <stdio.h>

static char app_version[32] = {0};

/*获取当前app运行版本
* @param 无
* @return 当前app运行版本号
*/
const char *get_app_version(void)
{
    if(app_version[0] == 0)
    {
        const esp_partition_t * app_running = esp_ota_get_running_partition();  /* 获取当前运行的分区表信息 */
        esp_app_desc_t app_description;  /* 用于保存分区表详细信息 */
        esp_ota_get_partition_description(app_running, &app_description);    /* 获取分区表描述信息保存在第二个结构体里面 */
        //拷贝app_description.version数组数据到app_version
        snprintf(app_version, sizeof(app_version), "%s", app_description.version);
    }
    return app_version;
}

/*设置当前程序合法性
* @param 无
* @return 无
*/
void set_app_valid(app_partition_valid_t app_valid)
{
    esp_ota_img_states_t app_state;
    const esp_partition_t * app_running = esp_ota_get_running_partition();  /* 获取当前运行的分区表信息 */
    if(esp_ota_get_state_partition(app_running, &app_state) == ESP_OK)/* 获取当前分区表状态 */
    {
        if(app_state == ESP_OTA_IMG_PENDING_VERIFY)/* 当前是pending_verify状态才可设置valid */
        {
            if (app_valid)
            {
                esp_ota_mark_app_valid_cancel_rollback();
            }
            else
            {
                esp_ota_mark_app_invalid_rollback_and_reboot();
            }
            
        }
    }

}