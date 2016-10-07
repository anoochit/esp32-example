/*
 * dht.h
 *
 *  Created on: Oct 7, 2016
 *      Author: esp32
 */

#ifndef DHT_DHT_H_
#define DHT_DHT_H_

#include <stdint.h>
#include <stdbool.h>

#define DHT11       11
#define DHT22       22

// Type of sensor to use
#define DHT_TYPE    DHT11

/**
 * Read data from sensor on specified pin.
 *
 * Humidity and temperature is returned as integers.
 * For example: humidity=625 is 62.5 %
 *              temperature=24.4 is 24.4 degrees Celsius
 *
 */
bool dht_read_data(uint8_t pin, int16_t *humidity, int16_t *temperature);


/**
 * Float version of dht_read_data.
 *
 * Return values as floating point values.
 */
bool dht_read_float_data(uint8_t pin, float *humidity, float *temperature);

#endif /* DHT_DHT_H_ */
