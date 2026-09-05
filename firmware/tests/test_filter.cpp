/*
 * Host tests for the Hydro Node decision logic.
 *
 *     make test
 *
 * These run the exact source that ships to the device - hn_filter.cpp and
 * hn_crc8.cpp are compiled unmodified against a shim Arduino.h. What is being
 * checked is the behaviour that is hard to observe on a bench: that a single
 * ghost echo is rejected rather than averaged in, that a bouncing flow switch
 * is reported as bouncing rather than as a wet connector, and that "nothing
 * came back" is not reported as a broken sensor.
 */
#include <cstdio>
#include <cstring>

#include "hn_filter.h"
#include "hn_crc8.h"
#include "hn_config.h"
#include "hn_packet.h"
#include "hn_telemetry.h"

static int g_failures = 0;
static int g_checks   = 0;

#define CHECK(cond, ...)                                                       \
    do {                                                                       \
        ++g_checks;                                                            \
        if (!(cond)) {                                                         \
            ++g_failures;                                                      \
            std::printf("  FAIL %s:%d  ", __FILE__, __LINE__);                 \
            std::printf(__VA_ARGS__);                                          \
            std::printf("\n");                                                 \
        }                                                                      \
    } while (0)

static void case_name(const char *s) { std::printf("- %s\n", s); }

/* ------------------------------------------------------------------------- */
/* Ultrasonic filter                                                          */
/* ------------------------------------------------------------------------- */

static hn_ultrasonic_reading_t fresh_us(hn_presence_t p = HN_PRESENCE_CONFIRMED)
{
    hn_ultrasonic_reading_t r;
    std::memset(&r, 0, sizeof(r));
    r.status            = HN_STATUS_NOT_MEASURED;
    r.presence          = p;
    r.echo_stuck_high   = false;
    r.rise_without_fall = false;
    return r;
}

