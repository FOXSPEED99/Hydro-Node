/*
 * hn_packet.h - the Node -> Hub wire format.
 *
 * THIS FILE IS THE CONTRACT. A byte-identical copy lives in the Hub sketch at
 * "Hydro Hub Device/HydroHubLite/hn_packet.h". They must never drift:
 *
 *     make check-protocol
 *
 * fails the build if they differ. Change this file, copy it across, re-run.
 *
 * ---------------------------------------------------------------------------
 * WHY BINARY AND NOT JSON
 * ---------------------------------------------------------------------------
 * The Hub's original protocol was JSON, around 110 bytes. On LoRa the packet
 * length is not a storage question, it is an ENERGY question - airtime is
 * roughly proportional to payload bytes, and the radio draws ~100 mA for every
 * millisecond of it. Against the two-year battery target on 4.4 Ah usable:
 *
 *   JSON ~110 B @ SF9   595 ms airtime   59.5 mA*s/TX   9.6 Ah over 2 y  FAILS
 *   binary  19 B @ SF9   185 ms airtime   18.5 mA*s/TX   3.6 Ah over 2 y  1.2x
 *   binary  19 B @ SF7    57 ms airtime    5.7 mA*s/TX   1.8 Ah over 2 y  2.5x
 *
 * Same information either way. Encoding it as text would cost roughly six
 * times the transmit energy and, at SF9, would put the product about a year
 * short of its own specification. Debuggability is not lost - the Hub decodes
 * this and prints it as JSON on its serial port, so a bench session looks
 * exactly like it would have.
 *
 * ---------------------------------------------------------------------------
 * DESIGN RULES
 * ---------------------------------------------------------------------------
 * - Raw values only. Echo microseconds, the DS18B20's own register, the flow
 *   switch's ADC count. No distances, no percentages, no litres. Every
 *   conversion happens on the Hub, which can be updated without a site visit.
 * - Little-endian, packed, fixed size. No alignment padding, no compiler
 *   dependence.
 * - Every field has a defined "not available" value, so a partially working
 *   Node still sends a valid packet rather than nothing.
 * - Version byte first. A Hub that meets a version it does not know rejects
 *   the packet instead of misreading it.
 */
#ifndef HN_PACKET_H
#define HN_PACKET_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Bump when the layout changes in a way an older Hub would misread. */
#define HN_PROTO_VERSION   0x01

/* Wire size. Both ends check this. */
#define HN_PACKET_BYTES    19

/* ------------------------------------------------------------------------- */
/* Field offsets                                                              */
/* ------------------------------------------------------------------------- */
/*
 *  off  size  field
 *   0    1    version
 *   1    2    pair hash    - CRC-16 of the pair id string; isolation layer 2
 *   3    2    node id      - which Node sent this
 *   5    2    sequence     - wraps at 65535, used to spot lost packets
 *   7    2    echo_us      - RAW round-trip microseconds, 0 = no reading
 *   9    2    temp_raw     - RAW DS18B20 register, HN_TEMP_RAW_NONE = none
 *  11    1    flow_adc8    - RAW A2 count >> 2, so 0..255 spans 0..VCC
 *  12    1    status: ultrasonic
 *  13    1    status: temperature
 *  14    1    status: flow  (also carries the flow state)
 *  15    1    flags
 *  16    1    battery      - decivolts, 0 = not measured (reserved for later)
 *  17    2    CRC-16 over bytes 0..16
 */
#define HN_OFF_VERSION     0
#define HN_OFF_PAIR        1
#define HN_OFF_NODE        3
#define HN_OFF_SEQ         5
#define HN_OFF_ECHO        7
#define HN_OFF_TEMP        9
#define HN_OFF_FLOWADC     11
#define HN_OFF_ST_US       12
#define HN_OFF_ST_TP       13
#define HN_OFF_ST_FL       14
#define HN_OFF_FLAGS       15
#define HN_OFF_BATT        16
#define HN_OFF_CRC         17

/* ------------------------------------------------------------------------- */
/* Wire enums                                                                 */
/* ------------------------------------------------------------------------- */

/* Sensor data quality. Mirrors hn_status_t on the Node; the Node has a
 * static_assert tying the two together so they cannot drift. */
