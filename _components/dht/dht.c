/*
 * dht.c
 *
 *  Created on: Oct 7, 2016
 *      Author: esp32
 */


#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "dht.h"
#include "rom/ets_sys.h"
#include "rom/gpio.h"
#include "soc/io_mux_reg.h"

// DHT timer precision in microseconds
#define DHT_TIMER_INTERVAL   4
#define DHT_DATA_BITS  40

//#define DEBUG_DHT

#ifdef DEBUG_DHT
#define debug(fmt, ...) printf("%s" fmt "\n", "dht: ", ## __VA_ARGS__);
#else
#define debug(fmt, ...) /* (do nothing) */
#endif

/*
 *  Note:
 *  A suitable pull-up resistor should be connected to the selected GPIO line
 *
 *  __           ______          _______                              ___________________________
 *    \    A    /      \   C    /       \   DHT duration_data_low    /                           \
 *     \_______/   B    \______/    D    \__________________________/   DHT duration_data_high    \__
 *
 *
 *  Initializing communications with the DHT requires four 'phases' as follows:
 *
 *  Phase A - MCU pulls signal low for at least 18000 us
 *  Phase B - MCU allows signal to float back up and waits 20-40us for DHT to pull it low
 *  Phase C - DHT pulls signal low for ~80us
 *  Phase D - DHT lets signal float back up for ~80us
 *
 *  After this, the DHT transmits its first bit by holding the signal low for 50us
 *  and then letting it float back high for a period of time that depends on the data bit.
 *  duration_data_high is shorter than 50us for a logic '0' and longer than 50us for logic '1'.
 *
 *  There are a total of 40 data bits transmitted sequentially. These bits are read into a byte array
 *  of length 5.  The first and third bytes are humidity (%) and temperature (C), respectively.  Bytes 2 and 4
 *  are zero-filled and the fifth is a checksum such that:
 *
 *  byte_5 == (byte_1 + byte_2 + byte_3 + btye_4) & 0xFF
 *
*/

static portMUX_TYPE lock_init_dht = portMUX_INITIALIZER_UNLOCKED;

/**
 * Wait specified time for pin to go to a specified state.
 * If timeout is reached and pin doesn't go to a requested state
 * false is returned.
 * The elapsed time is returned in pointer 'duration' if it is not NULL.
 */
static bool dht_await_pin_state(uint8_t pin, uint32_t timeout,
        bool expected_pin_state, uint32_t *duration)
{
    for (uint32_t i = 0; i < timeout; i += DHT_TIMER_INTERVAL) {
        // need to wait at least a single interval to prevent reading a jitter
        ets_delay_us(DHT_TIMER_INTERVAL);
        if (GPIO_INPUT_GET(pin) == expected_pin_state) {
            if (duration) {
                *duration = i;
            }
            return true;
        }
    }

    return false;
}

static inline bool dht_fetch_data(uint8_t pin, bool bits[DHT_DATA_BITS])
{
    uint32_t low_duration;
    uint32_t high_duration;

    debug( "Phase A start\n" );
    GPIO_OUTPUT_SET(pin, 0);
    ets_delay_us( 20000 );

    // Open drain
    GPIO_OUTPUT_SET(pin, 1);
    GPIO_DIS_OUTPUT(GPIO_ID_PIN(16));

    // Step through Phase 'B', 40us
    uint32_t durationB = 0;
    if (!dht_await_pin_state(pin, 40, false, &durationB)) {
        debug("Initialization error, problem in phase 'B'\n");
        return false;
    }

    // Step through Phase 'C', 88us
    uint32_t durationC = 0;
    if (!dht_await_pin_state(pin, 88, true, &durationC)) {
        debug("Initialization error, problem in phase 'C'\n");
        return false;
    }

    // Step through Phase 'D', 88us
    uint32_t durationD = 0;
    if (!dht_await_pin_state(pin, 88, false, &durationD)) {
        debug("Initialization error, problem in phase 'D'\n");
        return false;
    }

    //debug( "Phase B,C,D OK %i %i %i\n", (int)durationB, (int)durationC, (int)durationD );
    for (int i = 0; i < DHT_DATA_BITS; i++) {
        if (!dht_await_pin_state(pin, 65, true, &low_duration)) {
            debug("LOW bit timeout\n");
            return false;
        }
        if (!dht_await_pin_state(pin, 75, false, &high_duration)){
            debug("HIGHT bit timeout\n");
            return false;
        }
        bits[i] = high_duration > low_duration;
    }

    return true;
}

/**
 * Pack two data bytes into single value and take into account sign bit.
 */
static inline int16_t dht_convert_data(uint8_t msb, uint8_t lsb)
{
    int16_t data;

#if DHT_TYPE == DHT22
    data = msb & 0x7F;
    data <<= 8;
    data |= lsb;
    if (msb & BIT(7)) {
        data = 0 - data;       // convert it to negative
    }
#elif DHT_TYPE == DHT11
    data = msb * 10;
#else
#error "Unsupported DHT type"
#endif

    return data;
}

bool dht_read_data(uint8_t pin, int16_t *humidity, int16_t *temperature)
{
    bool bits[DHT_DATA_BITS];
    uint8_t data[DHT_DATA_BITS/8] = {0};
    bool result;

    taskENTER_CRITICAL( &lock_init_dht );
    result = dht_fetch_data(pin, bits);
    taskEXIT_CRITICAL( &lock_init_dht );

    if (!result) {
        return false;
    }

    for (uint8_t i = 0; i < DHT_DATA_BITS; i++) {
        // Read each bit into 'result' byte array...
        data[i/8] <<= 1;
        data[i/8] |= bits[i];
    }

    if (data[4] != ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
        debug("Checksum failed, invalid data received from sensor\n");
        return false;
    }
    debug( "%i %i %i %i %i\n", (int)data[0], (int)data[1], (int)data[2], (int)data[3], (int)data[4] );

    *humidity = dht_convert_data(data[0], data[1]);
    *temperature = dht_convert_data(data[2], data[3]);

    debug("Sensor data: humidity=%d, temp=%d\n", *humidity, *temperature);

    return true;
}

bool dht_read_float_data(uint8_t pin, float *humidity, float *temperature)
{
    int16_t i_humidity, i_temp;

    if (dht_read_data(pin, &i_humidity, &i_temp)) {
        *humidity = (float)i_humidity / 10;
        *temperature = (float)i_temp / 10;
        return true;
    }
    return false;
}
