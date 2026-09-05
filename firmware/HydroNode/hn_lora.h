/*
 * hn_lora.h - transmit-only SX1278 driver for the Ra-02 (U3).
 *
 * =========================================================================
 *  NEVER TRANSMIT WITHOUT THE ANTENNA CONNECTED.
 *  An open IPEX/SMA connector reflects the full PA output back into the
 *  SX1278 and can destroy it. This is the one irreversible mistake available
 *  in this section, and it takes one forgotten cable.
 * =========================================================================
 *
 * Why not RadioLib, which the Hub uses: flash. The Node has 30720 usable bytes
 * and Section 1 already spends 12 k of it, with a sleep manager and pairing
 * still to come. RadioLib is a full-featured multi-radio stack; this is ~1.5 k
 * and does exactly what a battery-powered transmitter needs. It also keeps
 * direct control of the one register write the power budget depends on -
 * putting the radio into SLEEP (0.2 uA) rather than leaving it in STANDBY
 * (~1.5 mA), a 7500x difference that would swamp everything else.
 *
 * The Hub keeps RadioLib. Only the modem settings have to match between the
 * two ends - frequency, bandwidth, spreading factor, coding rate, sync word,
 * preamble length, explicit header and payload CRC - and those all live in
 * hn_config.h with the Hub's values quoted beside them.
 *
 * OWNERSHIP: this module owns the SPI peripheral and D2/D9/D10/D11/D12/D13.
 * Section 1 parks those pins; building with HN_LORA_ENABLED takes them over.
 */
#ifndef HN_LORA_H
#define HN_LORA_H

#include <stdint.h>
#include <stdbool.h>

/* Reset the radio, verify it answers, configure the modem, leave it asleep.
 * Returns false if the chip does not identify itself - which on this board
 * means a solder fault, since the Ra-02 is not on a connector. */
bool hn_lora_begin();

/* True if begin() found the radio. */
bool hn_lora_present();

/* Send one packet and return the radio to SLEEP. Blocks for the airtime -
 * about 185 ms for a 19-byte packet at SF9, 57 ms at SF7. Returns false on
 * timeout, which means the radio stopped responding mid-transmission. */
bool hn_lora_send(const uint8_t *data, uint8_t len);

/* Force the radio to its lowest-power state. begin() and send() both leave it
 * there already; this exists for the Section 3 sleep manager to be certain. */
void hn_lora_sleep();

/* Airtime of the last successful send, milliseconds. Useful on the bench for
 * checking the energy model against reality. 0 if nothing has been sent. */
uint16_t hn_lora_last_airtime_ms();

#endif /* HN_LORA_H */
