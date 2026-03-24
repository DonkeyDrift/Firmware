#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_server.h"
#include "driver/gpio.h"
#include "cJSON.h"  // 引入 JSON 解析库

static const char *TAG = "MULTI_AGENT_ESP32";

#define ESP_WIFI_SSID      "MUS4_AGENT_AP"
#define ESP_WIFI_PASS      "12345678"
#define ESP_WIFI_CHANNEL   1
#define MAX_STA_CONN       4

#define BLINK_GPIO 2 // 假设控制板载 LED，GPIO2

// 全局状态模拟
static int current_sensor_value = 0;
static bool led_state = false;

/* 1. WiFi AP 初始化 */
static void wifi_init_softap(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = ESP_WIFI_SSID,
            .ssid_len = strlen(ESP_WIFI_SSID),
            .channel = ESP_WIFI_CHANNEL,
            .password = ESP_WIFI_PASS,
            .max_connection = MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    if (strlen(ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi AP 启动完成. SSID:%s", ESP_WIFI_SSID);
}

/* 2. WebSocket 处理器 (处理具体控制指令) */
static esp_err_t ws_handler(httpd_req_t *req) {
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "WebSocket 握手成功，新连接已建立");
        return ESP_OK;
    }
    
    httpd_ws_frame_t ws_pkt;
    uint8_t *buf = NULL;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) return ret;
    
    if (ws_pkt.len) {
        buf = calloc(1, ws_pkt.len + 1);
        if (buf == NULL) return ESP_ERR_NO_MEM;
        ws_pkt.payload = buf;
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "收到 WS 消息: %s", ws_pkt.payload);
            
            // 解析指令
            char response_str[128];
            if (strstr((char *)ws_pkt.payload, "LED_ON") != NULL) {
                led_state = true;
                gpio_set_level(BLINK_GPIO, 1);
                sprintf(response_str, "{\"event\":\"led_update\", \"state\": \"ON\"}");
            } 
            else if (strstr((char *)ws_pkt.payload, "LED_OFF") != NULL) {
                led_state = false;
                gpio_set_level(BLINK_GPIO, 0);
                sprintf(response_str, "{\"event\":\"led_update\", \"state\": \"OFF\"}");
            }
            else if (strstr((char *)ws_pkt.payload, "GET_SENSOR") != NULL) {
                sprintf(response_str, "{\"event\":\"sensor_data\", \"value\": %d}", current_sensor_value);
            }
            else {
                sprintf(response_str, "{\"event\":\"error\", \"msg\": \"Unknown command\"}");
            }

            // 发送 JSON 响应
            httpd_ws_frame_t ws_resp;
            memset(&ws_resp, 0, sizeof(httpd_ws_frame_t));
            ws_resp.payload = (uint8_t *)response_str;
            ws_resp.len = strlen(response_str);
            ws_resp.type = HTTPD_WS_TYPE_TEXT;
            httpd_ws_send_frame(req, &ws_resp);
        }
        free(buf);
    }
    return ret;
}

static void start_webserver(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t ws = {
            .uri        = "/ws",
            .method     = HTTP_GET,
            .handler    = ws_handler,
            .user_ctx   = NULL,
            .is_websocket = true
        };
        httpd_register_uri_handler(server, &ws);
        ESP_LOGI(TAG, "WebSocket 服务已在端口 %d 启动", config.server_port);
    }
}

/* 3. 传感器数据采集任务 (FreeRTOS) */
void sensor_task(void *pvParameters) {
    while (1) {
        // 模拟读取 ADC 数据或 I2C 传感器
        current_sensor_value = esp_random() % 1024; // 0-1023 随机数模拟 ADC
        ESP_LOGD(TAG, "[Sensor Task] 采集到新数据: %d", current_sensor_value);
        vTaskDelay(pdMS_TO_TICKS(1000)); // 1Hz 采样率
    }
}

void app_main(void) {
    // 初始化 NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 初始化 GPIO
    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(BLINK_GPIO, 0);

    ESP_LOGI(TAG, "=== 嵌入式智能体框架生成的 ESP32 固件 (增强版) ===");
    
    wifi_init_softap();
    start_webserver();

    // 启动独立的数据采集任务
    xTaskCreate(sensor_task, "sensor_task", 2048, NULL, 5, NULL);
}
