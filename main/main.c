#include <stdio.h>
#include "esp_log.h"
#include "esp_err.h"
#include "EPD_13in3e.h"
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

#include "connect.h"

#include <cJSON.h>

#define MAX_HTTP_RECV_BUFFER 1024
#define MAX_HTTP_OUTPUT_BUFFER 4000000


inline int MIN(int a, int b) { return a > b ? b : a; }
inline int MAX(int a, int b) { return a > b ? a : b; }

EXT_RAM_BSS_ATTR char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER + 1] = {0};
#define TAG "http"
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
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
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
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

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
    esp_sleep_enable_ext0_wakeup(21, 1);
    init_spi();
    while(true) {
        example_connect();
        esp_mqtt_client_start(mqtt_client);
        ESP_LOGW("main", "SPI Init done.");
        char url[160] = {};
        sprintf(url,"http://art-converter.example.com/image?format=binary");
     //   sprintf(url,"http://192.168.88.212:8000/image?format=binary");
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
            ESP_LOGE("weather", "Weather data lookup failed!");
            example_disconnect();
        }
        else {
            char artist[200] = {0};
            char title[200] = {0};
            char art_url[200] = {0};
            char * buf_ptr = local_response_buffer + (EPD_13IN3E_WIDTH * EPD_13IN3E_HEIGHT /  2);
            printf("Buf location %p\n", buf_ptr);
            buf_ptr = copy_until_null(title, buf_ptr, 200);
            printf("Buf location %p\n", buf_ptr);
            buf_ptr = copy_until_null(artist, buf_ptr, 200);
            printf("Buf location %p\n", buf_ptr);
            buf_ptr = copy_until_null(art_url, buf_ptr, 200);
            esp_mqtt_client_publish(mqtt_client,"homeassistant/sensor/art_display/title/state", title, 0, 0, 0);
            esp_mqtt_client_publish(mqtt_client,"homeassistant/sensor/art_display/artist/state", artist, 0, 0, 0);
            esp_mqtt_client_publish(mqtt_client,"homeassistant/image/art_display/image/url", art_url, 0, 0, 0);
            power_on();
            esp_mqtt_client_stop(mqtt_client);
            example_disconnect();
            ESP_LOGW("HTTP", "Artist: %s Title: %s", artist, title);
            ESP_LOGW("HTTP", "URL: %s", art_url);
            EPD_13IN3E_Init();
            ESP_LOGW("main", "EPD Init done.");
          //  EPD_13IN3E_Clear(EPD_13IN3E_RED);
        //    EPD_13IN3E_Clear(EPD_13IN3E_BLUE);
            EPD_13IN3E_Display2((UBYTE *)local_response_buffer, (UBYTE *)local_response_buffer+(EPD_13IN3E_BUF_WIDTH));
            EPD_13IN3E_Sleep();
            power_off();
        }
        esp_http_client_cleanup(client);
//        vTaskDelay(360000 /portTICK_PERIOD_MS);
        esp_deep_sleep_try(3600000000);
    }
}