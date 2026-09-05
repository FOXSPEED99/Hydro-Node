/*
 * hn_telemetry.h - turn one acquisition into one wire packet.
 *
 * Kept separate from the radio driver so it is pure logic with no hardware
 * dependency, and therefore testable on a host (make test). Getting this
 * mapping wrong is the kind of bug that shows up as the Hub displaying
 * plausible nonsense, which is much harder to spot than a dead link.
 */
#ifndef HN_TELEMETRY_H
#define HN_TELEMETRY_H

#include "hn_reading.h"
#include "hn_packet.h"

/* Pack a reading into the wire form. `pair_hash` and `node_id` come from
 * hn_config.h; they are parameters rather than globals so the tests can drive
 * them directly. */
void hn_telemetry_build(const hn_reading_t &r, uint16_t pair_hash,
                        uint16_t node_id, hn_packet_t &out);

#endif /* HN_TELEMETRY_H */
