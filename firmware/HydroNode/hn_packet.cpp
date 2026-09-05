#include "hn_packet.h"

/* ------------------------------------------------------------------------- */
/* Little-endian helpers. Written out by hand rather than memcpy'ing a struct
 * so the wire layout does not depend on the compiler's packing or on the
 * endianness of either end - the Node is an 8-bit AVR, the Hub a 32-bit
 * Xtensa, and they must agree byte for byte.                                 */
/* ------------------------------------------------------------------------- */

static void put_u16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)(v >> 8);
}

static uint16_t get_u16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

uint16_t hn_crc16(const uint8_t *data, uint8_t len)
{
    /* CRC-16/CCITT-FALSE: poly 0x1021, init 0xFFFF, no reflection.
     * Bitwise so it costs 8 bytes of flash instead of a 512-byte table. */
    uint16_t crc = 0xFFFF;
    for (uint8_t i = 0; i < len; ++i) {
        crc ^= (uint16_t)((uint16_t)data[i] << 8);
        for (uint8_t b = 0; b < 8; ++b) {
            crc = (crc & 0x8000u) ? (uint16_t)((crc << 1) ^ 0x1021u)
                                  : (uint16_t)(crc << 1);
        }
    }
    return crc;
}

uint16_t hn_pair_hash(const char *pair_id)
{
    uint8_t len = 0;
    while (pair_id[len] != '\0' && len < 255) len++;
    return hn_crc16((const uint8_t *)pair_id, len);
}

void hn_packet_encode(const hn_packet_t *in, uint8_t *out)
{
    out[HN_OFF_VERSION] = HN_PROTO_VERSION;
    put_u16(out + HN_OFF_PAIR, in->pair_hash);
    put_u16(out + HN_OFF_NODE, in->node_id);
    put_u16(out + HN_OFF_SEQ,  in->seq);
    put_u16(out + HN_OFF_ECHO, in->echo_us);
    put_u16(out + HN_OFF_TEMP, (uint16_t)in->temp_raw);
    out[HN_OFF_FLOWADC] = in->flow_adc8;
    out[HN_OFF_ST_US]   = in->st_us;
    out[HN_OFF_ST_TP]   = in->st_tp;
    out[HN_OFF_ST_FL]   = in->st_fl;
    out[HN_OFF_FLAGS]   = in->flags;
    out[HN_OFF_BATT]    = in->battery_dv;
    put_u16(out + HN_OFF_CRC, hn_crc16(out, HN_OFF_CRC));
}

uint8_t hn_packet_decode(const uint8_t *in, uint8_t len, hn_packet_t *out)
{
    if (len != HN_PACKET_BYTES)              return HN_DEC_BAD_LENGTH;
    if (in[HN_OFF_VERSION] != HN_PROTO_VERSION) return HN_DEC_BAD_VERSION;
    if (get_u16(in + HN_OFF_CRC) != hn_crc16(in, HN_OFF_CRC)) return HN_DEC_BAD_CRC;

    out->version    = in[HN_OFF_VERSION];
    out->pair_hash  = get_u16(in + HN_OFF_PAIR);
    out->node_id    = get_u16(in + HN_OFF_NODE);
    out->seq        = get_u16(in + HN_OFF_SEQ);
    out->echo_us    = get_u16(in + HN_OFF_ECHO);
    out->temp_raw   = (int16_t)get_u16(in + HN_OFF_TEMP);
    out->flow_adc8  = in[HN_OFF_FLOWADC];
    out->st_us      = in[HN_OFF_ST_US];
    out->st_tp      = in[HN_OFF_ST_TP];
    out->st_fl      = in[HN_OFF_ST_FL];
    out->flags      = in[HN_OFF_FLAGS];
    out->battery_dv = in[HN_OFF_BATT];
    return HN_DEC_OK;
}
