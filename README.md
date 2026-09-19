# ESP32-S3 OneNET 物模型 MQTT 控制

基于 ESP32-S3 与 ESP-IDF 开发的物联网示例工程，通过 MQTT 协议接入中国移动 OneNET 平台，使用**物模型（OneJSON）**实现云端属性下发控制板载 LED。

> **当前进度**：已实现物模型属性下发，控制两个 LED 同时亮灭。

---

## 功能特性

- **WiFi STA 连接**：带状态变化回调，配合 FreeRTOS 事件组与主流程同步
- **OneNET MQTT 鉴权**：本地计算设备 token（HMAC-SHA256），不依赖平台侧预生成
- **属性下发**：订阅 `property/set` 主题，解析 OneJSON 并驱动硬件，回复 `set_reply`
- **属性上报**：通过 `property/post` 上报当前属性值
- **OTA 辅助**：读取当前运行分区版本号、标记应用合法性（配合回滚机制）

### 已实现 / 待实现

| 功能 | 状态 |
| --- | --- |
| WiFi 连接与断线状态回调 | ✅ 已完成 |
| MQTT 接入 OneNET（token 鉴权） | ✅ 已完成 |
| 物模型属性下发 `LightSwitch` 控制 LED | ✅ 已完成 |
| 物模型属性上报 | ✅ 已完成 |
| 属性下发 `Brightness` 调光 | ⏳ 已解析，PWM 输出尚未接通 |
| 按键本地控制 | ⏳ 硬件已初始化，逻辑未实现 |

---

## 物模型定义

在 OneNET 控制台按如下定义物模型：

| 标识符 | 数据类型 | 读写 | 取值范围 | 说明 |
| --- | --- | --- | --- | --- |
| `LightSwitch` | bool | 读写 | true / false | 开关，控制 LED0 与 LED1 同时亮灭 |
| `Brightness` | int32 | 读写 | 0 ~ 100 | 亮度百分比（当前仅解析，未驱动 PWM） |

### 下行数据格式（云端 → 设备）

主题：`$sys/{pid}/{device-name}/thing/property/set`

```json
{
  "id": "123",
  "version": "1.0",
  "params": {
    "Brightness": 50,
    "LightSwitch": true
  }
}
```

### 上行数据格式（设备 → 云端）

主题：`$sys/{pid}/{device-name}/thing/property/post`

```json
{
  "id": "123",
  "version": "1.0",
  "params": {
    "Brightness": { "value": 50 },
    "LightSwitch": { "value": true }
  }
}
```

### 下行响应格式

主题：`$sys/{pid}/{device-name}/thing/property/set_reply`

```json
{
  "id": "123",
  "code": 200,
  "msg": "property success!!!"
}
```

---

## 硬件连接

| 外设 | 引脚 | 说明 |
| --- | --- | --- |
| LED0 | GPIO43 | 板载 LED |
| LED1 | GPIO44 | 板载 LED |
| I2C SDA | GPIO38 | XL9555 数据线 |
| I2C SCL | GPIO48 | XL9555 时钟线 |
| XL9555 INT | GPIO46 | IO 扩展中断（当前未启用） |

- **主控**：ESP32-S3
- **IO 扩展**：XL9555（I2C 地址 `0x20`，400 kHz，用于扩展按键等外设）
- **PWM**：LEDC Timer0 / Channel0，12 位分辨率，5 kHz

---

## 目录结构

```
08_onenet/
├── CMakeLists.txt                  # 顶层工程配置，注册 BSP / Middlewares 组件目录
├── sdkconfig                       # 工程配置（已被 .gitignore 忽略）
├── .gitignore
├── main/
│   └── main.c                      # 入口：NVS → WiFi → 事件组 → 启动 OneNET
├── components/
│   ├── BSP/
│   │   ├── LED/                    # 板载 LED 驱动（GPIO43 / GPIO44）
│   │   ├── MYIIC/                  # I2C 主机驱动（I2C0）
│   │   └── XL9555/                 # XL9555 IO 扩展芯片驱动
│   └── Middlewares/
│       └── IOT/
│           ├── onenet_secrets.h.example  # 敏感信息模板（提交到仓库）
│           ├── onenet_secrets.h          # 真实凭据（本地文件，不提交）
│           ├── wifi_manager.c/h          # WiFi STA 连接与状态回调
│           ├── onenet_mqtt.c/h           # MQTT 连接、主题订阅发布、事件处理
│           ├── onenet_control.c/h        # 物模型属性解析与上报
│           ├── onenet_token.c/h          # OneNET 鉴权 token 计算
│           └── onenet_ota.c/h            # 分区版本读取与应用合法性标记
└── .devcontainer/  .vscode/        # 开发环境配置
```

