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

#endif /* HN_REPORT_H */
