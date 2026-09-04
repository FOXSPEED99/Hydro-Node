/*
 * hn_onewire.h - a minimal 1-Wire master for a single-device bus.
 *
 * Why this exists instead of a library dependency: the whole point of Section 1
 * is telling a healthy sensor apart from a missing or broken one, and on 1-Wire
 * that information lives in the reset slot - whether the bus rises at all, and
 * whether anything answers. General-purpose libraries collapse that into a
 * boolean. This one does not, and it is 150 lines.
 *
 * Timing follows Maxim application note 126 (standard speed). The bit slots are
 * driven with cycle-accurate _delay_us() and direct port access; the Arduino
 * digitalWrite() takes ~4 us at 8 MHz, which would destroy a 6 us pulse.
 */
#ifndef HN_ONEWIRE_H
#define HN_ONEWIRE_H

#include <stdint.h>
#include "hn_crc8.h"   /* every 1-Wire transfer is CRC-checked */

enum hn_ow_reset_t : uint8_t {
    HN_OW_PRESENCE = 0,   /* a device pulled the bus low - somebody is there   */
    HN_OW_NO_PRESENCE,    /* bus rises and stays high - nothing connected      */
    HN_OW_SHORTED,        /* bus never rises - data line shorted to ground     */
};

/* Capture the port registers for `pin`. Call once before any other function. */
void hn_ow_begin(uint8_t pin);

hn_ow_reset_t hn_ow_reset();

/* Read a single time slot. Used to poll a DS18B20 for conversion-complete:
 * an externally powered device holds the bus low while converting and lets it
 * float high the moment it is done, which is faster and more honest than
 * waiting out the datasheet's worst-case conversion time. */
uint8_t hn_ow_read_bit();

void    hn_ow_write_byte(uint8_t v);
uint8_t hn_ow_read_byte();
void    hn_ow_read_bytes(uint8_t *buf, uint8_t n);

/* Release the bus (input, no internal pull-up). The external 4.7k referenced to
 * the power GPIO is the only thing that should ever pull this line high. */
void hn_ow_release();

/* ROM commands used by this firmware. */
#define HN_OW_CMD_READ_ROM  0x33
#define HN_OW_CMD_SKIP_ROM  0xCC

#endif /* HN_ONEWIRE_H */
