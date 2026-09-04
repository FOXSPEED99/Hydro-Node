#include "hn_status.h"
#include <Arduino.h>

const __FlashStringHelper *hn_status_name(hn_status_t s)
{
    switch (s) {
    case HN_STATUS_OK:            return F("OK");
    case HN_STATUS_ABSENT:        return F("NOT CONNECTED");
    case HN_STATUS_FAULT:         return F("FAULT");
    case HN_STATUS_UNSTABLE:      return F("UNSTABLE");
    case HN_STATUS_NO_TARGET:     return F("NO ECHO");
    case HN_STATUS_OUT_OF_RANGE:  return F("OUT OF RANGE");
    case HN_STATUS_NOT_MEASURED:  return F("NOT MEASURED");
    }
    return F("?");
}

const __FlashStringHelper *hn_status_code(hn_status_t s)
{
    switch (s) {
    case HN_STATUS_OK:            return F("OK");
    case HN_STATUS_ABSENT:        return F("ABSENT");
    case HN_STATUS_FAULT:         return F("FAULT");
    case HN_STATUS_UNSTABLE:      return F("UNSTABLE");
    case HN_STATUS_NO_TARGET:     return F("NOECHO");
    case HN_STATUS_OUT_OF_RANGE:  return F("RANGE");
    case HN_STATUS_NOT_MEASURED:  return F("SKIP");
    }
    return F("?");
}

const __FlashStringHelper *hn_presence_name(hn_presence_t p)
{
    switch (p) {
    case HN_PRESENCE_UNKNOWN:      return F("not probed");
    case HN_PRESENCE_CONFIRMED:    return F("connected");
    case HN_PRESENCE_UNCONFIRMED:  return F("cannot tell");
    case HN_PRESENCE_ABSENT:       return F("MISSING");
    }
    return F("?");
}

const __FlashStringHelper *hn_presence_code(hn_presence_t p)
{
    switch (p) {
    case HN_PRESENCE_UNKNOWN:      return F("UNK");
    case HN_PRESENCE_CONFIRMED:    return F("YES");
    case HN_PRESENCE_UNCONFIRMED:  return F("MAYBE");
    case HN_PRESENCE_ABSENT:       return F("NO");
    }
    return F("?");
}
