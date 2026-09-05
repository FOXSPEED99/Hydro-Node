/*
 * hn_report.h - serial output.
 *
 * Two views of the same struct:
 *
 *   the human block  - for a technician on a laptop next to the tank
 *   the machine line - one line of key=value pairs, which is the shape the
 *                      Section 2 LoRa payload gets packed from
 *
 * Keeping both means the bench view and the wire format can never drift apart
 * without somebody noticing.
 *
 * Everything here compiles to nothing when HN_SERIAL_ENABLED is 0.
 */
#ifndef HN_REPORT_H
#define HN_REPORT_H

#include "hn_reading.h"

void hn_report_begin();
void hn_report_banner();
void hn_report_selftest(const hn_reading_t &r);
void hn_report_reading(const hn_reading_t &r);

/* Why we booted, and the battery at that moment. A watchdog reset here is the
 * difference between "it ran for a month" and "it hung and rebooted 40 times". */
void hn_report_boot(bool watchdogReset, uint16_t battery_mv);

/* One line per cycle. Left in even on a quiet build because for a field test
 * this is the number the whole two-year estimate rests on. */
void hn_report_battery(uint16_t battery_mv);

/* Radio bring-up result, printed once at start-up. */
void hn_report_radio(bool present);

/* One line per transmission: whether it completed, and the measured airtime,
 * which is the number the whole battery model turns on. */
void hn_report_tx(bool sent, uint8_t bytes, uint16_t airtime_ms);

#endif /* HN_REPORT_H */
