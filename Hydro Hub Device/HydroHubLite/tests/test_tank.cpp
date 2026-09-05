/*
 * Host tests for the Hub's tank maths and the shared wire format.
 *
 *     make test
 *
 * The ESP32 half of this firmware (TFT, RadioLib) cannot be compiled on a
 * workstation, but the part that can actually be *wrong in a quiet way* can:
 * a geometry bug does not crash, it just displays a confident number that is
 * not true. So the maths is kept free of Arduino dependencies and pinned here.
 */
#include <cstdio>
#include <cmath>
#include <cstring>

#include "../TankMath.h"
#include "../hn_packet.h"

static int g_fail = 0, g_checks = 0;

#define CHECK(cond, ...) do { ++g_checks; if(!(cond)) { ++g_fail; \
    std::printf("  FAIL %s:%d  ", __FILE__, __LINE__); std::printf(__VA_ARGS__); \
    std::printf("\n"); } } while(0)

#define NEAR(a, b, tol) (std::fabs((double)(a) - (double)(b)) <= (tol))

static void name(const char *s) { std::printf("- %s\n", s); }

/* The configuration from config.h: two connected tanks, 1000 L + 500 L,
 * 80 cm of water at 100%, 10 cm of headspace above that. */
static tank_config_t cfg()
{
    tank_config_t c;
    c.tank_count = 2;
    c.total_liters = 1500;
    c.water_height_cm = 80;
    c.blind_cm = 10;
    c.transducer_sep_mm = 0;
    return c;
}

/* Round-trip microseconds that correspond to a given one-way distance. */
static uint16_t echo_for_cm(float cm, float temp_c)
{
    return (uint16_t)((cm * 20000.0f / tank_speed_of_sound(temp_c)) + 0.5f);
}

