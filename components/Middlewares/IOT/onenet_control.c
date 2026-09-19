#include "onenet_control.h"
#include "xl9555.h"
#include "led.h"
#include "driver/ledc.h"
#include "string.h"

static int led_brightness = 0;
static bool led_lightswitch = 0;

/**
 * 物模型数据初始化
 * @param 无
 * @return 无
 */
void onenet_control_init(void)
{
    //初始化按键
    xl9555_init();
    
    //初始化led
    led_init();

    //LED时钟初始化
    ledc_timer_config_t led_timer = 
    {
        .clk_cfg = LEDC_AUTO_CLK,
        .duty_resolution = LEDC_TIMER_12_BIT,   //2^12-1
        .freq_hz = 5000,
        .timer_num = LEDC_TIMER_0,
    };
    ledc_timer_config(&led_timer);

    //初始化pwm通道
    ledc_channel_config_t ledc_pwm = 
    {
        .channel = LEDC_CHANNEL_0,
        .duty = 0,
        .gpio_num = GPIO_NUM_43 | GPIO_NUM_44,
        .timer_sel = LEDC_TIMER_0,
    };
    ledc_channel_config(&ledc_pwm);
    //启用渐变功能，如果要调节占空比，必须启用这句
    ledc_fade_func_install(0);
}

/**
 * 下行(OneJSON):
 请求Topic:
 $sys/{pid}/{device-name}/thing/property/set
 OneJSON数据格式:
{
  "id": "123",
  "version": "1.0",
  "params": {
    "Brightness":"0"，
    "LightSwitch":"ture",
  }
}
 * 处理onenet下行的数据
 * @param property_js 包含下行数据的json
 * @return 无
 */
void onenet_property_handle(cJSON* property_js)
{
    cJSON* params_js = cJSON_GetObjectItem(property_js, "params");
    if(params_js)
    {
        cJSON* child_js = params_js->child;
        
        //cJSON遍历"params"JSON数据
        while(child_js)
        {
            if(strcmp(child_js->string, "Brightness") == 0)
            {
                led_brightness = cJSON_GetNumberValue(child_js);
                int duty = led_brightness * 4095 / 100;
                ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty, 0);
            }
            else if(strcmp(child_js->string, "LightSwitch") == 0)
            {
                if(cJSON_IsTrue(child_js))
                {
                    led_lightswitch = 1;
                    led_brightness = 50;
                    int duty = 50 * 4095 / 100;
                    ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty, 0);
                    LED0(1);
                    LED1(1);
                }
                else 
                {
                    led_lightswitch = 0;
                    led_brightness = 0;
                    ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0, 0);
                    LED0(0);
                    LED1(0);
                }
            }
            child_js = child_js->next;
        }
    }

}

/**上行(OneJSON）
请求Topic:
$sys/{pid}/{device-name}/thing/property/post
OneJSON数据格式:
{
  "id": "123",
  "version": "1.0",
  "params": {
    "Brightness": {
      "value": 0
    },
    "LightSwitch": {
      "value": ture
    }
  }
}
 * 生成上报所有数据的cJSON对象
 * @param 无
 * @return cJSON对象，包含所有属性值
 */
cJSON* onenet_property_upload_dm(void)
{
    //创建id，version键值对
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "id", "123");
    cJSON_AddStringToObject(root, "version", "1.0");
    //创建params对象
    cJSON* params_js = cJSON_AddObjectToObject(root, "params");
    //创建brightness和LightSwitch对象
    cJSON* brightness_js = cJSON_AddObjectToObject(params_js, "Brightness");
    cJSON* lightswitch_js = cJSON_AddObjectToObject(params_js, "LightSwitch");
    //创建brightness和LightSwitch的键值对
    cJSON_AddNumberToObject(brightness_js, "value", led_brightness);
    cJSON_AddBoolToObject(lightswitch_js, "value", (cJSON_bool)led_lightswitch);

    return root;
}