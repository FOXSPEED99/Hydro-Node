#include "hn_sleep.h"
#include "hn_config.h"

#include <Arduino.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

static bool s_wdtReset = false;

/*
 * Clear the watchdog before main() runs.
 *
 * After a watchdog reset the AVR comes back with the watchdog still enabled
 * and set to its SHORTEST timeout. If the sketch does not disable it within
 * ~16 ms - and Arduino's init() alone takes longer - the chip resets again,
 * forever. The device becomes a brick that only a chip erase recovers, on a
 * roof, which is the worst possible place for it.
 *
 * .init3 runs before the C runtime and before main(), which is early enough.
 */
static uint8_t s_mcusr __attribute__((section(".noinit")));
void hn_early_wdt_off(void) __attribute__((naked, used, section(".init3")));
void hn_early_wdt_off(void)
{
    s_mcusr = MCUSR;
    MCUSR = 0;
    wdt_disable();
}

/* Empty by design: the interrupt exists only to end sleep_cpu(). */
ISR(WDT_vect) { }

void hn_sleep_begin()
{
    s_wdtReset = (s_mcusr & _BV(WDRF)) != 0;
    wdt_disable();
}

bool hn_sleep_was_watchdog_reset() { return s_wdtReset; }

/* ------------------------------------------------------------------------- */
/* Reset watchdog, for the awake portion                                      */
/* ------------------------------------------------------------------------- */

void hn_sleep_guard_start()
{
#if HN_WDT_GUARD_ENABLED
    wdt_enable(WDTO_8S);
#endif
}

void hn_sleep_guard_kick()
{
#if HN_WDT_GUARD_ENABLED
    wdt_reset();
#endif
}

void hn_sleep_guard_stop()
{
#if HN_WDT_GUARD_ENABLED
    wdt_disable();
#endif
}

/* ------------------------------------------------------------------------- */
/* Power-down sleep                                                           */
/* ------------------------------------------------------------------------- */

/* Watchdog prescaler codes and what each is worth, longest first. */
struct WdtStep { uint8_t bits; uint16_t ms; };
static const WdtStep kSteps[] = {
    { 0x21, 8000 },   /* WDP3 | WDP0 */
    { 0x20, 4000 },   /* WDP3        */
    { 0x07, 2000 },
    { 0x06, 1000 },
    { 0x05,  500 },
    { 0x04,  250 },
    { 0x03,  120 },
    { 0x02,   60 },
    { 0x01,   30 },
    { 0x00,   15 },
};

/* Arm the watchdog in INTERRUPT mode - WDIE set, WDE clear. Interrupt mode
 * wakes us; reset mode would restart the chip instead. */
static void wdt_interrupt_arm(uint8_t bits)
{
    const uint8_t sreg = SREG;
    cli();
    wdt_reset();
    MCUSR &= (uint8_t)~_BV(WDRF);
    /* The timed sequence: WDCE must be set, then the real value written within
     * four cycles, or the hardware ignores it. */
    WDTCSR = (uint8_t)(_BV(WDCE) | _BV(WDE));
    WDTCSR = (uint8_t)(_BV(WDIE) | bits);
    SREG = sreg;
}

static void power_down_once(uint8_t bits)
{
    wdt_interrupt_arm(bits);

    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    cli();
    sleep_enable();
#if defined(sleep_bod_disable)
    /* The brown-out detector costs ~20 uA, which is four times the sleeping
     * CPU. It can only be disabled in a timed sequence immediately before
     * sleeping, and the hardware re-enables it on wake. */
    sleep_bod_disable();
#endif
    sei();
    sleep_cpu();
    sleep_disable();

    /* Leave the watchdog off between chunks so a stray timeout cannot reset
     * us while we are awake and unguarded. */
    wdt_disable();
}

void hn_sleep_ms(uint32_t ms)
{
    /*
     * The ADC and any leftover peripheral clocks must be off before power-down
     * or they keep drawing. hn_board_begin() already disables what Section 1
     * does not use; this is the belt-and-braces for anything switched on
     * during a cycle and not switched back off.
     */
    const uint8_t adcsra = ADCSRA;
    ADCSRA &= (uint8_t)~_BV(ADEN);

    while (ms >= 15) {
        for (uint8_t i = 0; i < sizeof(kSteps) / sizeof(kSteps[0]); ++i) {
            if (ms >= kSteps[i].ms) {
                power_down_once(kSteps[i].bits);
                ms -= kSteps[i].ms;
                break;
            }
        }
    }

    ADCSRA = adcsra;
}