static void test_ultrasonic()
{
    std::printf("ultrasonic filter\n");

    {
        case_name("five tight samples average cleanly");
        const uint16_t s[5] = { 3400, 3410, 3405, 3395, 3402 };
        hn_ultrasonic_reading_t r = fresh_us();
        hn_filter_ultrasonic(s, 5, r);
        CHECK(r.status == HN_STATUS_OK, "status=%d", (int)r.status);
        CHECK(r.samples_accepted == 5, "accepted=%u", r.samples_accepted);
        CHECK(r.echo_us == 3402, "echo=%u", r.echo_us);
        CHECK(r.spread_us == 15, "spread=%u", r.spread_us);
    }

    {
        case_name("one ghost echo is rejected, not averaged in");
        /* 9000 us is a sidewall or fill-stream return ~1.5 m away. A plain mean
         * would report 4521 us; the median-anchored filter must ignore it. */
        const uint16_t s[5] = { 3400, 3410, 9000, 3395, 3402 };
        hn_ultrasonic_reading_t r = fresh_us();
        hn_filter_ultrasonic(s, 5, r);
        CHECK(r.status == HN_STATUS_OK, "status=%d", (int)r.status);
        CHECK(r.samples_accepted == 4, "accepted=%u", r.samples_accepted);
        CHECK(r.echo_us == 3401, "echo=%u (a mean would give 4521)", r.echo_us);
    }

    {
        case_name("a wide but outlier-free spread is UNSTABLE, and still reported");
        const uint16_t s[5] = { 3300, 3400, 3450, 3500, 3550 };
        hn_ultrasonic_reading_t r = fresh_us();
        hn_filter_ultrasonic(s, 5, r);
        CHECK(r.status == HN_STATUS_UNSTABLE, "status=%d", (int)r.status);
        CHECK(r.spread_us == 250, "spread=%u", r.spread_us);
        CHECK(r.echo_us == 3440, "echo=%u", r.echo_us);
    }

    {
        case_name("too few survivors is UNSTABLE");
        const uint16_t s[2] = { 3400, 3410 };
        hn_ultrasonic_reading_t r = fresh_us();
        hn_filter_ultrasonic(s, 2, r);
        CHECK(r.status == HN_STATUS_UNSTABLE, "status=%d", (int)r.status);
        CHECK(r.samples_accepted == 2, "accepted=%u", r.samples_accepted);
    }

    {
        case_name("no echo + floating line = NOT CONNECTED");
        hn_ultrasonic_reading_t r = fresh_us(HN_PRESENCE_ABSENT);
        hn_filter_ultrasonic(nullptr, 0, r);
        CHECK(r.status == HN_STATUS_ABSENT, "status=%d", (int)r.status);
    }

    {
        case_name("no echo + line already high = FAULT");
        hn_ultrasonic_reading_t r = fresh_us();
        r.echo_stuck_high = true;
        hn_filter_ultrasonic(nullptr, 0, r);
        CHECK(r.status == HN_STATUS_FAULT, "status=%d", (int)r.status);
    }

    {
        case_name("no echo from a healthy, driven line = NO TARGET, not a fault");
        /* This is the full-tank / blind-zone case. Reporting it as a sensor
         * fault would hide the most important state the product measures. */
        hn_ultrasonic_reading_t r = fresh_us();
        hn_filter_ultrasonic(nullptr, 0, r);
        CHECK(r.status == HN_STATUS_NO_TARGET, "status=%d", (int)r.status);
    }

    {
        /* The shipping configuration has the geometry window disabled: tank
         * geometry lives on the Hub, so the Node reports a short echo as a
         * perfectly good measurement rather than labelling it against a window
         * compiled into a device on a roof. */
        const uint16_t s[5] = { 200, 201, 202, 203, 204 };
        hn_ultrasonic_reading_t r = fresh_us();
        hn_filter_ultrasonic(s, 5, r);
        CHECK(r.echo_us == 202, "echo=%u", r.echo_us);
#if HN_US_PLAUSIBLE_MAX_US > 0
        case_name("with the bench geometry window on, a short echo is flagged");
        CHECK(r.status == HN_STATUS_OUT_OF_RANGE, "status=%d", (int)r.status);
#else
        case_name("a short echo is reported, not flagged - the Hub owns geometry");
        CHECK(r.status == HN_STATUS_OK, "status=%d", (int)r.status);
#endif
    }

    {
        /* The bug this replaced: an unplugged harness is an antenna, its
         * floating input produces edges, a few land in the valid window, and
         * the sensor came back reported as "noisy" instead of "missing". */
        case_name("an unplugged sensor picking up noise reads MISSING, not noisy");
        const uint16_t s[5] = { 900, 7400, 2100, 15000, 400 };
        hn_ultrasonic_reading_t r = fresh_us(HN_PRESENCE_ABSENT);
        hn_filter_ultrasonic(s, 5, r);
        CHECK(r.status == HN_STATUS_ABSENT, "status=%d", (int)r.status);
        CHECK(r.presence == HN_PRESENCE_ABSENT, "presence=%d", (int)r.presence);
        CHECK(r.echo_us == 0, "a fabricated echo of %u us must not be reported", r.echo_us);
    }

    {
        /* ...but a noisy reading from a sensor that IS there stays UNSTABLE,
         * so the fix above cannot swallow a real fault. */
        case_name("noisy samples from a connected sensor stay UNSTABLE");
        const uint16_t s[5] = { 900, 7400, 2100, 15000, 400 };
        hn_ultrasonic_reading_t r = fresh_us(HN_PRESENCE_CONFIRMED);
        hn_filter_ultrasonic(s, 5, r);
        CHECK(r.status == HN_STATUS_UNSTABLE, "status=%d", (int)r.status);
    }

    {
        case_name("a real echo overrides a pull-up probe that said ABSENT");
        const uint16_t s[5] = { 3400, 3410, 3405, 3395, 3402 };
        hn_ultrasonic_reading_t r = fresh_us(HN_PRESENCE_ABSENT);
        hn_filter_ultrasonic(s, 5, r);
        CHECK(r.presence == HN_PRESENCE_CONFIRMED, "presence=%d", (int)r.presence);
        CHECK(r.status == HN_STATUS_OK, "status=%d", (int)r.status);
    }
}

/* ------------------------------------------------------------------------- */
/* Flow switch classifier                                                     */
/* ------------------------------------------------------------------------- */

