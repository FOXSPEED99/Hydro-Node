/*
 * Hydro Node - Section 1: sensor acquisition and connection detection.
 *
 * This file is deliberately thin. All it does is bring the board up, report
 * what is plugged in, and then repeat one acquisition cycle forever. Everything
 * that will later be shared with the LoRa, sleep and pairing sections lives in
 * a module behind a header, so those sections can be added without reopening
 * the sensor layer.
 *
 * What this firmware does NOT do, on purpose:
 *   - no LoRa                (Section 2 - the radio is parked in reset)
 *   - no sleep cycle         (Section 3 - see hn_delay_ms() in hn_board.h)
 *   - no pairing             (Section 6)
 *   - no sound               (Section 4 - the buzzer is parked low)
 *   - no interpretation of the readings. The Node measures and reports raw
 *     values; the Hub owns every conversion, threshold and decision.
 *
 * Wiring, evidence and open assumptions: docs/HARDWARE.md
 */

#include <Arduino.h>

#include "hn_board.h"
#include "hn_config.h"
#include "hn_acquire.h"
#include "hn_report.h"

static hn_reading_t g_reading;

void setup()
{
    /* Order matters. hn_board_begin() puts every pin into a defined state -
     * including holding the radio in reset and keeping the latch's shutdown
     * line high-impedance - before anything else is allowed to touch hardware. */
    hn_board_begin();

    hn_report_begin();
    hn_report_banner();

    hn_acquire_begin();

    /* Probe the sensors once and say what is there before any measurement
     * appears. A technician commissioning a unit should be able to see a
     * missing harness immediately, not infer it from odd numbers later. */
    hn_acquire_selftest(g_reading);
    hn_report_selftest(g_reading);
}

void loop()
{
    hn_acquire_cycle(g_reading);
    hn_report_reading(g_reading);

    /*
     * Section 3 replaces this with a watchdog-timed power-down of
     * HN_CYCLE_INTERVAL_MS. Nothing else in the loop has to change - which is
     * the whole point of routing every wait through hn_delay_ms().
     */
    hn_delay_ms(HN_CYCLE_INTERVAL_MS);
}
