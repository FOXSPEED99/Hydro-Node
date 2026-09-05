#include "NodeLink.h"
#include "config.h"

#include <RadioLib.h>
#include <SPI.h>

/*
 * A dedicated SPI bus for the radio. TFT_eSPI takes HSPI/SPI3 on this board
 * (USE_HSPI_PORT in its User_Setup), so sharing one bus would mean the display
 * and the radio fighting over it - which shows up as occasional corrupt
 * packets rather than as an obvious failure. Same arrangement as the full Hub.
 */
static SPIClass loraSPI(FSPI);
static SX1278 radio = new Module(PIN_LORA_CS, PIN_LORA_DIO0, PIN_LORA_RST, RADIOLIB_NC, loraSPI);

static volatile bool s_packetFlag = false;
static LinkStats s_stats;
static uint16_t s_pairHash = 0;
static bool s_radioOk = false;

#if defined(ESP32)
ICACHE_RAM_ATTR
#endif
static void onPacket() { s_packetFlag = true; }

bool nodeLinkBegin()
{
    s_pairHash = hn_pair_hash(HUB_PAIR_ID);

    loraSPI.begin(PIN_LORA_SCK, PIN_LORA_MISO, PIN_LORA_MOSI, PIN_LORA_CS);
    delay(10);

    /* Every one of these must equal the Node's. A mismatch is not an error on
     * either side - the two radios simply never hear each other. */
    int state = radio.begin(HUB_LORA_FREQ_MHZ,
                            HUB_LORA_BW_KHZ,
                            HUB_LORA_SF,
                            HUB_LORA_CR,
                            HUB_LORA_SYNC_WORD,
                            /* txPower */ 17,
                            HUB_LORA_PREAMBLE_LEN);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[LORA] begin failed: %d (check CS/RST/DIO0 and the SPI pins)\n", state);
        s_radioOk = false;
        return false;
    }

    radio.setCRC(true);                       /* matches the Node's modem config */
    radio.setPacketReceivedAction(onPacket);

    state = radio.startReceive();
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[LORA] startReceive failed: %d\n", state);
        s_radioOk = false;
        return false;
    }

    Serial.printf("[LORA] listening  %.1f MHz  SF%d  BW%.0f  CR4/%d  sync 0x%02X  pair 0x%04X\n",
                  HUB_LORA_FREQ_MHZ, (int)HUB_LORA_SF, HUB_LORA_BW_KHZ,
                  (int)HUB_LORA_CR, HUB_LORA_SYNC_WORD, s_pairHash);
    s_radioOk = true;
    return true;
}

bool nodeLinkPoll()
{
    if (!s_radioOk || !s_packetFlag) return false;
    s_packetFlag = false;

    /*
     * Read into a byte buffer, not a String. The payload is binary and
     * contains zero bytes; a String would truncate at the first one and every
     * packet would look corrupt.
     */
    uint8_t buf[HN_PACKET_BYTES + 8];
    size_t len = radio.getPacketLength();
    if (len > sizeof(buf)) len = sizeof(buf);

    int state = radio.readData(buf, len);
    radio.startReceive();                      /* re-arm immediately */

    if (state != RADIOLIB_ERR_NONE) {
        s_stats.rejected++;
        return false;
    }

    hn_packet_t pkt;
    uint8_t rc = hn_packet_decode(buf, (uint8_t)len, &pkt);
    if (rc != HN_DEC_OK) {
        s_stats.rejected++;
        Serial.printf("[LORA] rejected packet (%s)\n",
                      rc == HN_DEC_BAD_LENGTH  ? "length"
                      : rc == HN_DEC_BAD_VERSION ? "protocol version"
                                                 : "CRC");
        return false;
    }

    /* Isolation layer 2. The sync word already keeps most foreign traffic out;
     * this is what stops a neighbour's Hydro Node, which uses the same sync
     * word, from driving this Hub's display. */
    if (pkt.pair_hash != s_pairHash) {
        s_stats.foreign++;
        return false;
    }

    /* Sequence gaps are the honest measure of link quality - RSSI tells you
     * how loud the packets you received were, not how many you did not. */
    if (s_stats.haveLast) {
        uint16_t expected = (uint16_t)(s_stats.last.seq + 1);
        if (pkt.seq != expected) {
            s_stats.missed += (uint16_t)(pkt.seq - expected);
        }
    }

    s_stats.last = pkt;
    s_stats.haveLast = true;
    s_stats.accepted++;
    s_stats.everReceived = true;
    s_stats.lastPacketMs = millis();
    s_stats.rssi = radio.getRSSI();
    s_stats.snr = radio.getSNR();
    return true;
}

const LinkStats &nodeLinkStats() { return s_stats; }

uint32_t nodeLinkAgeMs(uint32_t nowMs)
{
    if (!s_stats.everReceived) return UINT32_MAX;
    return nowMs - s_stats.lastPacketMs;
}

LinkState nodeLinkState(uint32_t nowMs)
{
    if (!s_stats.everReceived) return LinkState::NeverHeard;
    const uint32_t age = nowMs - s_stats.lastPacketMs;
    if (age > LINK_LOST_MS)  return LinkState::Lost;
    if (age > LINK_STALE_MS) return LinkState::Stale;
    return LinkState::Live;
}
