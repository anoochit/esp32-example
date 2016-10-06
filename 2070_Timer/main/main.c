
/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

int timerID = 0;

/* Can run 'make menuconfig' to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO CONFIG_BLINK_GPIO

void LED_toggle()
{
	int n = gpio_get_level( BLINK_GPIO );
	if( n ) {
	    printf( "LED on\n" );
	    /* Blink off (output high) */
	    gpio_set_level(BLINK_GPIO, 0);
	}
	else {
	    printf( "LED off\n" );
	    /* Blink off (output low) */
	    gpio_set_level(BLINK_GPIO, 1);
	}
}

void timerCallback( TimerHandle_t xTimer )
{
	printf( "Timer in\n" );
	LED_toggle();
}

void main_task(void *pvParameter)
{
    /* Configure the IOMUX register for pad BLINK_GPIO (some pads are
       muxed to GPIO on reset already, but some default to other
       functions and need to be switched to GPIO. Consult the
       Technical Reference for a list of pads and their default
       functions.)
    */
    gpio_pad_select_gpio(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_INPUT_OUTPUT);

    TimerHandle_t timerHandler = xTimerCreate( "Timer1",
    		1000 / portTICK_PERIOD_MS,
            pdTRUE,
            &timerID,
            timerCallback );
    xTimerStart( timerHandler, 0 );

    while(1) {
        vTaskDelay(2000 / portTICK_RATE_MS);
    }
}

void app_main()
{
    nvs_flash_init();
    system_init();

    xTaskCreate(&main_task, "main_task", 2048, NULL, 5, NULL);
}
