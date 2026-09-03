/*
 * Minimal Arduino shim, host builds only.
 *
 * The pure-logic units (hn_filter.cpp, hn_crc8.cpp) reach Arduino.h only
 * through hn_status.h, which needs __FlashStringHelper for its name lookups.
 * Nothing here is used by the logic itself - it exists so the same source that
 * ships to the device compiles unmodified on a workstation.
 */
#ifndef HN_TEST_ARDUINO_SHIM_H
#define HN_TEST_ARDUINO_SHIM_H

#include <stdint.h>
#include <stddef.h>

class __FlashStringHelper;
#define F(str) (reinterpret_cast<const __FlashStringHelper *>(str))

#endif
