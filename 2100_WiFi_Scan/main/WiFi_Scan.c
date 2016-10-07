
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

static const char *TAG = "example";

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

uint16_t wifi_num = 0;
wifi_ap_list_t *wifi_list;

char *auth_name[] = {
    "open",
    "WEP",
    "WPA_PSK",
    "WPA2_PSK",
    "WPA_WPA2_PSK",
};

void main_task(void *pvParameter)
{
    while(1) {
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
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
    };
    ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

void scan_wifi(void)
{
    wifi_scan_config_t scan_config;
    memset(&scan_config,0,sizeof(scan_config));

    printf( "WiFi start scan\n" );
    ESP_ERROR_CHECK( esp_wifi_scan_start( &scan_config, true ) );
    printf( "WiFi scan OK\n" );

    printf( "WiFi scan done\n" );
    esp_wifi_get_ap_num( &wifi_num );
    printf( "WiFi number: %i\n", (int)wifi_num );

    wifi_list = (wifi_ap_list_t *)malloc( sizeof( wifi_ap_list_t ) * wifi_num );
    esp_wifi_get_ap_list( &wifi_num, wifi_list );
    for( int i=0; i<wifi_num; i++ )
    {
        printf( ">> %-32s : %s\n", wifi_list[i].ssid, auth_name[wifi_list[i].authmode] );
    }
}

void app_main()
{
    nvs_flash_init();
    system_init();
    initialise_wifi();
    scan_wifi();

    xTaskCreate(&main_task, "main_task", 2048, NULL, 5, NULL);
}
