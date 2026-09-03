/*
 * hn_reading.h - the one data structure the whole firmware is built around.
 *
 * ARCHITECTURAL DECISION, and the reason this struct looks the way it does:
 * the Node transmits RAW values and performs no interpretation. Echo time in
 * microseconds, the DS18B20's raw register, the flow switch's raw levels. The
 * speed-of-sound formula, the humidity correction, the split-transducer
 * parallax correction and the tank geometry all live on the Hub, where they can
 * be revised over Wi-Fi without touching a sealed rooftop device.
 *
 * The millimetre figure the serial report prints is a bench convenience,
 * computed in hn_report.cpp and stored nowhere. Do not be tempted to move it in
 * here - the moment a converted value enters this struct it will end up in the
 * LoRa payload, and the correction constants will be frozen into a device on a
 * roof.
 *
 * Section 2's LoRaManager packs its payload from this struct. Section 3's
 * SleepManager fills in nothing here. Adding a field is cheap; changing the
 * meaning of one is not.
 */
#ifndef HN_READING_H
#define HN_READING_H

#include <stdint.h>
#include "hn_status.h"

enum hn_flow_state_t : uint8_t {
    HN_FLOW_UNKNOWN = 0,
    HN_FLOW_IDLE,        /* contact open   - not filling */
    HN_FLOW_FILLING,     /* contact closed - filling     */
};

struct hn_ultrasonic_reading_t {
    hn_status_t   status;
    hn_presence_t presence;

    uint16_t echo_us;          /* RAW: filtered round-trip time, microseconds  */
    uint16_t spread_us;        /* max-min across accepted samples (noise)      */

    uint8_t  samples_taken;    /* triggers issued                              */
    uint8_t  samples_valid;    /* echoes inside the module's physical limits   */
    uint8_t  samples_accepted; /* survivors of median outlier rejection        */

    bool     echo_stuck_high;  /* echo line was high before we triggered       */
    bool     rise_without_fall;/* burst started but never ended - wiring fault */
};

struct hn_temperature_reading_t {
    hn_status_t   status;
    hn_presence_t presence;

    int16_t  raw;              /* RAW: DS18B20 temperature register            */
    uint8_t  resolution_bits;
    uint8_t  rom[8];           /* device ROM code, cached at startup           */
    bool     rom_valid;
    bool     crc_ok;           /* scratchpad CRC of THIS reading               */
};

struct hn_flow_reading_t {
    hn_status_t     status;
    hn_presence_t   presence;
    hn_flow_state_t state;

    bool     level_digital;    /* RAW: D5 as read (majority vote)              */
    uint16_t level_adc;        /* RAW: A2 counts of 1023, ratiometric to VCC   */
    uint8_t  samples;
    uint8_t  agree;            /* how many samples matched the majority        */
};

struct hn_reading_t {
    uint32_t seq;              /* cycle counter since power-up                 */
    uint32_t uptime_ms;

    hn_ultrasonic_reading_t  ultrasonic;
    hn_temperature_reading_t temperature;
    hn_flow_reading_t        flow;

    /* Set when the flow switch says water is running in. A falling stream is a
     * target at every depth it passes through, so a level reading taken during
     * filling is not trustworthy [REV HW-030]. The Node does not act on this -
     * it just tells the Hub, which decides whether to use the sample. */
    bool level_gated_by_flow;
};

/* Zero a reading and put every field into its "nothing measured yet" state. */
void hn_reading_clear(hn_reading_t &r);

#endif /* HN_READING_H */