static void test_flow()
{
    std::printf("flow switch classifier\n");

    {
        case_name("clean open: idle, and presence cannot be proven");
        const uint16_t a[5]  = { 1000, 1001, 999, 1000, 1002 };
        const bool     d[5]  = { true, true, true, true, true };
        hn_flow_reading_t r;
        hn_classify_flow(a, d, 5, r);
        CHECK(r.status == HN_STATUS_OK, "status=%d", (int)r.status);
        CHECK(r.state == HN_FLOW_IDLE, "state=%d", (int)r.state);
        CHECK(r.presence == HN_PRESENCE_UNCONFIRMED, "presence=%d", (int)r.presence);
        CHECK(r.agree == 5, "agree=%u", r.agree);
    }

    {
        case_name("clean closed: filling, and presence is proven");
        const uint16_t a[5]  = { 3, 4, 2, 3, 3 };
        const bool     d[5]  = { false, false, false, false, false };
        hn_flow_reading_t r;
        hn_classify_flow(a, d, 5, r);
        CHECK(r.status == HN_STATUS_OK, "status=%d", (int)r.status);
        CHECK(r.state == HN_FLOW_FILLING, "state=%d", (int)r.state);
        CHECK(r.presence == HN_PRESENCE_CONFIRMED, "presence=%d", (int)r.presence);
    }

    {
        case_name("a bouncing contact is UNSTABLE, NOT a wet-connector fault");
        /* Every sample sits on a rail; only the majority is unclear. Averaging
         * these first would produce a mid-rail mean and misreport a normal
         * bouncing paddle switch as a resistive fault. */
        const uint16_t a[5] = { 1000, 1000, 1000, 3, 3 };
        const bool     d[5] = { true, true, true, false, false };
        hn_flow_reading_t r;
        hn_classify_flow(a, d, 5, r);
        CHECK(r.status == HN_STATUS_UNSTABLE, "status=%d", (int)r.status);
        CHECK(r.state == HN_FLOW_IDLE, "state=%d", (int)r.state);
        CHECK(r.presence == HN_PRESENCE_CONFIRMED, "presence=%d", (int)r.presence);
        CHECK(r.agree == 3, "agree=%u", r.agree);
        CHECK(r.level_adc == 1000, "level_adc=%u", r.level_adc);
    }

    {
        case_name("a mid-rail sample is a FAULT, and proves the harness is there");
        const uint16_t a[5] = { 1000, 1000, 500, 1000, 1000 };
        const bool     d[5] = { true, true, true, true, true };
        hn_flow_reading_t r;
        hn_classify_flow(a, d, 5, r);
        CHECK(r.status == HN_STATUS_FAULT, "status=%d", (int)r.status);
        CHECK(r.presence == HN_PRESENCE_CONFIRMED, "presence=%d", (int)r.presence);
        CHECK(r.state == HN_FLOW_UNKNOWN, "state=%d", (int)r.state);
    }

    {
        case_name("digital and analogue disagreeing is a FAULT");
        /* A2 sees the open rail while D5 reads low: one of the two 100R/330R
         * paths to the same net is damaged. */
        const uint16_t a[5] = { 1000, 1000, 1000, 1000, 1000 };
        const bool     d[5] = { false, false, false, false, false };
        hn_flow_reading_t r;
        hn_classify_flow(a, d, 5, r);
        CHECK(r.status == HN_STATUS_FAULT, "status=%d", (int)r.status);
    }

    {
        case_name("zero samples is handled without reporting a state");
        hn_flow_reading_t r;
        hn_classify_flow(nullptr, nullptr, 0, r);
        CHECK(r.status == HN_STATUS_NOT_MEASURED, "status=%d", (int)r.status);
        CHECK(r.state == HN_FLOW_UNKNOWN, "state=%d", (int)r.state);
    }
}

/* ------------------------------------------------------------------------- */
/* CRC-8                                                                      */
/* ------------------------------------------------------------------------- */

