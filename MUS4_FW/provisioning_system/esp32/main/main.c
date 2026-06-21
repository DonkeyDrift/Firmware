#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_server.h"
#include "driver/uart.h"

#include "web_ui.h" // 包含HTML前端代码

static const char *TAG = "ESP32_PROVISIONING";

#define AP_SSID "MUS4-AP"
#define UART_NUM UART_NUM_1
#define TXD_PIN (17)
#define RXD_PIN (16)
#define BUF_SIZE (1024)

// 共享的配网状态
char current_prov_status[64] = "IDLE";

/* UART 初始化 */
static void init_uart(void) {
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM, &uart_config);
    uart_set_pin(UART_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

/* UART 接收任务 */
static void uart_rx_task(void *arg) {
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
    while (1) {
        int len = uart_read_bytes(UART_NUM, data, BUF_SIZE - 1, 20 / portTICK_PERIOD_MS);
        if (len > 0) {
            data[len] = '\0';
            ESP_LOGI(TAG, "UART RX: %s", (char*)data);
            
            // 简单的字符串提取，去除换行符
            char* newline = strchr((char*)data, '\n');
            if (newline) *newline = '\0';
            
            // 更新全局状态 (OK|IP 或 FAIL|Reason 或 STATUS|CONNECTING)
            strncpy(current_prov_status, (char*)data, sizeof(current_prov_status) - 1);
        }
    }
    free(data);
}

/* WiFi AP 初始化 */
static void wifi_init_softap(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = AP_SSID,
            .ssid_len = strlen(AP_SSID),
            .channel = 1,
            .max_connection = 4,
            .authmode = WIFI_AUTH_OPEN // 无密码便于用户连接
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi AP 启动完成: %s (IP: 192.168.4.1)", AP_SSID);
}

/* HTTP 路由处理器 */

// 1. 提供 HTML 页面
static esp_err_t get_index_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, web_ui_html, HTTPD_RESP_USE_STRLEN);
}

// 2. 接收配网表单提交 (POST /api/provision)
static esp_err_t post_provision_handler(httpd_req_t *req) {
    char buf[128];
    int ret, remaining = req->content_len;

    if (remaining >= sizeof(buf)) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    ret = httpd_req_recv(req, buf, remaining);
    if (ret <= 0) return ESP_FAIL;
    buf[ret] = '\0';

    // 简易的URL解码和参数解析 (假设格式为 ssid=XXX&pwd=YYY)
    char ssid[64] = {0};
    char pwd[64] = {0};
    
    char *ssid_ptr = strstr(buf, "ssid=");
    char *pwd_ptr = strstr(buf, "pwd=");
    
    if (ssid_ptr && pwd_ptr) {
        sscanf(ssid_ptr, "ssid=%63[^&]", ssid);
        sscanf(pwd_ptr, "pwd=%63s", pwd);
        
        // 构造协议字符串 WIFI|SSID|PASSWORD\n
        char uart_cmd[128];
        snprintf(uart_cmd, sizeof(uart_cmd), "WIFI|%s|%s\n", ssid, pwd);
        
        // 发送给 Linux 主机
        uart_write_bytes(UART_NUM, uart_cmd, strlen(uart_cmd));
        ESP_LOGI(TAG, "已发送配网指令: WIFI|%s|***", ssid);
        
        // 重置状态
        strcpy(current_prov_status, "STATUS|CONNECTING");
        
        httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
    } else {
        httpd_resp_send(req, "FAIL", HTTPD_RESP_USE_STRLEN);
    }
    return ESP_OK;
}

// 3. 轮询状态接口 (GET /api/status)
static esp_err_t get_status_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/plain");
    return httpd_resp_send(req, current_prov_status, HTTPD_RESP_USE_STRLEN);
}

/* 启动 HTTP 服务 */
static void start_webserver(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t uri_get = { .uri = "/", .method = HTTP_GET, .handler = get_index_handler, .user_ctx = NULL };
        httpd_uri_t uri_post = { .uri = "/api/provision", .method = HTTP_POST, .handler = post_provision_handler, .user_ctx = NULL };
        httpd_uri_t uri_status = { .uri = "/api/status", .method = HTTP_GET, .handler = get_status_handler, .user_ctx = NULL };

        httpd_register_uri_handler(server, &uri_get);
        httpd_register_uri_handler(server, &uri_post);
        httpd_register_uri_handler(server, &uri_status);
        ESP_LOGI(TAG, "HTTP 服务启动在端口: %d", config.server_port);
    }
}

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    init_uart();
    xTaskCreate(uart_rx_task, "uart_rx_task", 2048, NULL, 10, NULL);
    
    wifi_init_softap();
    start_webserver();
}
