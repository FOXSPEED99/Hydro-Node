#include "hn_board.h"
#include "hn_config.h"

#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

void hn_board_begin()
{
    /* --- Power latch -------------------------------------------------------
     * A1 goes through 100k to the 74HC74's active-low CLR. As a plain INPUT it
     * is high-impedance and R12 (1M to VLATCH) holds CLR high, so the latch
     * stays set. Note it is INPUT and not INPUT_PULLUP: the pull-up would do no
     * harm here, but leaving the pin completely passive makes it obvious in a
     * review that nothing in Section 1 can assert shutdown. */
    pinMode(HN_PIN_LATCH_CLEAR, INPUT);

    /* --- Ultrasonic --------------------------------------------------------
     * TRIG idles low so the module is not being asked for anything. ECHO is a
     * plain input with NO pull-up: the module drives it, and the pull-up is
     * only switched on for the brief presence probe. */
    pinMode(HN_PIN_US_TRIG, OUTPUT);
    digitalWrite(HN_PIN_US_TRIG, LOW);
    pinMode(HN_PIN_US_ECHO, INPUT);

    /* --- Temperature -------------------------------------------------------
     * Both lines low/off means the DS18B20 is unpowered and the 4.7k pull-up
     * has nothing across it. Order matters on the way down (data first, then
     * power) - see hn_temperature.cpp. */
    pinMode(HN_PIN_TEMP_DATA, INPUT);
    pinMode(HN_PIN_TEMP_POWER, OUTPUT);
    digitalWrite(HN_PIN_TEMP_POWER, LOW);

    /* --- Flow switch -------------------------------------------------------
     * INPUT, never INPUT_PULLUP. The board already has a 1 Mohm pull-up (R6);
     * adding the internal ~40 kohm one in parallel would turn a 3.6 uA standby
     * into ~110 uA whenever the contact is closed, which is most of the entire
     * current budget for the two-year target.
     *
     * A2 shares the same node as an analogue input. Its digital input buffer is
     * disabled so that a mid-rail fault voltage cannot sit on the buffer's
     * threshold and draw crossbar current. */
    pinMode(HN_PIN_FLOW_DIGITAL, INPUT);
    /* The A2 digital-input-buffer disable lives in hn_flow_begin(), because it
     * has to be written while the ADC still has a clock - see the note on
     * hn_adc_disable() below. */

    /* --- Buzzer ------------------------------------------------------------ */
    pinMode(HN_PIN_BUZZER, OUTPUT);
    digitalWrite(HN_PIN_BUZZER, LOW);

#if HN_PARK_LORA_PINS
    /* --- Radio -------------------------------------------------------------
     * Section 1 does not use the Ra-02, but it is soldered to this board and
     * powered whenever the latch is on, so "not using it" has to mean something
     * definite rather than leaving six pins floating.
     *
     *   RESET low   - hold the SX1278 in reset; it drives nothing and
     *                 initialises deterministically when Section 2 releases it.
     *   NSS high    - deselected, so it never clocks data off the bus.
     *   SCK/MOSI low- defined outputs, no floating inputs.
     *   MISO        - the radio only drives MISO while NSS is low, so it is
     *                 genuinely high-impedance here and needs a bias; the
     *                 internal pull-up costs nothing because nothing is
     *                 fighting it.
     *   DIO0        - driven low by the radio while in reset, so a plain input
     *                 is correct; a pull-up here would waste ~80 uA fighting it.
     *
     * Section 2 must build with HN_PARK_LORA_PINS 0 and take these over. */
    pinMode(HN_PIN_LORA_RESET, OUTPUT);
    digitalWrite(HN_PIN_LORA_RESET, LOW);
    pinMode(HN_PIN_LORA_NSS, OUTPUT);
    digitalWrite(HN_PIN_LORA_NSS, HIGH);
    pinMode(HN_PIN_LORA_SCK, OUTPUT);
    digitalWrite(HN_PIN_LORA_SCK, LOW);
    pinMode(HN_PIN_LORA_MOSI, OUTPUT);
    digitalWrite(HN_PIN_LORA_MOSI, LOW);
    pinMode(HN_PIN_LORA_MISO, INPUT_PULLUP);
    pinMode(HN_PIN_LORA_DIO0, INPUT);
#endif

    /* --- Genuinely unconnected pins ---------------------------------------
     * A0, A3, A4 and A5 go nowhere on this board [SCH]. A floating CMOS input
     * sits near its switching threshold and draws crossbar current in the tens
     * of microamps, so they get pull-ups: nothing is connected, so the pull-up
     * sources no current at all.
     *
     * A6 and A7 are deliberately absent from this list. On the ATmega328P's
     * TQFP/QFN package those two are analogue-input-only - they have no digital
     * input buffer and no pull-up hardware, so there is nothing to float and
     * nothing to configure. */
    pinMode(A0, INPUT_PULLUP);
    pinMode(A3, INPUT_PULLUP);
    pinMode(A4, INPUT_PULLUP);
    pinMode(A5, INPUT_PULLUP);

    /* D0/D1 belong to the UART and the programming header; leave them alone. */

    /* Peripherals Section 1 never uses. Timer1 is powered up on demand by the
     * ultrasonic driver, and the ADC by hn_adc_enable(). */
    power_timer2_disable();
    power_twi_disable();
    power_spi_disable();
    hn_adc_disable();
}

void hn_delay_ms(uint32_t ms)
{
#if HN_SLEEP_ENABLED
    /*
     * Idle-sleep the wait instead of spinning. IDLE keeps every peripheral
     * clock running - Timer0 for millis(), Timer1 for the echo capture - and
     * only stops the CPU core, so it is safe in the middle of a measurement.
     * Worth ~2.5 mA across the ~250 ms of inter-sample gaps in a cycle.
     *
     * Below ~4 ms it is not worth it: Timer0 only overflows every ~2 ms, so
     * the granularity would dominate the wait.
     */
    if (ms >= 4 && (SREG & _BV(SREG_I))) {
        const uint32_t start = millis();
        while ((millis() - start) < ms) hn_idle_once();
        return;
    }
#endif
    delay(ms);
}

void hn_idle_once()
{
    /* Sleeping with interrupts disabled would never wake. Refuse rather than
     * hang - a stalled Node on a roof is a site visit. */
    if (!(SREG & _BV(SREG_I))) return;

    /* SLEEP_MODE_IDLE stops the CPU clock but leaves the peripheral clocks
     * running, so Timer1, the input capture unit, Timer0/millis() and the UART
     * all keep working and any of their interrupts wakes us. That is what makes
     * it safe to sleep in the middle of an ultrasonic measurement. */
    set_sleep_mode(SLEEP_MODE_IDLE);
    sleep_enable();
    sleep_cpu();
    sleep_disable();
}

void hn_adc_enable()
{
    power_adc_enable();
    ADCSRA |= _BV(ADEN);
    /* The first conversion after enabling takes 25 ADC clocks instead of 13.
     * analogRead() waits for ADSC to clear, so it absorbs that automatically -
     * no delay is needed here. */
}

/* NOTE for anyone adding ADC work later: once PRADC is set the ADC block has
 * no clock, and writes to its registers - DIDR0 included - are not guaranteed
 * to land. Configure the ADC between hn_adc_enable() and hn_adc_disable(). */
void hn_adc_disable()
{
    ADCSRA &= (uint8_t)~_BV(ADEN);
    power_adc_disable();
}