static void test_crc8()
{
    std::printf("Dallas CRC-8\n");

    {
        case_name("catalogue check value for CRC-8/MAXIM-DOW");
        /* The standard check value: CRC of the ASCII string "123456789" is
         * 0xA1 for this polynomial and initial value. This is the test that
         * proves the implementation is the algorithm the DS18B20 actually
         * uses, rather than merely self-consistent. */
        const uint8_t v[9] = { '1','2','3','4','5','6','7','8','9' };
        CHECK(hn_crc8(v, 9) == 0xA1, "crc=0x%02X expected 0xA1", hn_crc8(v, 9));
    }

    {
        case_name("Maxim application note 27 worked example");
        const uint8_t v[7] = { 0x02, 0x1C, 0xB8, 0x01, 0x00, 0x00, 0x00 };
        CHECK(hn_crc8(v, 7) == 0xA2, "crc=0x%02X expected 0xA2", hn_crc8(v, 7));
    }

    {
        case_name("a ROM code validates against its own CRC byte");
        const uint8_t rom[8] = { 0x28, 0xAA, 0x1E, 0x2C, 0x1D, 0x13, 0x02, 0x6E };
        CHECK(hn_crc8(rom, 7) == rom[7], "crc=0x%02X expected 0x%02X",
              hn_crc8(rom, 7), rom[7]);
    }

    {
        case_name("appending the CRC makes the whole message check to zero");
        /* The defining property of a CRC, and the one the scratchpad read
         * relies on. */
        uint8_t buf[10] = { 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x11 };
        buf[9] = hn_crc8(buf, 9);
        CHECK(hn_crc8(buf, 10) == 0, "crc=0x%02X", hn_crc8(buf, 10));
    }

    {
        case_name("a single flipped bit is detected");
        uint8_t buf[10] = { 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x11 };
        buf[9] = hn_crc8(buf, 9);
        buf[4] ^= 0x08;
        CHECK(hn_crc8(buf, 10) != 0, "flipped bit went undetected");
    }

    {
        case_name("an empty message is zero");
        CHECK(hn_crc8(nullptr, 0) == 0, "non-zero");
    }
}


/* ------------------------------------------------------------------------- */
/* Wire format                                                                */
/* ------------------------------------------------------------------------- */

static hn_packet_t sample_packet()
{
    hn_packet_t p;
    p.version = HN_PROTO_VERSION;
    p.pair_hash = 0xC207;
    p.node_id = 0x0001;
    p.seq = 4242;
    p.echo_us = 3402;
    p.temp_raw = 328;
    p.flow_adc8 = 252;
    p.st_us = HN_ST_PACK(HN_W_OK, HN_W_PRES_CONFIRMED);
    p.st_tp = HN_ST_PACK(HN_W_OK, HN_W_PRES_CONFIRMED);
    p.st_fl = HN_ST_FLOW_PACK(HN_ST_PACK(HN_W_OK, HN_W_PRES_UNCONFIRMED), HN_W_FLOW_IDLE);
    p.flags = HN_FLAG_FLOW_LEVEL | HN_FLAG_TEMP_CRC_OK;
    p.battery_dv = HN_BATT_NONE;
    return p;
}

