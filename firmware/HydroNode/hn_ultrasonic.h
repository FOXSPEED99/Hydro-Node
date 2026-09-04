/*
 * hn_ultrasonic.h - RCWL-1670 on J3.
 *
 * When the echo line lands on D8 = PB0 = ICP1, the ATmega328P's hardware input
 * capture unit timestamps both edges of the echo pulse with no interrupt
 * latency jitter, and lets the CPU idle-sleep through the flight time. If the
 * built harness has TRIG/ECHO swapped, the same API falls back to software
 * pulse timing on the configured echo pin.
 *
 * OWNERSHIP: when HN_PIN_US_ECHO is D8, this module owns Timer1 while a
 * measurement is in flight, and powers the timer down between bursts. Nothing
 * else may use Timer1 in that build, and analogWrite() must not be called on
 * D9 or D10.
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
