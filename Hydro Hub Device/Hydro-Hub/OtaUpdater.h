#pragma once

#include <Arduino.h>

// Remote firmware updates (spec §9): the cloud advertises a release through
// device_sync; this module streams it over HTTPS into the inactive OTA slot
// (partitions.csv has app0/app1) and reboots into it.
namespace OtaUpdater {

// True when `version` should be installed (differs from the running firmware
// and hasn't already failed this boot — avoids a crash/retry loop on a bad
// binary).
bool shouldUpdate(const char *version);

// Download + flash + reboot. Blocks the calling task for the whole download;
// feeds the task watchdog from the progress callback. Returns only on failure.
bool applyUpdate(const char *url, const char *version);

}  // namespace OtaUpdater
