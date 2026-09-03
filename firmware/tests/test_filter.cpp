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
        case_name("a stable reading below the installation window is flagged, not dropped");
        const uint16_t s[5] = { 200, 201, 202, 203, 204 };
        hn_ultrasonic_reading_t r = fresh_us();
        hn_filter_ultrasonic(s, 5, r);
        CHECK(r.status == HN_STATUS_OUT_OF_RANGE, "status=%d", (int)r.status);
        CHECK(r.echo_us == 202, "echo=%u", r.echo_us);
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

int main()
{
    std::printf("\nHydro Node - Section 1 logic tests\n");
    std::printf("=================================\n");
    test_ultrasonic();
    test_flow();
    test_crc8();
    std::printf("=================================\n");
    std::printf("%d checks, %d failures\n\n", g_checks, g_failures);
    return g_failures ? 1 : 0;
}