int main()
{
    std::printf("\nHydro Hub Lite - tank maths\n===========================\n");
    const float T = 25.0f;
    tank_config_t c = cfg();
    tank_result_t r;

    {
        name("a full tank reads the blind distance and gives 100%");
        CHECK(tank_compute(&c, echo_for_cm(10.0f, T), T, &r), "compute failed");
        CHECK(NEAR(r.level_pct, 100.0f, 0.6f), "level=%.2f%%", r.level_pct);
        CHECK(NEAR(r.volume_liters, 1500, 10), "volume=%luL", (unsigned long)r.volume_liters);
        CHECK(NEAR(r.water_height_cm, 80.0f, 0.5f), "height=%.2f", r.water_height_cm);
    }

    {
        name("an empty tank reads blind + height and gives 0%");
        CHECK(tank_compute(&c, echo_for_cm(90.0f, T), T, &r), "compute failed");
        CHECK(NEAR(r.level_pct, 0.0f, 0.6f), "level=%.2f%%", r.level_pct);
        CHECK(r.volume_liters <= 10, "volume=%luL", (unsigned long)r.volume_liters);
    }

    {
        name("half full is 50% and half the summed capacity of both tanks");
        CHECK(tank_compute(&c, echo_for_cm(50.0f, T), T, &r), "compute failed");
        CHECK(NEAR(r.level_pct, 50.0f, 0.6f), "level=%.2f%%", r.level_pct);
        CHECK(NEAR(r.volume_liters, 750, 12), "volume=%luL", (unsigned long)r.volume_liters);
    }

    {
        name("the two tanks are summed, not averaged");
        /* 1000 + 500 must behave as 1500, or a second tank silently does
         * nothing and the user's total is wrong by a third. */
        tank_config_t one = cfg();
        one.tank_count = 1;
        one.total_liters = 1000;
        tank_result_t r1;
        CHECK(tank_compute(&one, echo_for_cm(50.0f, T), T, &r1), "compute failed");
        CHECK(NEAR(r1.volume_liters, 500, 10), "single-tank volume=%luL",
              (unsigned long)r1.volume_liters);
        CHECK(r.volume_liters > r1.volume_liters, "two tanks must hold more than one");
    }

    {
        name("echo microseconds convert to the distance that produced them");
        CHECK(NEAR(tank_distance_cm(echo_for_cm(50.0f, T), T, 0), 50.0f, 0.2f),
              "got %.2f cm", tank_distance_cm(echo_for_cm(50.0f, T), T, 0));
    }

    {
        name("temperature moves the answer, and by the expected amount");
        /* Speed of sound rises ~0.18%/C. Reading the same echo as if it were
         * 0 C when it is really 40 C is a ~7% distance error - which is why
         * the Node ships a temperature at all. */
        const uint16_t e = echo_for_cm(50.0f, 0.0f);
        const float cold = tank_distance_cm(e, 0.0f, 0);
        const float hot  = tank_distance_cm(e, 40.0f, 0);
        CHECK(NEAR(cold, 50.0f, 0.2f), "cold=%.2f", cold);
        CHECK(hot > cold, "warmer air must read further");
        CHECK(NEAR((hot - cold) / cold * 100.0f, 7.3f, 0.5f),
              "delta=%.2f%% expected ~7.3%%", (hot - cold) / cold * 100.0f);
    }

    {
        name("no echo produces no level, rather than a level of zero");
        /* The difference matters: 0% means "the tank is empty", no reading
         * means "we do not know". Showing the first when we mean the second is
         * how somebody runs a pump dry. */
        CHECK(!tank_compute(&c, HN_ECHO_NONE, T, &r), "accepted a missing echo");
        CHECK(!r.valid, "result should not be valid");
        CHECK(r.volume_liters == 0 && r.level_pct == 0.0f, "should be zeroed, not stale");
    }

    {
        name("readings beyond the ends of the tank clamp AND say they clamped");
        CHECK(tank_compute(&c, echo_for_cm(4.0f, T), T, &r), "compute failed");
        CHECK(NEAR(r.level_pct, 100.0f, 0.01f), "level=%.2f", r.level_pct);
        CHECK(r.clamped_full, "over-full reading not flagged");

        CHECK(tank_compute(&c, echo_for_cm(120.0f, T), T, &r), "compute failed");
        CHECK(NEAR(r.level_pct, 0.0f, 0.01f), "level=%.2f", r.level_pct);
        CHECK(r.clamped_empty, "under-empty reading not flagged");
    }

    {
        name("parallax correction shrinks the distance, most at close range");
        const uint16_t near_echo = echo_for_cm(5.0f, T);
        const uint16_t far_echo  = echo_for_cm(100.0f, T);
        const float near_err = tank_distance_cm(near_echo, T, 0) - tank_distance_cm(near_echo, T, 40);
        const float far_err  = tank_distance_cm(far_echo,  T, 0) - tank_distance_cm(far_echo,  T, 40);
        CHECK(near_err > 0.0f, "correction must reduce the distance");
        CHECK(NEAR(near_err, 0.40f, 0.10f), "near correction=%.3f cm, expected ~0.4mm*10", near_err);
        CHECK(far_err < 0.03f, "far correction=%.4f cm should be negligible", far_err);
        CHECK(near_err > far_err * 10.0f, "the correction must be range-dependent");
    }

    {
        name("a zero-height configuration is refused, not divided by");
        tank_config_t bad = cfg();
        bad.water_height_cm = 0;
        CHECK(!tank_compute(&bad, echo_for_cm(50.0f, T), T, &r), "accepted height=0");
    }

    {
        name("the DS18B20 register decodes to Celsius");
        CHECK(NEAR(tank_temp_c(328), 20.5f, 0.01f), "got %.3f", tank_temp_c(328));
        CHECK(NEAR(tank_temp_c(-80), -5.0f, 0.01f), "got %.3f", tank_temp_c(-80));
    }

    {
        name("the Hub decodes a packet the Node encoded");
        hn_packet_t in;
        std::memset(&in, 0, sizeof(in));
        in.pair_hash = hn_pair_hash("SWS-PAIR-0001");
        in.node_id = 1; in.seq = 9; in.echo_us = 2912; in.temp_raw = 328;
        in.st_us = HN_ST_PACK(HN_W_OK, HN_W_PRES_CONFIRMED);
        uint8_t wire[HN_PACKET_BYTES];
        hn_packet_encode(&in, wire);

        hn_packet_t out;
        CHECK(hn_packet_decode(wire, HN_PACKET_BYTES, &out) == HN_DEC_OK, "decode");
        CHECK(out.pair_hash == hn_pair_hash("SWS-PAIR-0001"), "pair mismatch");
        CHECK(out.echo_us == 2912, "echo");

        tank_result_t tr;
        CHECK(tank_compute(&c, out.echo_us, tank_temp_c(out.temp_raw), &tr), "compute");
        CHECK(NEAR(tr.distance_cm, 50.0f, 0.5f), "end-to-end distance=%.2f", tr.distance_cm);
    }

    std::printf("===========================\n%d checks, %d failures\n\n", g_checks, g_fail);
    return g_fail ? 1 : 0;
}
