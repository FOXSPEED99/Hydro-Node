#include "hn_flow.h"
#include "hn_board.h"
#include "hn_config.h"
#include "hn_filter.h"

#include <Arduino.h>
#include <util/delay.h>

void hn_flow_begin()
{
    /* INPUT, never INPUT_PULLUP - the board's 1 Mohm pull-up is what keeps the
     * closed-contact standby current at ~3.6 uA instead of ~110 uA. */
    pinMode(HN_PIN_FLOW_DIGITAL, INPUT);

    /* Disable A2's digital input buffer so a mid-rail fault voltage cannot sit
     * on its switching threshold drawing crossbar current. This has to be
     * written while the ADC still has a clock - see hn_adc_disable(). */
    hn_adc_enable();
    DIDR0 |= _BV(HN_ADC_CH_FLOW);
    hn_adc_disable();
}

/*
 * With the contact open, the ADC looks into 1 Mohm - a hundred times the
 * 10 kohm source impedance the ATmega328P's sample-and-hold is specified for.
 * The first conversion after a channel change borrows charge from the node to
 * fill the 14 pF S/H capacitor and reads low; the node then has to recover
 * through that 1 Mohm before the reading means anything.
 *
 * So: one conversion to select the channel and take the charge hit, a settling
 * delay, then the conversion that counts.
 */
static uint16_t flow_analog_read()
{
    (void)analogRead(HN_PIN_FLOW_ANALOG);
    _delay_us(HN_FLOW_ADC_SETTLE_US);
    return (uint16_t)analogRead(HN_PIN_FLOW_ANALOG);
}

static void flow_sample_pass(uint16_t *adc, bool *digital_high)
{
    hn_adc_enable();
    for (uint8_t i = 0; i < HN_FLOW_SAMPLES; ++i) {
        adc[i]          = flow_analog_read();
        digital_high[i] = (digitalRead(HN_PIN_FLOW_DIGITAL) == HIGH);
        if (i + 1 < HN_FLOW_SAMPLES) hn_delay_ms(HN_FLOW_SAMPLE_GAP_MS);
    }
    hn_adc_disable();
}

void hn_flow_read(hn_flow_reading_t &r)
{
    uint16_t adc[HN_FLOW_SAMPLES];
    bool     digital_high[HN_FLOW_SAMPLES];

    flow_sample_pass(adc, digital_high);

    /* Voting, cross-checking and fault classification are pure logic and live
     * in hn_filter.cpp so they can be tested on a host machine. */
    hn_classify_flow(adc, digital_high, HN_FLOW_SAMPLES, r);

    if (r.status == HN_STATUS_FAULT) {
        /*
         * Before believing a fault, let the node settle and look again.
         *
         * The 40 ms sampling window is shorter than the ~120 ms the node needs
         * to climb back through the 1 Mohm pull-up after the contact opens, so
         * a perfectly normal end-of-fill can be caught mid-transition and looks
         * identical to a resistive fault. Waiting 3*tau and re-reading tells
         * them apart: the transient is gone, the fault is not.
         *
         * A digital-versus-analogue disagreement will simply reproduce, so
         * retrying on any fault costs one extra look in a case that is already
         * abnormal, and never runs on a healthy cycle.
         */
        hn_delay_ms(HN_FLOW_SETTLE_MS);
        flow_sample_pass(adc, digital_high);
        hn_classify_flow(adc, digital_high, HN_FLOW_SAMPLES, r);
    }
}
