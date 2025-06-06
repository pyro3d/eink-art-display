#include <stdio.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_http_client.h"
#include "esp_tls.h"
#include "esp_tls.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_sleep.h"
#include "mqtt_client.h"
#include "app.h"
#include "esp_adc/adc_oneshot.h"
#include "soc/clk_tree_defs.h"
#if CONFIG_EPD_133INE
#include "EPD_13in3e.h"
#endif
#include "connect.h"

#include <cJSON.h>

inline int MIN(int a, int b) { return a > b ? b : a; }
inline int MAX(int a, int b) { return a > b ? a : b; }


void EPD_Display(UBYTE * buffer) {
    #if CONFIG_EPD_133INE
    power_on();
    EPD_13IN3E_Init();
    ESP_LOGI("main", "EPD Init done.");
    EPD_13IN3E_Display2((UBYTE *)buffer, (UBYTE *)buffer+(EPD_13IN3E_BUF_WIDTH));
    EPD_13IN3E_Sleep();
    power_off();
    #endif

}

EXT_RAM_BSS_ATTR char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER + 1] = {0};
static char * TAG = "main";
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read

    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD("http", "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD("http", "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD("http", "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD("http", "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD("http", "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);

            // Clean the buffer in case of a new request
            if (output_len == 0 && evt->user_data) {
                // we are just starting to copy the output data into the use
                memset(evt->user_data, 0, MAX_HTTP_OUTPUT_BUFFER);
            }
            /*
             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
             *  However, event handler can also be used in case chunked encoding is used.
             */
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // If user_data buffer is configured, copy the response into the buffer
                int copy_len = 0;
                if (evt->user_data) {
                    // The last byte in evt->user_data is kept for the NULL character in case of out-of-bound access.
                    copy_len = MIN(evt->data_len, (MAX_HTTP_OUTPUT_BUFFER - output_len));
                    if (copy_len) {
                        memcpy(evt->user_data + output_len, evt->data, copy_len);
                    }
                } else {
                    int content_len = esp_http_client_get_content_length(evt->client);
                    if (output_buffer == NULL) {
                        // We initialize output_buffer with 0 because it is used by strlen() and similar functions therefore should be null terminated.
                        output_buffer = (char *) calloc(content_len + 1, sizeof(char));
                        output_len = 0;
                        if (output_buffer == NULL) {
                            ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                            return ESP_FAIL;
                        }
                    }
                    copy_len = MIN(evt->data_len, (content_len - output_len));
                    if (copy_len) {
                        memcpy(output_buffer + output_len, evt->data, copy_len);
                    }
                }
                output_len += copy_len;
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD("httpc", "HTTP_EVENT_ON_FINISH");
                if (output_buffer != NULL) {
                ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI("httpc", "HTTP_EVENT_DISCONNECTED");
            if (output_buffer != NULL) {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD("httpc", "HTTP_EVENT_REDIRECT");
            esp_http_client_set_redirection(evt->client);
            break;
    }
    return ESP_OK;
}
char * copy_until_null(char * dst, char * src, int len) {
    for (int i=0; (i<len && *src); i++) {
       dst[i] = *src;
       src++;
    }
    return ++src;

 }
static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
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
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA on %.*s",event->topic_len, event->topic);
        if (strnstr(event->topic, "art_display/source",event->topic_len))
        {
            strncpy(app_config.art_source, event->data, event->data_len);
        }
        else if(strnstr(event->topic, "art_display/type_oil", event->topic_len)) 
        {
            if( strnstr(event->data, "OFF" ,event->data_len)) {
                app_config.oil = false;
            }
            else {
                app_config.oil = true;
            }
        }
        else if(strnstr(event->topic, "type_landscape", event->topic_len)) 
        {
            if( strnstr(event->data, "OFF", event->data_len)) {
                app_config.landscape = false;
            }
            else {
                app_config.landscape = true;
            }
        }
        else if(strnstr(event->topic, "update_interval", event->topic_len)) 
        {
            app_config.update_interval = strtol(event->data, NULL, 10);
        }
        app_config.updated = true;
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}
void app_main(void)
{
    // System initialization
    ESP_ERROR_CHECK(nvs_flash_init());
        // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    adc_unit_t batt_voltage_unit;
    adc_channel_t batt_voltage_channel;
    adc_oneshot_io_to_channel(BATT_VOLTAGE_GPIO, &batt_voltage_unit, &batt_voltage_channel);
    ESP_LOGW("temp", "ADC Unit is %d, ADC Channel is %d", batt_voltage_unit, batt_voltage_channel);
    adc_oneshot_unit_handle_t batt_voltage_handle;
    adc_oneshot_unit_init_cfg_t batt_voltage_handle_cfg;
    batt_voltage_handle_cfg.unit_id = batt_voltage_unit;
    batt_voltage_handle_cfg.clk_src =  ADC_DIGI_CLK_SRC_DEFAULT;
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&batt_voltage_handle_cfg, &batt_voltage_handle));
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_0,
    };
    int voltage;
    ESP_ERROR_CHECK(adc_oneshot_read(batt_voltage_handle, batt_voltage_channel, &voltage));
    float battery_voltage = ((float)voltage) * (4.2/4095);
    ESP_LOGW("temp", "Voltage Read? %d %f", voltage, battery_voltage);
    ESP_ERROR_CHECK( err );
    nvs_handle_t nvs_handle;
    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &nvs_handle));
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    size_t art_source_size = 0;
    err = nvs_get_str(nvs_handle, "art_source", app_config.art_source, art_source_size);
    if (err == ESP_ERR_NVS_NOT_FOUND ) ESP_LOGW(TAG, "%s not in config", "art_source");
    err = nvs_get_u8(nvs_handle, "oil", (uint8_t*)(&app_config.oil));
    if (err == ESP_ERR_NVS_NOT_FOUND ) ESP_LOGW(TAG, "%s not in config", "oil");
    err = nvs_get_u8(nvs_handle, "landscape", (uint8_t*)(&app_config.landscape));
    if (err == ESP_ERR_NVS_NOT_FOUND ) ESP_LOGW(TAG, "%s not in config", "landscape");
    err = nvs_get_u32(nvs_handle, "update_interval", &app_config.update_interval);
    if (err == ESP_ERR_NVS_NOT_FOUND ) ESP_LOGW(TAG, "%s not in config", "update_interval");
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = CONFIG_BROKER_URL,
        .credentials = {
            .username = CONFIG_BROKER_USERNAME,
            .authentication = {
                .password = CONFIG_BROKER_PASSWORD,
            }
        },
    };
    esp_mqtt_client_handle_t mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_sleep_enable_ext0_wakeup(WAKEUP_SWITCH_GPIO, 1);
    init_spi();
    ESP_LOGI(TAG, "SPI Init done.");
    while(true) {
        example_connect();
        app_config.updated = false;
        esp_mqtt_client_start(mqtt_client);
        esp_mqtt_client_subscribe_multiple(mqtt_client, topics, 4);
        ESP_LOGI(TAG, "Waiting for config to be updated or timeout...");
        for (int i = 0; (i < 100 && !app_config.updated); i++) vTaskDelay(10/portTICK_PERIOD_MS);
        if (!app_config.updated) ESP_LOGW(TAG, "Could not update config via MQTT!!!");
        char url[160] = {};

        sprintf(url, "%s/image?format=binary&source=%s&oil=%s&landscape=%s&display=%s", CONFIG_ART_CONVERTER_URL, 
            app_config.art_source, app_config.oil ? "true" : "false", app_config.landscape ? "true": "false",
            DISPLAY_API_STRING);

        ESP_LOGI(TAG,"Using URL %s", url);
        esp_http_client_config_t config = {
            .url = url,
            .timeout_ms = 20000,
            .event_handler = _http_event_handler,
            .user_data = local_response_buffer,
            .user_agent = "espwx/0.1 github.com/pyro3d",
            .disable_auto_redirect = true,
        };
        esp_http_client_handle_t client = esp_http_client_init(&config);
        esp_err_t err = esp_http_client_perform(client);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Could not contact art service!!!");
            example_disconnect();
        }
        else {
            char artist[200] = {0};
            char title[200] = {0};
            char art_url[200] = {0};
            char art_source[200] = {0};
            char * buf_ptr = local_response_buffer + (EPD_13IN3E_WIDTH * EPD_13IN3E_HEIGHT /  2);
            buf_ptr = copy_until_null(title, buf_ptr, 200);
            buf_ptr = copy_until_null(artist, buf_ptr, 200);
            buf_ptr = copy_until_null(art_url, buf_ptr, 200);
            buf_ptr = copy_until_null(art_source, buf_ptr, 200);
            esp_mqtt_client_publish(mqtt_client,"homeassistant/sensor/art_display/title/state", title, 0, 0, 0);
            esp_mqtt_client_publish(mqtt_client,"homeassistant/sensor/art_display/artist/state", artist, 0, 0, 0);
            esp_mqtt_client_publish(mqtt_client,"homeassistant/sensor/art_display/used_source/state", art_source, 0, 0, 0);
            esp_mqtt_client_publish(mqtt_client,"homeassistant/image/art_display/image/url", art_url, 0, 0, 0);
            char battery_level_str[20] = {};
            sprintf(battery_level_str, "%d", (int)((battery_voltage-3.3)/(4.2-3.3)*100));
            printf("Battery Level = %f, String = %s\n", (battery_voltage-3.3)/(4.2-3.3)*100, battery_level_str);
            esp_mqtt_client_publish(mqtt_client,"homeassistant/sensor/art_display/battery/state", battery_level_str, 0, 0, 0);
            esp_mqtt_client_stop(mqtt_client);
            example_disconnect();
            EPD_Display((uint8_t *)local_response_buffer);
            ESP_LOGI("HTTP", "Artist: %s Title: %s", artist, title);
            ESP_LOGI("HTTP", "Source: %s", art_source);
            ESP_LOGI("HTTP", "URL: %s", art_url);
        }
        nvs_set_str(nvs_handle, "art_source", app_config.art_source);
        nvs_set_u8(nvs_handle, "oil", (uint8_t)(app_config.oil));
        nvs_set_u8(nvs_handle, "landscape", (uint8_t)(app_config.landscape));
        nvs_set_u32(nvs_handle, "update_interval", app_config.update_interval);
        esp_http_client_cleanup(client);
        ESP_LOGI(TAG, "Sleeping for %ld minutes", app_config.update_interval);
        esp_deep_sleep_try(app_config.update_interval * 60 * 1000000);
    }
}