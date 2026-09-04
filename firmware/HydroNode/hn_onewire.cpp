#include "hn_onewire.h"
#include "hn_config.h"

#include <Arduino.h>
#include <util/delay.h>

static volatile uint8_t *s_ddr = nullptr;
static volatile uint8_t *s_in  = nullptr;
static uint8_t s_mask = 0;

/*
 * The PORT bit for this pin is left at 0 permanently. That turns the whole
 * driver into DDR manipulation: DDR=1 drives a hard low, DDR=0 is an input with
 * no pull-up and the external 4.7k takes the line high. It is one register
 * write per edge and it makes an accidental internal pull-up impossible.
 */
static inline void ow_drive_low() { *s_ddr |= s_mask; }
static inline void ow_release_hw() { *s_ddr = (uint8_t)(*s_ddr & (uint8_t)~s_mask); }
static inline uint8_t ow_sample() { return (uint8_t)((*s_in & s_mask) ? 1 : 0); }

void hn_ow_begin(uint8_t pin)
{
    uint8_t port = digitalPinToPort(pin);
    s_mask = digitalPinToBitMask(pin);
    s_ddr  = portModeRegister(port);
    s_in   = portInputRegister(port);

    volatile uint8_t *out = portOutputRegister(port);
    uint8_t sreg = SREG;
    cli();
    *out = (uint8_t)(*out & (uint8_t)~s_mask);   /* PORT bit stays 0 forever */
    *s_ddr = (uint8_t)(*s_ddr & (uint8_t)~s_mask);
    SREG = sreg;
}

void hn_ow_release()
{
    if (s_mask) ow_release_hw();
}

hn_ow_reset_t hn_ow_reset()
{
    ow_release_hw();

    /*
     * Before driving anything, confirm the bus can actually be high. If the
     * data line is shorted to ground - a crushed cable, water in the connector -
     * it never rises, and that is a different fault from "no sensor fitted".
     * Reporting them separately is the entire reason for this function's
     * three-valued return.
     */
    uint8_t tries = HN_OW_IDLE_TIMEOUT_US / 2;
    while (!ow_sample()) {
        if (tries-- == 0) return HN_OW_SHORTED;
        _delay_us(2);
    }

    /* The 480 us low is a minimum, so interrupts may stay enabled through it -
     * an ISR can only make it longer, which is harmless, and millis() keeps
     * counting. */
    ow_drive_low();
    _delay_us(HN_OW_RESET_LOW_US);

    /* The presence window is not a minimum - it is a 60-240 us slot that has to
     * be sampled inside. Interrupts off for exactly that long. */
    uint8_t sreg = SREG;
    cli();
    ow_release_hw();
    _delay_us(HN_OW_PRESENCE_WAIT_US);
    uint8_t present = (uint8_t)!ow_sample();
    SREG = sreg;

    _delay_us(HN_OW_RESET_TAIL_US);

    /* After a complete reset slot the bus must have recovered. If it has not,
     * something is holding it down. */
    if (!ow_sample()) return HN_OW_SHORTED;

    return present ? HN_OW_PRESENCE : HN_OW_NO_PRESENCE;
}

static void ow_write_bit(uint8_t v)
{
    uint8_t sreg = SREG;
    cli();
    if (v) {
        ow_drive_low();
        _delay_us(HN_OW_WRITE1_LOW_US);
        ow_release_hw();
        SREG = sreg;
        _delay_us(HN_OW_WRITE1_TAIL_US);
    } else {
        ow_drive_low();
        _delay_us(HN_OW_WRITE0_LOW_US);
        ow_release_hw();
        SREG = sreg;
        _delay_us(HN_OW_WRITE0_TAIL_US);
    }
}

uint8_t hn_ow_read_bit()
{
    uint8_t sreg = SREG;
    cli();
    ow_drive_low();
    _delay_us(HN_OW_READ_LOW_US);
    ow_release_hw();
    _delay_us(HN_OW_READ_SAMPLE_US);
    uint8_t r = ow_sample();
    SREG = sreg;
    _delay_us(HN_OW_READ_TAIL_US);
    return r;
}

void hn_ow_write_byte(uint8_t v)
{
    for (uint8_t i = 0; i < 8; ++i) {
        ow_write_bit((uint8_t)(v & 0x01));
        v = (uint8_t)(v >> 1);          /* 1-Wire is least-significant bit first */
    }
}

uint8_t hn_ow_read_byte()
{
    uint8_t v = 0;
    for (uint8_t i = 0; i < 8; ++i) {
        if (hn_ow_read_bit()) v |= (uint8_t)(1u << i);
    }
    return v;
}

void hn_ow_read_bytes(uint8_t *buf, uint8_t n)
{
    for (uint8_t i = 0; i < n; ++i) buf[i] = hn_ow_read_byte();
}
