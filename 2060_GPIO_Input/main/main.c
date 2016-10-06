
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

static const char *TAG = "example";

/* Can run 'make menuconfig' to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define INPUT_GPIO CONFIG_INPUT_GPIO

int last_status = -1;

void main_task(void *pvParameter)
{
    /* Configure the IOMUX register for pad BLINK_GPIO (some pads are
       muxed to GPIO on reset already, but some default to other
       functions and need to be switched to GPIO. Consult the
       Technical Reference for a list of pads and their default
       functions.)
    */
    gpio_pad_select_gpio(INPUT_GPIO);
    /* Set the GPIO as a input */
    gpio_set_direction(INPUT_GPIO, GPIO_MODE_INPUT);
    /* Set the GPIO pull */
    gpio_set_pull_mode(INPUT_GPIO, GPIO_PULLUP_ONLY);

    while(1) {
        int status = GPIO_INPUT_GET(GPIO_ID_PIN(INPUT_GPIO));

        if( status != last_status ) {
            printf("INPUT PIN%i: %i\n", INPUT_GPIO, status);
            last_status = status;
        }
    }
}

void app_main()
{
    nvs_flash_init();
    system_init();

    xTaskCreate(&main_task, "main_task", 2048, NULL, 5, NULL);
}
