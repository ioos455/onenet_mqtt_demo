# 敏感信息保存在 onenet_secrets.h(已被 .gitignore 忽略)，首次编译时自动从模板生成
# 注意: 本文件在 ESP-IDF 需求扫描阶段也会被 include, 那时 CMAKE_CURRENT_SOURCE_DIR 为空,
# 因此这里使用 CMAKE_CURRENT_LIST_DIR 定位组件目录
if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/onenet_secrets.h")
    message(STATUS "onenet_secrets.h not found, generating from onenet_secrets.h.example")
    configure_file("${CMAKE_CURRENT_LIST_DIR}/onenet_secrets.h.example"
                   "${CMAKE_CURRENT_LIST_DIR}/onenet_secrets.h" COPYONLY)
endif()

idf_component_register(SRCS "onenet_control.c" "onenet_mqtt.c" "onenet_ota.c" "onenet_token.c" "wifi_manager.c"
                    INCLUDE_DIRS "."
                    REQUIRES app_update esp_driver_ledc esp_event esp_netif esp_wifi json mbedtls mqtt nvs_flash XL9555 LED)


