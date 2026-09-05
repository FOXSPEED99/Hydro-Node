/*
 * hn_battery.h - measure the battery with no battery-measuring hardware.
 *
 * There is no divider on this board and no spare pin for one, which is
 * fortunate, because a divider across a 3.6 V pack is a permanent leak: even
 * 1 Mohm + 1 Mohm draws 1.8 uA continuously, comparable to the entire sleeping
 * MCU.
 *
 * Instead the ATmega328P measures its own supply. The ADC normally compares an
 * input against AVcc; point it at the internal 1.1 V bandgap instead and the
 * comparison runs the other way - a known voltage against an unknown
 * reference - so VCC falls out of the result. It costs nothing, draws nothing
 * between readings, and VCC here IS the battery, because the Pro Mini's
 * regulator is bypassed and VCC is wired straight to BATT+.
 *
 * ACCURACY: the bandgap is only specified to +/-10% untrimmed, so out of the
 * box this reads to about +/-0.35 V - useless for tracking a discharge curve.
 * Calibrating it per board takes one measurement and a constant; see
 * HN_BANDGAP_CAL in hn_config.h. After that it is good to ~1%.
 *
 * WHY IT MATTERS FOR A FIELD TEST: a month of data with no battery figure
 * tells you the system worked, not what it cost. This is the number the whole
 * two-year claim rests on, and it is the only way to find out whether the
 * model was right.
 */
#ifndef HN_BATTERY_H
#define HN_BATTERY_H

#include <stdint.h>

/* Supply voltage in millivolts, or 0 if the measurement is not available. */
uint16_t hn_battery_read_mv();

#endif /* HN_BATTERY_H */
