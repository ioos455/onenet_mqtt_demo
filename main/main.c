#include <stdio.h>
#include "onenet_mqtt.h"
#include "wifi_manager.h"
#include "onenet_control.h"
#include "onenet_secrets.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#define TAG "main_onenet"
#define WIFI_CONNECT_BIT BIT0

static EventGroupHandle_t wifi_ev = NULL;

/** WiFi状态回调函数
 * @param state  当前WiFi连接状态
 * @return 无
 */
static void wifi_manager_cb(WIFI_STATE state)
{
    if(state == WIFI_STATE_CONNECTED)
    {
        xEventGroupSetBits(wifi_ev, WIFI_CONNECT_BIT);
    }
}

/** 应用程序入口函数
 * @param 无
 * @return 无
 */
void app_main(void)
{
    nvs_flash_init();
    wifi_ev = xEventGroupCreate();
    onenet_control_init();
    wifi_manager_init(wifi_manager_cb);
    wifi_manager_connect(WIFI_SSID, WIFI_PASSWORD);
    EventBits_t ev;
    while (1)
    {
        ev = xEventGroupWaitBits(wifi_ev, WIFI_CONNECT_BIT, pdTRUE, pdFALSE, pdMS_TO_TICKS(50));
        if(ev & WIFI_CONNECT_BIT)
        {
            onenet_start();
        }
    }
    
}