---

## 开发环境

- **ESP-IDF**：v5.5.4
- **目标芯片**：esp32s3
- **Flash**：2 MB，单应用分区（`partitions_singleapp`）

> `sdkconfig` 未纳入版本管理，克隆后需重新执行 `idf.py set-target esp32s3` 生成。

---

## 快速开始

### 1. 克隆并进入工程

```bash
git clone https://github.com/ioos455/onenet_mqtt_demo.git
cd onenet_mqtt_demo
```

### 2. 配置敏感信息

工程把产品 ID、产品密钥、设备名称、WiFi 账号密码统一放在 `components/Middlewares/IOT/onenet_secrets.h` 中，**该文件已被 `.gitignore` 忽略，不会提交到仓库**。

首次编译时，若该文件不存在，CMake 会自动从模板 `onenet_secrets.h.example` 生成一份占位版本。你也可以手动复制：

```bash
cp components/Middlewares/IOT/onenet_secrets.h.example \
   components/Middlewares/IOT/onenet_secrets.h
```

然后填入自己的真实配置：

```c
#define ONENET_PRODUCT_ID          "YOUR_PRODUCT_ID"
#define ONENET_PRODUCT_ACCESS_KEY  "YOUR_PRODUCT_ACCESS_KEY"
#define ONENET_DEVICE_NAME         "YOUR_DEVICE_NAME"
#define WIFI_SSID                  "YOUR_WIFI_SSID"
#define WIFI_PASSWORD              "YOUR_WIFI_PASSWORD"
```

> 若使用自动生成的占位版本，编译可以通过，但运行时会因凭据无效而连接失败，务必替换为真实值后重新编译。

### 3. 设置目标芯片并编译烧录

```bash
idf.py set-target esp32s3
idf.py build
idf.py -p <你的串口> flash monitor
```

Windows 下串口形如 `COM3`，Linux/macOS 下形如 `/dev/ttyUSB0`。

### 4. 验证

设备启动后会依次打印 WiFi 连接、MQTT 连接成功日志。此时在 OneNET 控制台的设备调试页下发 `LightSwitch` 属性，即可看到两个 LED 同时亮灭。

---

## MQTT 通信说明

- **Broker**：`mqtt://mqtts.heclouds.com:1883`
- **Client ID**：`ONENET_DEVICE_NAME`
- **Username**：`ONENET_PRODUCT_ID`
- **Password**：由 `dev_token_generate()` 使用产品密钥 + 设备名 + 过期时间经 HMAC-SHA256 计算得到

### 主题列表

| 方向 | 主题 |
| --- | --- |
| 订阅 | `$sys/{pid}/{dev}/thing/property/set` |
| 订阅 | `$sys/{pid}/{dev}/thing/property/post/reply` |
| 发布 | `$sys/{pid}/{dev}/thing/property/post` |
| 发布 | `$sys/{pid}/{dev}/thing/property/set_reply` |

---

## 注意事项

- 修改 Wi-Fi 密码或平台凭据时，只需改动本地 `onenet_secrets.h`，无需提交任何代码变更。
- 固件二进制（`build/*.bin`）会把凭据以明文形式编译进去，请勿单独分发编译产物。
- `onenet_token.c` 中的 token 过期时间为固定值，如需长期稳定运行，建议改为按当前时间动态计算。

---

## 后续计划

- [ ] 接通 `Brightness` 属性的 PWM 调光输出
- [ ] 实现按键本地控制 LED，并与云端状态双向同步
- [ ] 增加断线重连与属性上报周期化

---

## 许可证

本项目未指定开源许可证。如需他人使用，请补充 `LICENSE` 文件。
