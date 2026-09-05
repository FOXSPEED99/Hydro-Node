/*
 * NodeLink.h - receive side of the Node link.
 *
 * Owns the radio, validates packets, and keeps enough history to say something
 * honest about link quality. The distinction it exists to preserve: a Node that
 * is transmitting "the sensor is broken" and a Node that has gone off the air
 * look identical if all you track is the last good reading. One is a sensor
 * fault, the other is a dead node or a flat battery, and the screen must not
 * blur them together.
 */
#pragma once

#include "hn_packet.h"

#include <Arduino.h>

enum class LinkState : uint8_t {
    NeverHeard,   /* nothing since boot                                    */
    Live,         /* a packet arrived recently                             */
    Stale,        /* overdue - the level on screen may no longer be true   */
    Lost,         /* long overdue - treat the displayed data as history    */
};

struct LinkStats {
    bool     everReceived = false;
    uint32_t lastPacketMs = 0;

    uint32_t accepted   = 0;   /* passed length, version, CRC and pair id   */
    uint32_t rejected   = 0;   /* corrupt or wrong protocol                 */
    uint32_t foreign    = 0;   /* valid, but from someone else's pair       */
    uint32_t missed     = 0;   /* gaps in the sequence number               */

    float    rssi = 0;
    float    snr  = 0;

    bool        haveLast = false;
    hn_packet_t last{};
};

/* Bring up the radio. Returns false if it does not answer - the screen then
 * shows a hardware fault rather than an empty dashboard forever. */
bool nodeLinkBegin();

/* Call often. Returns true when a new, accepted packet has just landed. */
bool nodeLinkPoll();

const LinkStats &nodeLinkStats();
LinkState nodeLinkState(uint32_t nowMs);

/* Milliseconds since the last accepted packet, or UINT32_MAX if never. */
uint32_t nodeLinkAgeMs(uint32_t nowMs);
