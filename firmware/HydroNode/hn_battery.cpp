#include "hn_battery.h"
#include "hn_board.h"
#include "hn_config.h"

#include <Arduino.h>
#include <avr/power.h>

uint16_t hn_battery_read_mv()
{
    hn_adc_enable();

    /* AVcc as the reference, the internal 1.1 V bandgap as the input. */
    ADMUX = (uint8_t)(_BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1));

    /* The bandgap needs time to settle after the multiplexer switches to it.
     * The datasheet asks for ~70 us; a couple of milliseconds costs nothing
     * once every two minutes and removes the question entirely. */
    hn_delay_ms(2);

    /* Throw the first conversion away - it carries the mux change. */
    ADCSRA |= _BV(ADSC);
    while (ADCSRA & _BV(ADSC)) { }
    (void)ADC;

    /* Then take a few and use the median, so a single ADC glitch or a
     * transient from the last radio burst cannot become the reading. */
    uint16_t s[3];
    for (uint8_t i = 0; i < 3; ++i) {
        ADCSRA |= _BV(ADSC);
        while (ADCSRA & _BV(ADSC)) { }
        s[i] = ADC;
    }
    for (uint8_t i = 1; i < 3; ++i) {          /* tiny insertion sort */
        uint16_t v = s[i];
        int8_t j = (int8_t)(i - 1);
        while (j >= 0 && s[j] > v) { s[j + 1] = s[j]; --j; }
        s[j + 1] = v;
    }
    const uint16_t adc = s[1];

    hn_adc_disable();

    if (adc == 0) return 0;
    return (uint16_t)(HN_BANDGAP_CAL / (uint32_t)adc);
}
