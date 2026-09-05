#include "hn_filter.h"
#include "hn_config.h"

/* ------------------------------------------------------------------------- */
/* Ultrasonic                                                                 */
/* ------------------------------------------------------------------------- */

static void sort_u16(uint16_t *a, uint8_t n)
{
    /* Insertion sort. n is HN_US_SAMPLES, which is 5. */
    for (uint8_t i = 1; i < n; ++i) {
        uint16_t v = a[i];
        int8_t j = (int8_t)(i - 1);
        while (j >= 0 && a[j] > v) { a[j + 1] = a[j]; --j; }
        a[j + 1] = v;
    }
}

void hn_filter_ultrasonic(const uint16_t *valid, uint8_t n_valid,
                          hn_ultrasonic_reading_t &r)
{
    r.samples_valid    = n_valid;
    r.samples_accepted = 0;
    r.echo_us          = 0;
    r.spread_us        = 0;

    if (n_valid == 0) {
        /*
         * Nothing came back. Three different situations, and reporting them as
         * one would make the device lie about itself:
         *
         *  - the echo line floated under a pull-up AND no echo arrived:
         *    the harness is unplugged.
         *  - the line misbehaved (already high, or a burst that never ended):
         *    it is connected and something is wrong with it.
         *  - the line is driven and quiet: the sensor is fine and there is
         *    simply no echo. On this tank that is the FULL case - the water
         *    sits 50-150 mm away and may be inside the module's blind zone.
         *    Calling that a sensor fault would hide the single most important
         *    state this product exists to measure.
         */
        if (r.presence == HN_PRESENCE_ABSENT)              r.status = HN_STATUS_ABSENT;
        else if (r.echo_stuck_high || r.rise_without_fall) r.status = HN_STATUS_FAULT;
        else                                               r.status = HN_STATUS_NO_TARGET;
        return;
    }

    /* Something answered, so the sensor is there whatever the pull-up probe
     * thought. Behaviour is the better witness. */
    r.presence = HN_PRESENCE_CONFIRMED;

    /*
     * Median first, then reject around it, then average what survives.
     *
     * A plain mean is the wrong tool: one sidewall return or a ghost echo off
     * the fill stream lands hundreds of microseconds away and drags the mean
     * with it. The median cannot be moved by a minority of samples; averaging
     * the survivors still buys back the noise reduction on the good ones.
     */
    uint16_t sorted[HN_US_SAMPLES];
    uint8_t n = (n_valid > HN_US_SAMPLES) ? (uint8_t)HN_US_SAMPLES : n_valid;
    for (uint8_t i = 0; i < n; ++i) sorted[i] = valid[i];
    sort_u16(sorted, n);
    uint16_t median = sorted[n / 2];

    uint32_t sum = 0;
    uint16_t lo = 0xFFFF, hi = 0;
    uint8_t  n_acc = 0;
    for (uint8_t i = 0; i < n; ++i) {
        uint16_t v = sorted[i];
        uint16_t d = (v > median) ? (uint16_t)(v - median) : (uint16_t)(median - v);
        if (d <= HN_US_OUTLIER_US) {
            sum += v;
            if (v < lo) lo = v;
            if (v > hi) hi = v;
            n_acc++;
        }
    }
    r.samples_accepted = n_acc;

    /* n_acc can never be zero - the median is its own neighbour - but a
     * division by zero here would be a hang on a rooftop, so it is guarded. */
    if (n_acc == 0) {
        r.echo_us = median;
        r.status  = HN_STATUS_UNSTABLE;
        return;
    }

    r.echo_us   = (uint16_t)(sum / n_acc);
    r.spread_us = (uint16_t)(hi - lo);

    if (n_acc < HN_US_MIN_ACCEPTED || r.spread_us > HN_US_SPREAD_LIMIT_US) {
        /* Echoes arrived but do not agree: a moving surface, a partial
         * obstruction, or a sensor beginning to fail. The value is still
         * reported - the Hub can decide what to do with it - but it is
         * labelled so nobody treats it as a clean measurement. */
        r.status = HN_STATUS_UNSTABLE;
        return;
    }

#if HN_US_PLAUSIBLE_MAX_US > 0
    /* Optional bench-only geometry check - see hn_config.h. Compiled out
     * entirely on a production build, where the Hub owns tank geometry and the
     * Node has no business having an opinion about it. */
    if (r.echo_us < HN_US_PLAUSIBLE_MIN_US || r.echo_us > HN_US_PLAUSIBLE_MAX_US) {
        /* Advisory only. Still a real measurement, still transmitted raw. */
        r.status = HN_STATUS_OUT_OF_RANGE;
        return;
    }
#endif

    r.status = HN_STATUS_OK;
}

