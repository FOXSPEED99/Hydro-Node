#include "hn_telemetry.h"

/*
 * The wire enums in hn_packet.h are a copy of the internal ones, because that
 * header has to compile on the Hub where hn_status.h does not exist. These
 * assertions are what stop the copy drifting: renumber an internal enum and
 * the build fails here rather than the Hub silently showing "FAULT" where the
 * Node meant "UNSTABLE".
 */
static_assert((int)HN_STATUS_OK           == (int)HN_W_OK,           "wire status drift");
static_assert((int)HN_STATUS_ABSENT       == (int)HN_W_ABSENT,       "wire status drift");
static_assert((int)HN_STATUS_FAULT        == (int)HN_W_FAULT,        "wire status drift");
static_assert((int)HN_STATUS_UNSTABLE     == (int)HN_W_UNSTABLE,     "wire status drift");
static_assert((int)HN_STATUS_NO_TARGET    == (int)HN_W_NO_TARGET,    "wire status drift");
static_assert((int)HN_STATUS_OUT_OF_RANGE == (int)HN_W_OUT_OF_RANGE, "wire status drift");
static_assert((int)HN_STATUS_NOT_MEASURED == (int)HN_W_NOT_MEASURED, "wire status drift");

static_assert((int)HN_PRESENCE_UNKNOWN     == (int)HN_W_PRES_UNKNOWN,     "wire presence drift");
static_assert((int)HN_PRESENCE_CONFIRMED   == (int)HN_W_PRES_CONFIRMED,   "wire presence drift");
static_assert((int)HN_PRESENCE_UNCONFIRMED == (int)HN_W_PRES_UNCONFIRMED, "wire presence drift");
static_assert((int)HN_PRESENCE_ABSENT      == (int)HN_W_PRES_ABSENT,      "wire presence drift");

static_assert((int)HN_FLOW_UNKNOWN == (int)HN_W_FLOW_UNKNOWN, "wire flow drift");
static_assert((int)HN_FLOW_IDLE    == (int)HN_W_FLOW_IDLE,    "wire flow drift");
static_assert((int)HN_FLOW_FILLING == (int)HN_W_FLOW_FILLING, "wire flow drift");

void hn_telemetry_build(const hn_reading_t &r, uint16_t pair_hash,
                        uint16_t node_id, uint16_t battery_mv, hn_packet_t &out)
{
    out.version   = HN_PROTO_VERSION;
    out.pair_hash = pair_hash;
    out.node_id   = node_id;

    /* The cycle counter is 32-bit locally and 16-bit on the wire. Truncating is
     * deliberate: the Hub only uses it to spot gaps, and 65535 cycles is 45
     * days at a 2-minute interval - far longer than any window it reasons
     * over. Spending two more bytes of airtime on it would be silly. */
    out.seq = (uint16_t)r.seq;

    /*
     * Only send an echo time the Node actually believes. UNSTABLE readings do
     * go out - the Hub is entitled to see a noisy-but-real measurement, and the
     * status byte tells it so - but ABSENT, FAULT and NO_TARGET carry no
     * number at all. Sending the last good value, or a zero that looks like a
     * distance, is how a dead sensor ends up displayed as a full tank.
     */
    out.echo_us = (r.ultrasonic.status == HN_STATUS_OK ||
                   r.ultrasonic.status == HN_STATUS_OUT_OF_RANGE ||
                   r.ultrasonic.status == HN_STATUS_UNSTABLE)
                      ? r.ultrasonic.echo_us
                      : HN_ECHO_NONE;

    /* Same rule for temperature, with the extra condition that the scratchpad
     * CRC held - an unverified reading is not a reading. */
    out.temp_raw = (r.temperature.crc_ok &&
                    (r.temperature.status == HN_STATUS_OK ||
                     r.temperature.status == HN_STATUS_OUT_OF_RANGE))
                       ? r.temperature.raw
                       : HN_TEMP_RAW_NONE;

    /* 10-bit ADC down to 8. Losing the bottom two bits costs ~14 mV of
     * resolution on a signal whose decision bands are hundreds of millivolts
     * wide, and saves a byte of airtime on every packet forever. */
    out.flow_adc8 = (uint8_t)(r.flow.level_adc >> 2);

    out.st_us = HN_ST_PACK(r.ultrasonic.status, r.ultrasonic.presence);
    out.st_tp = HN_ST_PACK(r.temperature.status, r.temperature.presence);
    out.st_fl = HN_ST_FLOW_PACK(HN_ST_PACK(r.flow.status, r.flow.presence),
                                r.flow.state);

    out.flags = 0;
    if (r.level_gated_by_flow) out.flags |= HN_FLAG_GATED_BY_FLOW;
    if (r.flow.level_digital)  out.flags |= HN_FLAG_FLOW_LEVEL;
    if (r.temperature.crc_ok)  out.flags |= HN_FLAG_TEMP_CRC_OK;

    out.battery_dv = (battery_mv == 0) ? HN_BATT_NONE : HN_BATT_ENCODE(battery_mv);
}
