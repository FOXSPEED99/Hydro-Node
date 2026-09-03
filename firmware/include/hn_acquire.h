/*
 * hn_acquire.h - one measurement cycle: the sequencing layer.
 *
 * This is the seam the rest of the product plugs into. Section 2 calls
 * hn_acquire_cycle() and hands the struct to LoRaManager; Section 3 wraps the
 * call in a sleep cycle. Neither has to know anything about capture registers
 * or 1-Wire timing, and neither should ever need to modify a sensor module.
 */
#ifndef HN_ACQUIRE_H
#define HN_ACQUIRE_H

#include "hn_reading.h"

/* Bring up all three sensor modules. Call after hn_board_begin(). */
void hn_acquire_begin();

/* Probe all three sensors without taking a measurement. Used for the start-up
 * report so a technician sees what is plugged in before any data appears. */
void hn_acquire_selftest(hn_reading_t &r);

/* One full acquisition. Takes roughly 400 ms. */
void hn_acquire_cycle(hn_reading_t &r);

#endif /* HN_ACQUIRE_H */
