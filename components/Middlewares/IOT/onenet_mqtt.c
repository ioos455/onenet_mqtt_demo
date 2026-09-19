#include "onenet_mqtt.h"
#include "mqtt_client.h"
#include "onenet_token.h"
#include "onenet_control.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <cJSON.H>

#define TAG "ONENET_MQTT"

static esp_mqtt_client_handle_t mqtt_handler = NULL;    /* 回调函数句柄 */
static char token[128];                                 /* token数组 */
static char topic[128];                                 /* topic数组 */

//下行响应函数
static void onenet_porprety_ack(const char* id, int code, const char* msg);
//订阅主题
static void onenet_subscribe(void);
//上传数据
esp_err_t onenet_post_property_data(char* data);

/** MQTT事件回调函数
 * @param handler_args   用户传递的参数
 * @param base           事件所属的事件类别
 * @param event_id       事件ID
 * @param event_data     事件携带的数据
 * @return 无
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        //订阅主题
        onenet_subscribe();
        //上传默认数据
        cJSON* upload = onenet_property_upload_dm();
        char* data = cJSON_PrintUnformatted(upload);
        if(onenet_post_property_data(data))
        {
            ESP_LOGI(TAG, "Data upload succeeded");
        }
        else
        {
            ESP_LOGI(TAG, "Data upload failed");
        }
        //释放内存
        cJSON_free(data);
        cJSON_Delete(upload);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:                       /* 收到发布报文 */
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        if(strstr(event->topic, "/thing/property/set"))  /* 接收到下行topic */
        {
            cJSON* property_js = cJSON_Parse(event->data);
            //解析下行数据
            onenet_property_handle(property_js);
            //下行响应
            cJSON* id_js = cJSON_GetObjectItem(property_js, "id");/* 响应id要和该下行id保持一致 */
            onenet_porprety_ack(cJSON_GetStringValue(id_js), 200, "property success!!!");
            //释放节点
            cJSON_Delete(property_js);
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

/** 启动OneNET MQTT连接
 * @param 无
 * @return 成功返回ESP_OK，失败返回对应的ESP错误码
 */
esp_err_t onenet_start(void)
{
    esp_mqtt_client_config_t mqtt_config;
    memset(&mqtt_config, 0, sizeof(esp_mqtt_client_config_t));
    mqtt_config.broker.address.uri = "mqtt://mqtts.heclouds.com:1883";              //MQTT协议地址
    mqtt_config.broker.address.port = 1883;                                         //端口号

    //安全认证
    mqtt_config.credentials.client_id = ONENET_DEVICE_NAME;          //client_id
    mqtt_config.credentials.username = ONENET_PRODUCT_ID;            //产品ID
    
    //使用token计算key
    dev_token_generate(token, SIG_METHOD_SHA256, 3049771600, ONENET_PRODUCT_ID, ONENET_DEVICE_NAME, ONENET_PRODUCT_ACCESS_KEY);
    mqtt_config.credentials.authentication.password = token;

    mqtt_handler = esp_mqtt_client_init(&mqtt_config);
    esp_mqtt_client_register_event(mqtt_handler, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

    return esp_mqtt_client_start(mqtt_handler);
}

/*
响应Topic:
$sys/{pid}/{device-name}/thing/property/set_reply

OneJSON数据格式:
{
    "id":"123",
    "code":200,
    "msg":"xxxx"
}
    下行响应函数
*/
static void onenet_porprety_ack(const char* id, int code, const char* msg)
{
    //向topic_porperty_ack写入响应topic
    snprintf(topic, sizeof(topic), "$sys/%s/%s/thing/property/set_reply", ONENET_PRODUCT_ID, ONENET_DEVICE_NAME);
    //解析JSON
    cJSON* relay_js = cJSON_CreateObject();
    cJSON_AddStringToObject(relay_js, "id", id);
    cJSON_AddNumberToObject(relay_js, "code", code);
    cJSON_AddStringToObject(relay_js, "msg", msg);
    //简化json格式
    char* data = cJSON_PrintUnformatted(relay_js);
    //上传json
    esp_mqtt_client_publish(mqtt_handler, topic, data, strlen(data), 1, 0);
    //释放内存
    cJSON_free(data);
    //删除节点
    cJSON_Delete(relay_js);
}

//订阅主题
static void onenet_subscribe(void)
{
    //订阅上行回应主题
    snprintf(topic, sizeof(topic), "$sys/%s/%s/thing/property/post/reply", ONENET_PRODUCT_ID, ONENET_DEVICE_NAME);
    esp_mqtt_client_subscribe_single(mqtt_handler, topic, 1);
    //订阅下行主题
    snprintf(topic, sizeof(topic), "$sys/%s/%s/thing/property/set", ONENET_PRODUCT_ID, ONENET_DEVICE_NAME);
    esp_mqtt_client_subscribe_single(mqtt_handler, topic, 1);
}

//数据上报
esp_err_t onenet_post_property_data(char* data)
{
    snprintf(topic, sizeof(topic), "$sys/%s/%s/thing/property/post", ONENET_PRODUCT_ID, ONENET_DEVICE_NAME);
    ESP_LOGI(TAG, "publish topic:%s, publish data:%s", topic, data);
    return esp_mqtt_client_publish(mqtt_handler, topic, data, strlen(data), 1, 0);
}