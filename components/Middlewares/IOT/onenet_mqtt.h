#ifndef _ONENET_MQTT_
#define _ONENET_MQTT_

#include "esp_err.h"
#include "onenet_secrets.h"

esp_err_t onenet_start(void);

esp_err_t onenet_post_property_data(char* data);

#endif