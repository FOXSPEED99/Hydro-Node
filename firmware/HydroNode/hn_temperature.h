/*
 * hn_temperature.h - DS18B20 on J2, powered from a GPIO.
 *
 * The read is split into start/finish on purpose. A 9-bit conversion takes
 * ~94 ms, and the ultrasonic burst takes ~300 ms; running them one after the
 * other would keep the MCU awake for the sum of the two. hn_acquire.cpp starts
 * the conversion, fires the ultrasonic burst, and collects the temperature
 * afterwards - the conversion is free.
 *
 * read() is the sequential convenience form, used by the start-up self-test
 * where there is nothing to overlap with.
 */
#ifndef HN_TEMPERATURE_H
#define HN_TEMPERATURE_H

#include "hn_reading.h"

/* Powers the sensor briefly, reads and caches its ROM code, and leaves it
 * unpowered again. Safe to call with no sensor fitted. */
void hn_temperature_begin();

/* Power up and kick off a conversion. Returns false if the sensor did not
 * answer; `r` carries the reason. Leaves the sensor POWERED - every path out of
 * a successful start must reach hn_temperature_finish(). */
bool hn_temperature_start(hn_temperature_reading_t &r);

/* Wait for the conversion, read and validate the scratchpad, then depower.
 * Safe to call after a failed start (it just depowers). */
void hn_temperature_finish(hn_temperature_reading_t &r);

/* start() + finish(). */
void hn_temperature_read(hn_temperature_reading_t &r);

/* Decode the raw register to degrees Celsius. Diagnostics only - the raw value
 * is what gets transmitted. */
float hn_temperature_celsius(int16_t raw);

#endif /* HN_TEMPERATURE_H */
