/*
 * hn_status.h - the vocabulary every sensor module reports in.
 *
 * Two orthogonal questions are answered separately on purpose:
 *
 *   status   - is the DATA usable?
 *   presence - is the SENSOR physically there?
 *
 * They are not the same question. An ultrasonic sensor that is definitely
 * connected can still return no echo (a full tank inside the blind zone), and a
 * flow switch that reads "open" is indistinguishable from one that has been
 * unplugged. Collapsing the two into a single "ok/error" flag is what makes a
 * field device lie about itself.
 */
#ifndef HN_STATUS_H
#define HN_STATUS_H

#include <stdint.h>
#include <Arduino.h>   /* for __FlashStringHelper */

enum hn_status_t : uint8_t {
    HN_STATUS_OK = 0,        /* data is good                                   */
    HN_STATUS_ABSENT,        /* positive evidence the sensor is not connected  */
    HN_STATUS_FAULT,         /* something is there, and it is misbehaving      */
    HN_STATUS_UNSTABLE,      /* it answered, but the data is too noisy to trust*/
    HN_STATUS_NO_TARGET,     /* ultrasonic: healthy sensor, nothing came back  */
    HN_STATUS_OUT_OF_RANGE,  /* stable value, outside the plausible window     */
    HN_STATUS_NOT_MEASURED,  /* module was skipped this cycle                  */
};

/*
 * Presence is deliberately a three-state answer plus "unknown". "I cannot
 * tell" is a legitimate result and has to be representable: a two-wire dry
 * contact that is open looks exactly like a two-wire dry contact that has been
 * unplugged, and no amount of firmware changes that.
 */
enum hn_presence_t : uint8_t {
    HN_PRESENCE_UNKNOWN = 0,   /* not probed yet                               */
    HN_PRESENCE_CONFIRMED,     /* positive electrical evidence it is connected */
    HN_PRESENCE_UNCONFIRMED,   /* no evidence either way - see the note above  */
    HN_PRESENCE_ABSENT,        /* positive electrical evidence it is not there */
};

/* Both return pointers into flash; print with Serial.print(...) as usual. */
const __FlashStringHelper *hn_status_name(hn_status_t s);
const __FlashStringHelper *hn_presence_name(hn_presence_t p);

/* Short forms for the machine-readable line. */
const __FlashStringHelper *hn_status_code(hn_status_t s);
const __FlashStringHelper *hn_presence_code(hn_presence_t p);

/* True when the data may be used as a measurement. OUT_OF_RANGE is included:
 * the value is real, it is just outside what this installation expects, and the
 * Hub is entitled to see it. */
static inline bool hn_status_is_usable(hn_status_t s)
{
    return s == HN_STATUS_OK || s == HN_STATUS_OUT_OF_RANGE;
}

#endif /* HN_STATUS_H */
