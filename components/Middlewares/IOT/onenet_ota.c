#include "esp_ota_ops.h"
#include "onenet_ota.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

#define TAG     "onenet_ota_http"

//当前app版本号
static char app_version[32] = {0};
//接收到的ota缓存
#define OTA_BUFF_LEN       1024
static uint8_t ota_data_buf[OTA_BUFF_LEN];
//接收到的OTA数据长度
static int ota_data_size = 0;

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

//http回调函数
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
        {
            ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            ESP_LOGI(TAG, "HTTP_EVENT_DATA:%s", evt->data);
            //定义拷贝长度
            int copy_len = 0;
            //判断接收到的数据长度是否超出缓冲区
            if(evt->data_len > OTA_BUFF_LEN - ota_data_size)
            {
                copy_len = OTA_BUFF_LEN - ota_data_size;
            }
            else
            {
                copy_len = evt->data_len;
            }
            //数据拷贝
            memcpy(&ota_data_buf[ota_data_size], evt->data, copy_len);
            //更新缓冲区长度
            ota_data_size += copy_len;

        }   break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            break;
        default: break;
    }
    return ESP_OK;
}

/*函数上报版本号,使用http协议请求应答
* @param 无
* @return esp_err_t，返回成功return ESP_OK
*/
esp_err_t onenet_upload_version(void)
{

    esp_http_client_config_t config = {
        .url = "",
        .event_handler = _http_event_handler, 
    };
    ESP_LOGI(TAG, "HTTP request with url =>");
    esp_http_client_handle_t client = esp_http_client_init(&config);

    // POST
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    err = esp_http_client_perform(client);
}