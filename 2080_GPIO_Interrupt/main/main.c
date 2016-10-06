
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

void gpioCallback( void *arg )
{
	printf( "Interrupt in %s\n", (char *)arg );

	//GPIO intr process
	uint32_t gpio_num = 0;
	uint32_t gpio_intr_status = READ_PERI_REG(GPIO_STATUS_REG);   //read status to get interrupt status for GPIO0-31
	uint32_t gpio_intr_status_h = READ_PERI_REG(GPIO_STATUS1_REG);//read status1 to get interrupt status for GPIO32-39
	SET_PERI_REG_MASK(GPIO_STATUS_W1TC_REG, gpio_intr_status);    //Clear intr for gpio0-gpio31
	SET_PERI_REG_MASK(GPIO_STATUS1_W1TC_REG, gpio_intr_status_h); //Clear intr for gpio32-39
	do {
		if(gpio_num < 32) {
			if(gpio_intr_status & BIT(gpio_num)) { //gpio0-gpio31
				ets_printf("Intr GPIO%d ,val: %d\n",gpio_num,gpio_get_level(gpio_num));
				//This is an isr handler, you should post an event to process it in RTOS queue.
			}
		} else {
			if(gpio_intr_status_h & BIT(gpio_num - 32)) {
				ets_printf("Intr GPIO%d, val : %d\n",gpio_num,gpio_get_level(gpio_num));
				//This is an isr handler, you should post an event to process it in RTOS queue.
			}
		}
	} while(++gpio_num < GPIO_PIN_COUNT);
}

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

    gpio_set_intr_type(INPUT_GPIO, GPIO_INTR_NEGEDGE);

    gpio_intr_enable(INPUT_GPIO);

    // Intterrupt number see below
    gpio_isr_register(17, gpioCallback, (void *)TAG);


    while(1) {
    	printf( "Loop...\n" );
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}

void app_main()
{
    nvs_flash_init();
    system_init();

    xTaskCreate(&main_task, "main_task", 2048, NULL, 5, NULL);
}


//interrupt cpu using table, Please see the core-isa.h
/*************************************************************************************************************
 *      Intr num                Level           Type                    PRO CPU usage           APP CPU uasge
 *      0                       1               extern level            WMAC                    Reserved
 *      1                       1               extern level            BT/BLE Host             Reserved
 *      2                       1               extern level            FROM_CPU                FROM_CPU
 *      3                       1               extern level            TG0_WDT                 Reserved
 *      4                       1               extern level            WBB
 *      5                       1               extern level            Reserved
 *      6                       1               timer                   FreeRTOS Tick(L1)       FreeRTOS Tick(L1)
 *      7                       1               software                Reserved                Reserved
 *      8                       1               extern level            Reserved
 *      9                       1               extern level
 *      10                      1               extern edge             Internal Timer
 *      11                      3               profiling
 *      12                      1               extern level
 *      13                      1               extern level
 *      14                      7               nmi                     Reserved                Reserved
 *      15                      3               timer                   FreeRTOS Tick(L3)       FreeRTOS Tick(L3)
 *      16                      5               timer
 *      17                      1               extern level
 *      18                      1               extern level
 *      19                      2               extern level
 *      20                      2               extern level
 *      21                      2               extern level
 *      22                      3               extern edge
 *      23                      3               extern level
 *      24                      4               extern level
 *      25                      4               extern level            Reserved                Reserved
 *      26                      5               extern level            Reserved                Reserved
 *      27                      3               extern level            Reserved                Reserved
 *      28                      4               extern edge
 *      29                      3               software                Reserved                Reserved
 *      30                      4               extern edge             Reserved                Reserved
 *      31                      5               extern level            Reserved                Reserved
 *************************************************************************************************************
 */