static void test_packet()
{
    std::printf("wire format\n");

    {
        case_name("CRC-16/CCITT-FALSE catalogue check value");
        const uint8_t v[9] = { '1','2','3','4','5','6','7','8','9' };
        CHECK(hn_crc16(v, 9) == 0x29B1, "crc=0x%04X expected 0x29B1", hn_crc16(v, 9));
    }

    {
        case_name("a packet survives encode -> decode unchanged");
        hn_packet_t in = sample_packet();
        uint8_t wire[HN_PACKET_BYTES];
        hn_packet_encode(&in, wire);

        hn_packet_t out;
        CHECK(hn_packet_decode(wire, HN_PACKET_BYTES, &out) == HN_DEC_OK, "decode failed");
        CHECK(out.pair_hash == in.pair_hash, "pair");
        CHECK(out.node_id == in.node_id, "node");
        CHECK(out.seq == in.seq, "seq");
        CHECK(out.echo_us == in.echo_us, "echo=%u", out.echo_us);
        CHECK(out.temp_raw == in.temp_raw, "temp=%d", out.temp_raw);
        CHECK(out.flow_adc8 == in.flow_adc8, "adc");
        CHECK(out.st_us == in.st_us && out.st_tp == in.st_tp && out.st_fl == in.st_fl, "status");
        CHECK(out.flags == in.flags, "flags");
    }

    {
        case_name("the packet is exactly 19 bytes - airtime is the whole point");
        hn_packet_t in = sample_packet();
        uint8_t wire[HN_PACKET_BYTES + 4] = {0};
        hn_packet_encode(&in, wire);
        CHECK(HN_PACKET_BYTES == 19, "size=%d", (int)HN_PACKET_BYTES);
        CHECK(wire[HN_PACKET_BYTES] == 0 && wire[HN_PACKET_BYTES + 1] == 0,
              "encode wrote past the end of the packet");
    }

    {
        case_name("a single corrupted byte is rejected, not misread");
        hn_packet_t in = sample_packet();
        uint8_t wire[HN_PACKET_BYTES];
        hn_packet_encode(&in, wire);
        wire[HN_OFF_ECHO] ^= 0x01;          /* one bit of the distance */
        hn_packet_t out;
        CHECK(hn_packet_decode(wire, HN_PACKET_BYTES, &out) == HN_DEC_BAD_CRC,
              "corrupted packet accepted");
    }

    {
        case_name("wrong length and wrong version are rejected");
        hn_packet_t in = sample_packet();
        uint8_t wire[HN_PACKET_BYTES];
        hn_packet_encode(&in, wire);
        hn_packet_t out;
        CHECK(hn_packet_decode(wire, HN_PACKET_BYTES - 1, &out) == HN_DEC_BAD_LENGTH, "length");
        wire[HN_OFF_VERSION] = HN_PROTO_VERSION + 1;
        CHECK(hn_packet_decode(wire, HN_PACKET_BYTES, &out) == HN_DEC_BAD_VERSION, "version");
    }

    {
        case_name("status and presence pack into one byte without collision");
        for (int st = 0; st <= 6; ++st) {
            for (int pr = 0; pr <= 3; ++pr) {
                uint8_t b = HN_ST_PACK(st, pr);
                CHECK(HN_ST_STATUS(b) == st && HN_ST_PRESENCE(b) == pr,
                      "st=%d pr=%d packed to 0x%02X", st, pr, b);
            }
        }
        for (int fl = 0; fl <= 2; ++fl) {
            uint8_t b = HN_ST_FLOW_PACK(HN_ST_PACK(HN_W_FAULT, HN_W_PRES_ABSENT), fl);
            CHECK(HN_ST_STATUS(b) == HN_W_FAULT && HN_ST_PRESENCE(b) == HN_W_PRES_ABSENT &&
                  HN_ST_FLOW(b) == fl, "flow=%d collided", fl);
        }
    }

    {
        case_name("the pair hash separates two nearby installations");
        CHECK(hn_pair_hash("SWS-PAIR-0001") == 0xC207, "hash=0x%04X", hn_pair_hash("SWS-PAIR-0001"));
        CHECK(hn_pair_hash("SWS-PAIR-0001") != hn_pair_hash("SWS-PAIR-0002"), "collision");
    }
}

/* ------------------------------------------------------------------------- */
/* Reading -> packet                                                          */
/* ------------------------------------------------------------------------- */

static hn_reading_t good_reading()
{
    hn_reading_t r;
    hn_reading_clear(r);
    r.seq = 7;
    r.ultrasonic.status = HN_STATUS_OK;
    r.ultrasonic.presence = HN_PRESENCE_CONFIRMED;
    r.ultrasonic.echo_us = 3402;
    r.temperature.status = HN_STATUS_OK;
    r.temperature.presence = HN_PRESENCE_CONFIRMED;
    r.temperature.raw = 328;
    r.temperature.crc_ok = true;
    r.flow.status = HN_STATUS_OK;
    r.flow.presence = HN_PRESENCE_UNCONFIRMED;
    r.flow.state = HN_FLOW_IDLE;
    r.flow.level_adc = 1009;
    r.flow.level_digital = true;
    return r;
}

