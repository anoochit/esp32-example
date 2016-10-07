
/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "dht.h"

static const char *TAG = "example";

/* Can run 'make menuconfig' to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define DHT_GPIO CONFIG_DHT_GPIO

void main_task(void *pvParameter)
{
    /* Configure the IOMUX register for pad BLINK_GPIO (some pads are
       muxed to GPIO on reset already, but some default to other
       functions and need to be switched to GPIO. Consult the
       Technical Reference for a list of pads and their default
       functions.)
    */
    gpio_pad_select_gpio(DHT_GPIO);
    /* Set the GPIO as a input */
    gpio_set_direction(DHT_GPIO, GPIO_MODE_INPUT);
    /* Set the GPIO pull */
    gpio_set_pull_mode(DHT_GPIO, GPIO_FLOATING);


    //PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO16_U, FUNC_GPIO16_GPIO16);
    //PIN_PULLDWN_DIS(PERIPHS_IO_MUX_GPIO16_U);
    //PIN_PULLUP_DIS(PERIPHS_IO_MUX_GPIO16_U);
    //GPIO_DIS_OUTPUT(GPIO_ID_PIN(16));

    while(1) {
        float f_humidity, f_temp;

        if (dht_read_float_data(GPIO_ID_PIN(16), &f_humidity, &f_temp)) {
            printf( "Temperature: %0.2f, Humidity: %0.2f\n", f_temp, f_humidity );
        }
        else {
            printf( "ERROR\n" );
        }

        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}

void app_main()
{
    nvs_flash_init();
    system_init();

    xTaskCreate(&main_task, "main_task", 2048, NULL, 5, NULL);
}

