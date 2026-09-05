/*
 * FieldLog.h - what you actually want to know after a month.
 *
 * The dashboard answers "how much water is there right now". This answers a
 * different question: "did the system work, and what did it cost". They need
 * different data, and the second kind is only obtainable by accumulating it as
 * it happens - you cannot reconstruct a month of link reliability from the
 * final screen.
 *
 * Everything is mirrored to NVS, so a power cut costs at most an hour of
 * counters rather than the whole test.
 */
#pragma once

#include "hn_packet.h"

#include <Arduino.h>

struct FieldStats {
    uint32_t bootCount     = 0;
    uint32_t upSeconds     = 0;   /* accumulated across reboots */

    uint32_t accepted      = 0;
    uint32_t missed        = 0;   /* sequence gaps */
    uint32_t rejected      = 0;   /* bad CRC / version / length */
    uint32_t foreign       = 0;   /* another pair's node */

    uint32_t worstGapSec   = 0;   /* longest silence between packets */

    int16_t  rssiMin       = 0;
    int16_t  rssiMax       = 0;
    int16_t  snrMinTenths  = 0;

    uint16_t battFirstMv   = 0;
    uint16_t battLastMv    = 0;
    uint16_t battMinMv     = 0;

    /* Cycles in which each sensor was NOT ok. The ratio against `accepted` is
     * the useful figure - an occasional no-echo is normal, a rising trend is a
     * sensor going. */
    uint32_t faultLevel    = 0;
    uint32_t faultTemp     = 0;
    uint32_t faultFlow     = 0;
    uint32_t noEcho        = 0;   /* healthy sensor, nothing came back */
    uint32_t fillingCycles = 0;
};

void fieldLogBegin();
void fieldLogOnPacket(const hn_packet_t &p, float rssi, float snr, uint32_t gapMs);
void fieldLogOnReject(bool foreign);
void fieldLogTick(uint32_t nowMs);      /* accumulates uptime */
void fieldLogSaveNow();
const FieldStats &fieldLogStats();

/* Reliability as a percentage, 0-100. Returns -1 when nothing has arrived. */
int fieldLogReliabilityPct();
