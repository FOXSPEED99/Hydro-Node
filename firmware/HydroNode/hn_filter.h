/*
 * hn_filter.h - the decision logic, deliberately separated from the hardware.
 *
 * Everything in here is a pure function of its inputs: no port access, no
 * timers, no delays. That is not an aesthetic choice. Sample filtering and
 * fault classification are the parts of this firmware most likely to be wrong
 * and least likely to be caught on a bench - a median that mishandles an
 * outlier, or a chattering switch misreported as a wet connector, both look
 * plausible on a serial monitor. Keeping them here means they can be exercised
 * on a host machine against known inputs:
 *
 *     make test
 *
 * The sensor modules collect raw samples and hand them to these functions.
 */
#ifndef HN_FILTER_H
#define HN_FILTER_H

#include <stdint.h>
#include "hn_reading.h"

/*
 * Ultrasonic: median, then outlier rejection around it, then mean of the
 * survivors, then classification.
 *
 * `valid` holds only the samples already inside the module's physical range,
 * so n_valid may be less than the number of triggers issued. The caller must
 * have filled in r.presence, r.echo_stuck_high and r.rise_without_fall first -
 * they are what separates "unplugged" from "broken" from "nothing came back",
 * and that decision is made here so it can be tested.
 */
void hn_filter_ultrasonic(const uint16_t *valid, uint8_t n_valid,
                          hn_ultrasonic_reading_t &r);

/*
 * Flow switch: vote across the digital samples, classify each analogue sample
 * against the rails, cross-check the two, and decide state, presence and
 * status. `n` must be at least 1.
 */
void hn_classify_flow(const uint16_t *adc, const bool *digital_high,
                      uint8_t n, hn_flow_reading_t &r);

#endif /* HN_FILTER_H */
