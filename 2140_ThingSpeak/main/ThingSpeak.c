
/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_event_loop.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "errno.h"

#include "lwip/inet.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "dht.h"
#include "driver/gpio.h"
static const char *TAG = "example";

/* The examples use simple WiFi configuration that you can set via
   'make menuconfig'.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_WIFI_SSID CONFIG_WIFI_SSID
#define EXAMPLE_WIFI_PASS CONFIG_WIFI_PASSWORD
#define DHT_GPIO CONFIG_DHT_GPIO

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

ip4_addr_t ip;
ip4_addr_t gw;
ip4_addr_t msk;

#define WEB_SERVER "api.thingspeak.com"
#define WEB_PORT 80
#define WEB_URL "http://api.thingspeak.com/update"

char szURL[] = "api.thingspeak.com";
char szPath[] = "/update";
char szTemplate[] = "api_key=WZXH5HH9AC2CH3RI&field1=%0.2f&field2=%0.2f\r\n";
char szData[128];

#define MAXLINE 8192
char sendline[MAXLINE + 1], recvline[MAXLINE + 1];



struct addrinfo *Get_HostIP( const char *url )
{
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    static struct addrinfo *res;

    int err = getaddrinfo(url, "80", &hints, &res);

    if(err != 0 || res == NULL) {
        ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
        vTaskDelay(1000 / portTICK_RATE_MS);
        return NULL;
    }

    /* Code to print the resolved IP.

       Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
    struct in_addr *addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
    ESP_LOGI(TAG, "DNS lookup for %s succeeded.", url);
    ESP_LOGI(TAG, "IP=%s", inet_ntoa(*addr));

    return res;
}

void HTTP_Get( struct addrinfo *res  )
{
	float f_humidity, f_temp;

    if (!dht_read_float_data(DHT_GPIO, &f_humidity, &f_temp)) {
    	printf( "ERROR\n" );
    	return;
    }
    struct in_addr *addr;
    int s, r;
    char recv_buf[64];

    addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
    ESP_LOGI(TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

    s = socket(res->ai_family, res->ai_socktype, 0);
    if(s < 0) {
        ESP_LOGE(TAG, "... Failed to allocate socket.");
        freeaddrinfo(res);
        vTaskDelay(1000 / portTICK_RATE_MS);
        return;
    }
    ESP_LOGI(TAG, "... allocated socket\r\n");

    if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
        ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
        close(s);
        freeaddrinfo(res);
        vTaskDelay(4000 / portTICK_RATE_MS);
        return;
    }

    ESP_LOGI(TAG, "... connected");

    printf( "Temperature: %0.2fC, Humidity: %0.2f%%\n", f_temp, f_humidity );
    sprintf( szData, szTemplate, f_temp, f_humidity );

    sprintf( sendline,
             "POST %s HTTP/1.0\r\n\
Host: %s\r\n\
Content-type: application/x-www-form-urlencoded\r\n\
Content-length: %d\r\n\r\n", szPath, szURL, strlen(szData));

        strcat( sendline, szData );
        uint32_t len = strlen(sendline);

    if (write(s, sendline, len) < 0) {
        ESP_LOGE(TAG, "... socket send failed");
        close(s);
        vTaskDelay(4000 / portTICK_RATE_MS);
        return;
    }
    ESP_LOGI(TAG, "... socket send success");

    /* Read HTTP response */
    do {
        bzero(recv_buf, sizeof(recv_buf));
        r = read(s, recv_buf, sizeof(recv_buf)-1);
        for(int i = 0; i < r; i++) {
            putchar(recv_buf[i]);
        }
    } while(r > 0);

    ESP_LOGI(TAG, "... done reading from socket. Last read return=%d errno=%d\r\n", r, errno);
    close(s);
    for(int countdown = 10; countdown >= 0; countdown--) {
        ESP_LOGI(TAG, "%d... ", countdown);
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
    ESP_LOGI(TAG, "Finish!");
}

void main_task(void *pvParameter)
{
    /* Wait for the callback to set the CONNECTED_BIT in the
       event group.
    */
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                        false, true, portMAX_DELAY);
    ESP_LOGI(TAG, "Connected to AP");
    printf("Got IP: %s\n", inet_ntoa( ip ) );
    printf("Net mask: %s\n", inet_ntoa( msk ) );
    printf("Gateway: %s\n", inet_ntoa( gw ) );

    struct addrinfo *res = Get_HostIP( WEB_SERVER );


    gpio_pad_select_gpio(DHT_GPIO);
    /* Set the GPIO as a input */
    gpio_set_direction(DHT_GPIO, GPIO_MODE_INPUT);
    /* Set the GPIO pull */
    gpio_set_pull_mode(DHT_GPIO, GPIO_FLOATING);

    while(1) {
    	//printf( "Loop\n" );
        vTaskDelay(1000 / portTICK_RATE_MS);
        HTTP_Get( res );
    }
    freeaddrinfo(res);
}

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
    	printf( ">>> WiFi started.\n" );
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
    	printf( ">>> WiFi connected and got IP.\n" );
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        ip = event->event_info.got_ip.ip_info.ip;
        gw = event->event_info.got_ip.ip_info.gw;
        msk = event->event_info.got_ip.ip_info.netmask;

        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
    	printf( ">>> WiFi disconnected.\n" );
        /* This is a workaround as ESP32 WiFi libs don't currently
           auto-reassociate. */
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_SCAN_DONE:
    	printf( ">>> Scan done.\n" );
    	break;
    default:
        break;
    }
    return ESP_OK;
}

static void initialise_wifi(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );

    wifi_config_t wifi_config = {
		.sta = {
			.ssid = EXAMPLE_WIFI_SSID,
			.password = EXAMPLE_WIFI_PASS,
		},
    };
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);

    ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

void app_main()
{
    nvs_flash_init();
    system_init();
    initialise_wifi();

    uint8_t mac[6];
    system_efuse_read_mac(mac);
    printf("MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
        (int)mac[0], (int)mac[1], (int)mac[2], (int)mac[3], (int)mac[4], (int)mac[5]);

    xTaskCreate(&main_task, "main_task", 2048, NULL, 5, NULL);
}
