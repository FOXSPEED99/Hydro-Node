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
 *   - no interpretation of the readings (the Hub owns every conversion)
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
#include "hn_lora.h"
#include "hn_packet.h"
#include "hn_telemetry.h"
#include "hn_sleep.h"
#include "hn_battery.h"

static hn_reading_t g_reading;

#if HN_LORA_ENABLED
/* Hashed once at start-up rather than per packet - it never changes, and the
 * CRC over a 13-character string is not free on an 8 MHz AVR. */
static uint16_t g_pair_hash;
#endif

void setup()
{
    /* First, before anything that could hang: clear the watchdog left armed by
     * a watchdog reset, and record whether that is why we are here. */
    hn_sleep_begin();

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
    hn_report_boot(hn_sleep_was_watchdog_reset(), hn_battery_read_mv());

    hn_acquire_selftest(g_reading);
    hn_report_selftest(g_reading);

#if HN_LORA_ENABLED
    g_pair_hash = hn_pair_hash(HN_PAIR_ID);
    hn_report_radio(hn_lora_begin());
#endif
}

void loop()
{
    /* Guard only the awake portion. A normal cycle is well under a second, so
     * an 8 s timeout only fires on a genuine hang - and a reboot on a roof
     * beats a Node sitting there flat. */
    hn_sleep_guard_start();

    const uint16_t battery_mv = hn_battery_read_mv();

    hn_acquire_cycle(g_reading);
    hn_sleep_guard_kick();
    hn_report_reading(g_reading);

#if HN_LORA_ENABLED
    /* Pack and send. The packet is built even when sensors failed - the Hub
     * needs to be told that a sensor is missing, and silence is the one thing
     * it cannot tell apart from a Node that is out of range. */
    hn_packet_t pkt;
    hn_telemetry_build(g_reading, g_pair_hash, HN_NODE_ID, battery_mv, pkt);

    uint8_t wire[HN_PACKET_BYTES];
    hn_packet_encode(&pkt, wire);

    bool sent = hn_lora_send(wire, HN_PACKET_BYTES);
    hn_report_tx(sent, HN_PACKET_BYTES, hn_lora_last_airtime_ms());
#endif

    hn_report_battery(battery_mv);

#if HN_SERIAL_ENABLED
    Serial.flush();          /* never power down mid-character */
#endif

    /* Stop guarding before sleeping: the watchdog is about to be re-purposed
     * as the wake-up timer, and an armed reset watchdog would restart the chip
     * instead of waking it. */
    hn_sleep_guard_stop();

#if HN_SLEEP_ENABLED
    hn_sleep_ms(HN_CYCLE_INTERVAL_MS);
#else
    hn_delay_ms(HN_CYCLE_INTERVAL_MS);
#endif
}