/* ------------------------------------------------------------------------- */
/* Flow switch                                                                */
/* ------------------------------------------------------------------------- */

void hn_classify_flow(const uint16_t *adc, const bool *digital_high,
                      uint8_t n, hn_flow_reading_t &r)
{
    r.samples       = n;
    r.level_adc     = 0;
    r.level_digital = false;
    r.agree         = 0;
    r.state         = HN_FLOW_UNKNOWN;
    r.status        = HN_STATUS_NOT_MEASURED;
    r.presence      = HN_PRESENCE_UNKNOWN;

    if (n == 0) return;

    uint8_t  n_digital_high = 0;
    uint8_t  n_open = 0, n_closed = 0, n_mid = 0;
    uint32_t sum_open = 0, sum_closed = 0, sum_mid = 0;

    for (uint8_t i = 0; i < n; ++i) {
        if (digital_high[i]) n_digital_high++;

        /*
         * Classify every sample individually rather than averaging first.
         * Averaging a chattering switch produces a mid-rail mean out of samples
         * that were each at a rail - indistinguishable from a genuine resistive
         * fault. Those two must never be confused: one is water arriving in the
         * pipe, the other is water in the connector.
         */
        uint16_t a = adc[i];
        if (a <= HN_FLOW_ADC_CLOSED_MAX)    { n_closed++; sum_closed += a; }
        else if (a >= HN_FLOW_ADC_OPEN_MIN) { n_open++;   sum_open   += a; }
        else                                { n_mid++;    sum_mid    += a; }
    }

    r.level_digital = (uint8_t)(n_digital_high * 2) > n;
    r.agree = r.level_digital ? n_digital_high : (uint8_t)(n - n_digital_high);

    /* Report a representative voltage from the majority class, so the number
     * printed next to the state actually describes that state. */
    if (n_mid > 0 && n_mid >= n_open && n_mid >= n_closed)
        r.level_adc = (uint16_t)(sum_mid / n_mid);
    else if (n_open >= n_closed && n_open > 0)
        r.level_adc = (uint16_t)(sum_open / n_open);
    else if (n_closed > 0)
        r.level_adc = (uint16_t)(sum_closed / n_closed);

    /* Presence: anything other than a clean open rail proves a conductive path
     * exists beyond the connector, so the harness is there. A clean open proves
     * nothing either way - an open dry contact and an unplugged cable are the
     * same circuit. */
    r.presence = (n_closed || n_mid) ? HN_PRESENCE_CONFIRMED
                                     : HN_PRESENCE_UNCONFIRMED;

    /* Fault: the node is not at either rail. Roughly 250 kohm to 2.3 Mohm to
     * ground. A dry, healthy contact is either a short or an open; this is
     * neither, so it is water in the contacts, a corroded pin or a chafed
     * cable. */
    if (n_mid > 0) {
        r.status = HN_STATUS_FAULT;
        return;
    }

    /* Fault: the two views of the same node disagree. D5 and A2 are wired to
     * the same net through 100 ohm and 330 ohm respectively. In any healthy
     * condition - including a bouncing contact - their majorities must agree.
     * If they do not, one of the two paths is damaged and neither reading can
     * be trusted. */
    bool analog_majority_high = (uint8_t)(n_open * 2) > n;
    if (analog_majority_high != r.level_digital) {
        r.status = HN_STATUS_FAULT;
        return;
    }

    /* Chatter: a paddle switch on a fill pipe bounces, and it genuinely sits on
     * its threshold as flow starts and stops. Report the majority state, but
     * label it when the samples did not agree. */
    r.status = (r.agree < HN_FLOW_MIN_AGREE) ? HN_STATUS_UNSTABLE : HN_STATUS_OK;

#if HN_FLOW_FILLING_IS_LOW
    r.state = r.level_digital ? HN_FLOW_IDLE : HN_FLOW_FILLING;
#else
    r.state = r.level_digital ? HN_FLOW_FILLING : HN_FLOW_IDLE;
#endif
}
