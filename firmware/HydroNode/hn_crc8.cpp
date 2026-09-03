#include "hn_crc8.h"

uint8_t hn_crc8(const uint8_t *data, uint8_t len)
{
    /* Bitwise form: a few bytes of flash instead of a 256-byte lookup table,
     * and a whole 9-byte scratchpad is only 72 iterations. */
    uint8_t crc = 0;
    while (len--) {
        uint8_t b = *data++;
        for (uint8_t i = 0; i < 8; ++i) {
            uint8_t mix = (uint8_t)((crc ^ b) & 0x01);
            crc = (uint8_t)(crc >> 1);
            if (mix) crc ^= 0x8C;
            b = (uint8_t)(b >> 1);
        }
    }
    return crc;
}
