#include "hn_reading.h"
#include <string.h>

void hn_reading_clear(hn_reading_t &r)
{
    memset(&r, 0, sizeof(r));

    r.ultrasonic.status    = HN_STATUS_NOT_MEASURED;
    r.ultrasonic.presence  = HN_PRESENCE_UNKNOWN;
    r.temperature.status   = HN_STATUS_NOT_MEASURED;
    r.temperature.presence = HN_PRESENCE_UNKNOWN;
    r.flow.status          = HN_STATUS_NOT_MEASURED;
    r.flow.presence        = HN_PRESENCE_UNKNOWN;
    r.flow.state           = HN_FLOW_UNKNOWN;
}
