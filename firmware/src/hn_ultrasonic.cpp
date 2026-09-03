#include "hn_ultrasonic.h"
#include "hn_board.h"
#include "hn_config.h"
#include "hn_filter.h"

#include <Arduino.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <util/delay.h>

#if HN_PIN_US_ECHO != 8
#error "The echo pin must be D8 (PB0/ICP1): this driver uses Timer1 input capture."
#endif

/* Capture state machine, shared with the ISR. */
#define CAP_WAIT_RISE  0
#define CAP_WAIT_FALL  1
#define CAP_DONE       2

static volatile uint8_t  s_cap_state;
static volatile uint16_t s_rise_ticks;
static volatile uint16_t s_width_ticks;

ISR(TIMER1_CAPT_vect)
{
    uint16_t t = ICR1;

    if (s_cap_state == CAP_WAIT_RISE) {
        s_rise_ticks = t;
        TCCR1B = (uint8_t)(TCCR1B & (uint8_t)~_BV(ICES1));   /* now catch the falling edge */
        TIFR1  = _BV(ICF1);   /* changing the edge can set the flag spuriously */
        s_cap_state = CAP_WAIT_FALL;
    } else if (s_cap_state == CAP_WAIT_FALL) {
        /* Unsigned 16-bit subtraction is correct across a timer wrap, and the
         * longest echo we accept (25 ms) is well inside the 65.5 ms period. */
        s_width_ticks = (uint16_t)(t - s_rise_ticks);
        s_cap_state = CAP_DONE;
        TIMSK1 = 0;
    }
}

/* One Timer1 tick at prescaler 8 is 8/F_CPU seconds: exactly 1 us at the
 * Pro Mini's 8 MHz. Written out so a 16 MHz board still gets the right answer. */
static inline uint16_t ticks_to_us(uint16_t ticks)
{
    return (uint16_t)(((uint32_t)ticks * 8UL) / (F_CPU / 1000000UL));
}

void hn_ultrasonic_begin()
{
    pinMode(HN_PIN_US_TRIG, OUTPUT);
    digitalWrite(HN_PIN_US_TRIG, LOW);
    pinMode(HN_PIN_US_ECHO, INPUT);
    power_timer1_disable();
}

hn_presence_t hn_ultrasonic_probe_presence()
{
    /*
     * With the module connected, its echo output actively drives the line low
     * while idle, and it wins easily against the ATmega's ~40 kohm internal
     * pull-up. With the harness unplugged there is nothing on the line but the
     * 100 ohm series resistor, so the pull-up takes it high.
     *
     * So: pull-up on, look, pull-up off. HIGH means nothing is driving the
     * line. The pull-up sources ~70 uA while it is on and the sensor is
     * present, which is why it is on for 200 us and not one microsecond longer.
     *
     * This is corroborating evidence, not proof - hn_ultrasonic_read() lets a
     * real echo override it, because a working sensor is the better witness.
     */
    pinMode(HN_PIN_US_ECHO, INPUT_PULLUP);
    _delay_us(HN_US_PRESENCE_PROBE_US);
    uint8_t floating = digitalRead(HN_PIN_US_ECHO);
    pinMode(HN_PIN_US_ECHO, INPUT);          /* pull-up off again */

    return floating ? HN_PRESENCE_ABSENT : HN_PRESENCE_CONFIRMED;
}

/* Result of a single trigger. */
#define SHOT_OK          0
#define SHOT_NO_RISE     1   /* module never started a burst                  */
#define SHOT_NO_FALL     2   /* burst started and never ended - wiring fault  */

static uint8_t take_one_shot(uint16_t &echo_us)
{
    echo_us = 0;

    s_cap_state = CAP_WAIT_RISE;

    power_timer1_enable();
    TCCR1A = 0;
    TCNT1  = 0;
    /* ICNC1: the input capture noise canceller requires four agreeing samples
     * before it accepts an edge. It costs 4 clock cycles of latency on BOTH
     * edges, so it cancels out of the width entirely, and it makes a long
     * sensor cable far less likely to produce a false trigger. */
    TCCR1B = _BV(ICNC1) | _BV(ICES1) | _BV(CS11);   /* rising edge, clk/8 */
    TIFR1  = _BV(ICF1);
    TIMSK1 = _BV(ICIE1);

    /* HC-SR04-compatible modules want a >=10 us trigger pulse. */
    digitalWrite(HN_PIN_US_TRIG, HIGH);
    _delay_us(10);
    digitalWrite(HN_PIN_US_TRIG, LOW);

    uint32_t start = millis();
    while (s_cap_state != CAP_DONE) {
        if ((uint16_t)(millis() - start) >= HN_US_ECHO_TIMEOUT_MS) break;
        /*
         * There is a benign race here: the capture interrupt can land between
         * the test above and the sleep. It costs nothing, because Timer0 wakes
         * the CPU every ~2 ms anyway, and it cannot affect accuracy - the echo
         * width was timestamped in hardware by the capture unit, not by when
         * this loop happens to notice.
         */
        hn_idle_once();
    }

    /* Silence the capture interrupt BEFORE sampling the shared state, so the
     * state and the width that goes with it cannot be updated between the two
     * reads. */
    TIMSK1 = 0;
    uint8_t  state = s_cap_state;
    uint16_t ticks = s_width_ticks;

    TCCR1B = 0;
    power_timer1_disable();

    if (state == CAP_DONE) {
        echo_us = ticks_to_us(ticks);
        return SHOT_OK;
    }
    return (state == CAP_WAIT_FALL) ? SHOT_NO_FALL : SHOT_NO_RISE;
}

void hn_ultrasonic_read(hn_ultrasonic_reading_t &r)
{
    r.status            = HN_STATUS_NOT_MEASURED;
    r.presence          = HN_PRESENCE_UNKNOWN;
    r.echo_us           = 0;
    r.spread_us         = 0;
    r.samples_taken     = 0;
    r.samples_valid     = 0;
    r.samples_accepted  = 0;
    r.echo_stuck_high   = false;
    r.rise_without_fall = false;

    r.presence = hn_ultrasonic_probe_presence();

    /* A line that is high with NO pull-up means something is actively driving
     * it before we have asked for anything: a stuck module, or TRIG and ECHO
     * swapped in the harness so our own trigger output is on this pin.
     *
     * Only meaningful when something is connected - an unplugged line floats
     * and can read either way, which is not a fault, so the probe result gates
     * this check. */
    r.echo_stuck_high = (r.presence != HN_PRESENCE_ABSENT) &&
                        (digitalRead(HN_PIN_US_ECHO) == HIGH);

    uint16_t samples[HN_US_SAMPLES];
    uint8_t  n_valid = 0;

    for (uint8_t i = 0; i < HN_US_SAMPLES; ++i) {
        uint16_t us;
        uint8_t shot = take_one_shot(us);
        r.samples_taken++;

        if (shot == SHOT_NO_FALL) r.rise_without_fall = true;

        if (shot == SHOT_OK && us >= HN_US_ECHO_MIN_US && us <= HN_US_ECHO_MAX_US) {
            samples[n_valid++] = us;
        }

        /* Let the module's 50 ms measurement cycle finish before the next
         * trigger, so the tail of this burst is not read as the next echo. */
        if (i + 1 < HN_US_SAMPLES) hn_delay_ms(HN_US_SAMPLE_GAP_MS);
    }

    /* Filtering and classification are pure logic and live in hn_filter.cpp so
     * they can be exercised against known inputs on a host machine. */
    hn_filter_ultrasonic(samples, n_valid, r);
}
