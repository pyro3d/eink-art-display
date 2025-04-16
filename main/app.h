#pragma once

#define MAX_HTTP_RECV_BUFFER 1024
#define MAX_HTTP_OUTPUT_BUFFER 4000000

#define DEVICE_SETTINGS_STR "\"device\": {\"identifiers\": [\"01ad\"], \"manufacturer\": \"pyro3d\", \"sw_version\": \"v0.0.1\", \"model\": \"ESP32-S3&EPD13.3E\", \"name\": \"Art Display\" }"
#define MQTT_TOPIC_TEMPLATE(Type,UID) "homeassistant/"Type"/art_display/"UID
#define MQTT_CMD_DISCOVERY_TEMPLATE(Type, Name, UID) "{\"name\": \""Name"\", \"command_topic\": \"homeassistant/"Type"/art_display/"UID"/command\",\"optimistic\": true, \"unique_id\": art_"UID"\","DEVICE_SETTINGS_STR

#define WAKEUP_SWITCH_GPIO 21

bool updated_settings = false;
typedef struct config_struct {
    char art_source[20];
    bool oil;
    bool landscape;
    bool updated;
    uint32_t update_interval;
} config_t;

config_t app_config = {
    .art_source = {},
    .oil = true,
    .landscape = true,
    .updated = false,
    .update_interval = 60
};
