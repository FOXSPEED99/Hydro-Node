/*
 * hn_sleep.h - Section 3. Where the two-year battery life actually comes from.
 *
 * Everything before this ran the ATmega at full speed between readings, which
 * is ~4 mA continuous. Two LS14500 cells would last about six weeks like that.
 * In SLEEP_MODE_PWR_DOWN with the watchdog as the only thing running, the same
 * chip draws ~4.5 uA - a factor of nearly a thousand.
 *
 * This module replaces the body of hn_delay_ms(), which is why every wait in
 * the firmware was routed through that one function from the beginning.
 * Nothing in the sensor layer changes.
 *
 * WHAT THIS COSTS IN ACCURACY: the watchdog runs from its own ~128 kHz RC
 * oscillator, not the resonator, and that oscillator drifts with temperature
 * and supply - roughly +/-10%. A nominal 120 s cycle is really 108-132 s, and
 * it will run visibly slower on a hot roof. Nothing here depends on the
 * interval being exact: the Hub timestamps packets on arrival and the level
 * changes over minutes. Do not build anything on it that does.
 */
#ifndef HN_SLEEP_H
#define HN_SLEEP_H

#include <stdint.h>

/* Prepare the watchdog and clear any reset flag. Call once, early in setup(),
 * BEFORE anything that could hang. */
void hn_sleep_begin();

/* Power down for approximately this long, in the largest watchdog steps that
 * fit. Wakes early only if another interrupt fires. */
void hn_sleep_ms(uint32_t ms);

/* Arm the reset watchdog for the awake portion of a cycle. If the firmware
 * hangs for longer than the timeout the chip resets rather than sitting there
 * flat until somebody visits the roof. */
void hn_sleep_guard_start();
void hn_sleep_guard_kick();
void hn_sleep_guard_stop();

/* True if the last boot was caused by the watchdog rather than power-on. Worth
 * knowing after a month: it is the difference between "it ran perfectly" and
 * "it hung and quietly rebooted 40 times". */
bool hn_sleep_was_watchdog_reset();

#endif /* HN_SLEEP_H */
