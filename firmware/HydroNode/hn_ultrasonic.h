/*
 * hn_ultrasonic.h - RCWL-1670 on J3, timed with Timer1 input capture.
 *
 * The echo line lands on D8 = PB0 = ICP1 because that is the ATmega328P's
 * hardware input capture pin. The capture unit timestamps both edges of the
 * echo pulse in hardware, with no interrupt-latency jitter, and it lets the CPU
 * idle-sleep through the flight time instead of spinning inside pulseIn().
 *
 * OWNERSHIP: this module owns Timer1 while a measurement is in flight, and
 * powers the timer down between bursts. Nothing else may use Timer1, and
 * analogWrite() must not be called on D9 or D10.
 *
 * The module's VCC comes straight from BATT+ and cannot be switched, so unlike
 * the DS18B20 there is no rail to drop between readings. Its 1.5 uA standby is
 * budgeted for.
 */
#ifndef HN_ULTRASONIC_H
#define HN_ULTRASONIC_H

#include "hn_reading.h"

void hn_ultrasonic_begin();

/* Probe whether anything is driving the echo line, without triggering.
 * Cheap enough to call on its own for a start-up self-test. */
hn_presence_t hn_ultrasonic_probe_presence();

/* Take HN_US_SAMPLES measurements, filter them, and classify the result.
 * Blocks for roughly HN_US_SAMPLES * HN_US_SAMPLE_GAP_MS. */
void hn_ultrasonic_read(hn_ultrasonic_reading_t &r);

#endif /* HN_ULTRASONIC_H */