static void test_telemetry()
{
    std::printf("reading -> packet\n");

    {
        case_name("a healthy cycle maps across intact");
        hn_reading_t r = good_reading();
        hn_packet_t p;
        hn_telemetry_build(r, 0xC207, 1, 3600, p);
        CHECK(p.echo_us == 3402, "echo=%u", p.echo_us);
        CHECK(p.temp_raw == 328, "temp=%d", p.temp_raw);
        CHECK(p.flow_adc8 == (1009 >> 2), "adc=%u", p.flow_adc8);
        CHECK(HN_ST_FLOW(p.st_fl) == HN_W_FLOW_IDLE, "flow state");
        CHECK((p.flags & HN_FLAG_TEMP_CRC_OK) != 0, "crc flag");
        CHECK(HN_BATT_MV(p.battery_dv) == 3600, "battery=%u mV", HN_BATT_MV(p.battery_dv));
    }

    {
        case_name("battery encodes to 10 mV steps and back");
        hn_reading_t r = good_reading();
        hn_packet_t p;
        for (uint16_t mv = 2000; mv <= 4600; mv += 130) {
            hn_telemetry_build(r, 0xC207, 1, mv, p);
            const uint16_t back = HN_BATT_MV(p.battery_dv);
            const uint16_t want = (mv < 2010) ? 2010 : (mv > 4550 ? 4550 : mv);
            CHECK(back >= want - 10 && back <= want + 10,
                  "%u mV -> %u mV", mv, back);
        }
        hn_telemetry_build(r, 0xC207, 1, 0, p);
        CHECK(p.battery_dv == HN_BATT_NONE, "unmeasured battery must stay none");
    }

    {
        /* The failure that matters most: a dead ultrasonic must not arrive as
         * a number the Hub can turn into a water level. */
        case_name("a missing ultrasonic sends NO echo value, not a stale or zero one");
        hn_reading_t r = good_reading();
        r.ultrasonic.status = HN_STATUS_ABSENT;
        r.ultrasonic.presence = HN_PRESENCE_ABSENT;
        hn_packet_t p;
        hn_telemetry_build(r, 0xC207, 1, 3600, p);
        CHECK(p.echo_us == HN_ECHO_NONE, "echo=%u should be none", p.echo_us);
        CHECK(HN_ST_STATUS(p.st_us) == HN_W_ABSENT, "status");
        CHECK(HN_ST_PRESENCE(p.st_us) == HN_W_PRES_ABSENT, "presence");
    }

    {
        case_name("NO ECHO on a healthy sensor is distinguishable from a fault");
        hn_reading_t r = good_reading();
        r.ultrasonic.status = HN_STATUS_NO_TARGET;
        hn_packet_t p;
        hn_telemetry_build(r, 0xC207, 1, 3600, p);
        CHECK(p.echo_us == HN_ECHO_NONE, "echo");
        CHECK(HN_ST_STATUS(p.st_us) == HN_W_NO_TARGET, "status=%u", HN_ST_STATUS(p.st_us));
        CHECK(HN_ST_PRESENCE(p.st_us) == HN_W_PRES_CONFIRMED, "still connected");
    }

    {
        case_name("an UNSTABLE reading is still sent, and labelled");
        hn_reading_t r = good_reading();
        r.ultrasonic.status = HN_STATUS_UNSTABLE;
        hn_packet_t p;
        hn_telemetry_build(r, 0xC207, 1, 3600, p);
        CHECK(p.echo_us == 3402, "a noisy but real measurement must still reach the Hub");
        CHECK(HN_ST_STATUS(p.st_us) == HN_W_UNSTABLE, "status");
    }

    {
        case_name("a temperature that failed CRC is not sent as a temperature");
        hn_reading_t r = good_reading();
        r.temperature.crc_ok = false;
        r.temperature.status = HN_STATUS_FAULT;
        hn_packet_t p;
        hn_telemetry_build(r, 0xC207, 1, 3600, p);
        CHECK(p.temp_raw == HN_TEMP_RAW_NONE, "temp=%d should be none", p.temp_raw);
        CHECK((p.flags & HN_FLAG_TEMP_CRC_OK) == 0, "crc flag should be clear");
    }

    {
        case_name("filling sets the gate flag so the Hub can distrust the level");
        hn_reading_t r = good_reading();
        r.flow.state = HN_FLOW_FILLING;
        r.flow.presence = HN_PRESENCE_CONFIRMED;
        r.level_gated_by_flow = true;
        hn_packet_t p;
        hn_telemetry_build(r, 0xC207, 1, 3600, p);
        CHECK((p.flags & HN_FLAG_GATED_BY_FLOW) != 0, "gate flag");
        CHECK(HN_ST_FLOW(p.st_fl) == HN_W_FLOW_FILLING, "flow state");
    }

    {
        case_name("the 32-bit cycle counter truncates cleanly onto 16 bits");
        hn_reading_t r = good_reading();
        r.seq = 65536UL + 5UL;
        hn_packet_t p;
        hn_telemetry_build(r, 0xC207, 1, 3600, p);
        CHECK(p.seq == 5, "seq=%u", p.seq);
    }
}

int main()
{
    std::printf("\nHydro Node - Section 1 logic tests\n");
    std::printf("=================================\n");
    test_ultrasonic();
    test_flow();
    test_crc8();
    test_packet();
    test_telemetry();
    std::printf("=================================\n");
    std::printf("%d checks, %d failures\n\n", g_checks, g_failures);
    return g_failures ? 1 : 0;
}