typedef enum {
    HN_W_OK           = 0,
    HN_W_ABSENT       = 1,   /* proven not connected                          */
    HN_W_FAULT        = 2,   /* connected and misbehaving                     */
    HN_W_UNSTABLE     = 3,   /* answered, too noisy to trust                  */
    HN_W_NO_TARGET    = 4,   /* ultrasonic: healthy, nothing came back        */
    HN_W_OUT_OF_RANGE = 5,
    HN_W_NOT_MEASURED = 6,
} hn_wire_status_t;

/* Whether the sensor is physically there. "Cannot tell" is a real answer: an
 * open dry contact and an unplugged one are the same circuit. */
typedef enum {
    HN_W_PRES_UNKNOWN     = 0,
    HN_W_PRES_CONFIRMED   = 1,
    HN_W_PRES_UNCONFIRMED = 2,
    HN_W_PRES_ABSENT      = 3,
} hn_wire_presence_t;

typedef enum {
    HN_W_FLOW_UNKNOWN = 0,
    HN_W_FLOW_IDLE    = 1,
    HN_W_FLOW_FILLING = 2,
} hn_wire_flow_t;

/* Status byte layout: bits 0-2 status, bits 3-4 presence, bits 5-6 flow state
 * (flow sensor byte only), bit 7 spare. */
#define HN_ST_PACK(status, presence)  ((uint8_t)(((status) & 0x07) | (((presence) & 0x03) << 3)))
#define HN_ST_STATUS(b)               ((uint8_t)((b) & 0x07))
#define HN_ST_PRESENCE(b)             ((uint8_t)(((b) >> 3) & 0x03))
#define HN_ST_FLOW_PACK(b, flow)      ((uint8_t)((b) | (((flow) & 0x03) << 5)))
#define HN_ST_FLOW(b)                 ((uint8_t)(((b) >> 5) & 0x03))

/* Flags byte. */
#define HN_FLAG_GATED_BY_FLOW  0x01  /* water was running in during this read -
                                      * the level sample is not trustworthy   */
#define HN_FLAG_FLOW_LEVEL     0x02  /* raw D5 as read: 1 = high (open)       */
#define HN_FLAG_TEMP_CRC_OK    0x04  /* the scratchpad CRC of this reading    */

/* "Not available" markers. */
#define HN_TEMP_RAW_NONE   ((int16_t)0x8000)
#define HN_ECHO_NONE       ((uint16_t)0)
#define HN_BATT_NONE       ((uint8_t)0)

/* ------------------------------------------------------------------------- */
/* Decoded form                                                               */
/* ------------------------------------------------------------------------- */

typedef struct {
    uint8_t  version;
    uint16_t pair_hash;
    uint16_t node_id;
    uint16_t seq;

    uint16_t echo_us;      /* RAW round-trip time; HN_ECHO_NONE if no reading */
    int16_t  temp_raw;     /* RAW DS18B20 register; HN_TEMP_RAW_NONE if none  */
    uint8_t  flow_adc8;    /* RAW A2 >> 2                                     */

    uint8_t  st_us;        /* use HN_ST_STATUS / HN_ST_PRESENCE               */
    uint8_t  st_tp;
    uint8_t  st_fl;        /* also HN_ST_FLOW                                 */

    uint8_t  flags;
    uint8_t  battery_dv;   /* decivolts; HN_BATT_NONE when not measured       */
} hn_packet_t;

/* ------------------------------------------------------------------------- */
/* API                                                                        */
/* ------------------------------------------------------------------------- */

/* CRC-16/CCITT-FALSE. Used for the payload check and for hashing the pair id. */
uint16_t hn_crc16(const uint8_t *data, uint8_t len);

/* Hash a pair-id string into the 16 bits carried on the wire. Both ends run
 * this over the same string, so the strings themselves never fly. */
uint16_t hn_pair_hash(const char *pair_id);

/* Serialise into exactly HN_PACKET_BYTES. Fills version and CRC itself. */
void hn_packet_encode(const hn_packet_t *in, uint8_t *out);

/* Parse and validate: length, version, CRC. Returns 0 on success, or one of
 * the codes below. The pair check is the caller's - it needs the local id. */
#define HN_DEC_OK           0
#define HN_DEC_BAD_LENGTH   1
#define HN_DEC_BAD_VERSION  2
#define HN_DEC_BAD_CRC      3
uint8_t hn_packet_decode(const uint8_t *in, uint8_t len, hn_packet_t *out);

#ifdef __cplusplus
}
#endif

#endif /* HN_PACKET_H */
