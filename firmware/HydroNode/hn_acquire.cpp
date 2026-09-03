#include "hn_acquire.h"
#include "hn_ultrasonic.h"
#include "hn_temperature.h"
#include "hn_flow.h"
#include "hn_board.h"
#include "hn_config.h"

#include <Arduino.h>

static uint32_t s_seq = 0;

void hn_acquire_begin()
{
    hn_ultrasonic_begin();
    hn_temperature_begin();
    hn_flow_begin();
}

void hn_acquire_selftest(hn_reading_t &r)
{
    hn_reading_clear(r);
    r.uptime_ms = millis();

    /* Ultrasonic: the pull-up probe only, no trigger. */
    r.ultrasonic.presence = hn_ultrasonic_probe_presence();

    /* Temperature: a full read, because on 1-Wire the presence pulse and the
     * ROM code are the presence test, and doing the whole transaction also
     * proves the CRC path works. */
    hn_temperature_read(r.temperature);

    hn_flow_read(r.flow);
}

void hn_acquire_cycle(hn_reading_t &r)
{
    hn_reading_clear(r);
    r.seq       = ++s_seq;
    r.uptime_ms = millis();

    /*
     * Flow first. It is the cheapest of the three, and knowing whether water is
     * running in BEFORE the level samples are taken is what makes the gate flag
     * meaningful: a stream falling from the fill pipe is an ultrasonic target
     * at every depth it passes through, so a level reading taken during filling
     * is not trustworthy. The Node does not act on that - it records it and
     * lets the Hub decide.
     */
    hn_flow_read(r.flow);
    r.level_gated_by_flow = (r.flow.state == HN_FLOW_FILLING);

    /*
     * Now overlap the two slow operations. The DS18B20 conversion takes ~94 ms
     * and the ultrasonic burst takes ~300 ms, and they use completely separate
     * hardware. Starting the conversion first and collecting it afterwards
     * makes the temperature reading free in awake-time terms - it costs 94 ms
     * if you do it sequentially, and nothing if you do it like this.
     *
     * The temperature that comes back is therefore measured concurrently with
     * the echo samples rather than just before them, which is if anything the
     * more correct pairing.
     */
    bool converting = hn_temperature_start(r.temperature);

    hn_ultrasonic_read(r.ultrasonic);

    if (converting) {
        /* The burst already took ~300 ms, comfortably past the 94 ms
         * conversion, so this returns as soon as it polls the bus. */
        hn_temperature_finish(r.temperature);
    }
}
