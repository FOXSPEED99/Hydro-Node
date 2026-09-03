/*
 * hn_crc8.h - Dallas/Maxim CRC-8, polynomial x^8 + x^5 + x^4 + 1.
 *
 * Its own translation unit because it is pure arithmetic with no hardware
 * dependency, which means it can be tested on a host (make test). It is also
 * the only thing standing between a corrupted 1-Wire transfer and a plausible
 * looking temperature, so it is worth testing.
 */
#ifndef HN_CRC8_H
#define HN_CRC8_H

#include <stdint.h>

uint8_t hn_crc8(const uint8_t *data, uint8_t len);

#endif /* HN_CRC8_H */
