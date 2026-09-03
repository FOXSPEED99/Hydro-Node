#include "hn_temperature.h"
#include "hn_onewire.h"
#include "hn_board.h"
#include "hn_config.h"

#include <Arduino.h>
#include <util/delay.h>
#include <string.h>

#define DS18B20_FAMILY_CODE      0x28
#define DS18B20_CMD_CONVERT_T    0x44
#define DS18B20_CMD_WRITE_SCRATCH 0x4E
#define DS18B20_CMD_READ_SCRATCH 0xBE

static uint8_t s_rom[8];
static bool    s_rom_valid = false;

/* ------------------------------------------------------------------------- */
/* Power gating                                                               */
/* ------------------------------------------------------------------------- */

static void temp_power_up()
{
    /* Data line stays high-impedance while VDD rises. The external 4.7k is
     * referenced to this same pin, so the bus follows the rail up. */
    hn_ow_release();
    digitalWrite(HN_PIN_TEMP_POWER, HIGH);
    hn_delay_ms(HN_TEMP_POWERUP_MS);
}

static void temp_power_down()
{
    /*
     * ORDER MATTERS. Release the data line first, then drop the power pin.
     *
     * If the data pin were left driven high while VDD went to 0, it would
     * back-feed the sensor through its input protection diode and the sensor
     * would stay half-alive at an undefined voltage - which both wastes current
     * and leaves the part in a state where its next power-on reset may not
     * happen. Releasing first means the 4.7k discharges the bus into the power
     * pin as that pin is pulled low, and everything ends at 0 V.
     */
    hn_ow_release();
    digitalWrite(HN_PIN_TEMP_POWER, LOW);
}

/* ------------------------------------------------------------------------- */
/* Startup: identify the device                                               */
/* ------------------------------------------------------------------------- */

void hn_temperature_begin()
{
    hn_ow_begin(HN_PIN_TEMP_DATA);
    s_rom_valid = false;
    memset(s_rom, 0, sizeof(s_rom));

    temp_power_up();

    if (hn_ow_reset() == HN_OW_PRESENCE) {
        /* READ ROM is only legal with a single device on the bus, which is what
         * this connector has. It costs one transaction and it proves the thing
         * that answered is actually a DS18B20 and not, say, a DS2401 or a
         * miswired harness that happens to pull the line low. */
        hn_ow_write_byte(HN_OW_CMD_READ_ROM);
        hn_ow_read_bytes(s_rom, 8);
        s_rom_valid = (s_rom[0] == DS18B20_FAMILY_CODE) &&
                      (hn_crc8(s_rom, 7) == s_rom[7]);
    }

    temp_power_down();
}

/* ------------------------------------------------------------------------- */
/* Measurement                                                                */
/* ------------------------------------------------------------------------- */

static void temp_clear(hn_temperature_reading_t &r)
{
    r.status          = HN_STATUS_NOT_MEASURED;
    r.presence        = HN_PRESENCE_UNKNOWN;
    r.raw             = 0;
    r.resolution_bits = HN_TEMP_RESOLUTION_BITS;
    r.crc_ok          = false;
    r.rom_valid       = s_rom_valid;
    memcpy(r.rom, s_rom, sizeof(r.rom));
}

bool hn_temperature_start(hn_temperature_reading_t &r)
{
    temp_clear(r);
    temp_power_up();

    switch (hn_ow_reset()) {
    case HN_OW_NO_PRESENCE:
        /* Bus rises cleanly, nobody answers: the sensor is not there. */
        r.presence = HN_PRESENCE_ABSENT;
        r.status   = HN_STATUS_ABSENT;
        temp_power_down();
        return false;

    case HN_OW_SHORTED:
        /* Bus will not come up: the data line is held down. A sensor cannot do
         * that on its own, so this is cable or connector damage. */
        r.presence = HN_PRESENCE_CONFIRMED;   /* something is loading the line */
        r.status   = HN_STATUS_FAULT;
        temp_power_down();
        return false;

    case HN_OW_PRESENCE:
        break;
    }

    r.presence = HN_PRESENCE_CONFIRMED;

    /*
     * Write the resolution on every cycle rather than programming the sensor's
     * EEPROM once. The sensor is power-cycled between readings, so its config
     * register reloads from EEPROM each time and would otherwise have to be
     * trusted; writing it costs ~2 ms of bus traffic, and it means a
     * replacement sensor fitted in the field is configured correctly on its
     * first cycle with no commissioning step. The EEPROM is deliberately not
     * touched - it has a finite write endurance and nothing here needs it.
     *
     * TH/TL are the alarm thresholds; this design does not use the alarm
     * search, so they are set to harmless values.
     */
    hn_ow_write_byte(HN_OW_CMD_SKIP_ROM);
    hn_ow_write_byte(DS18B20_CMD_WRITE_SCRATCH);
    hn_ow_write_byte(0x7F);                    /* TH */
    hn_ow_write_byte(0x80);                    /* TL */
    hn_ow_write_byte(HN_TEMP_CONFIG_BYTE);

    if (hn_ow_reset() != HN_OW_PRESENCE) {
        r.status = HN_STATUS_FAULT;
        temp_power_down();
        return false;
    }

    hn_ow_write_byte(HN_OW_CMD_SKIP_ROM);
    hn_ow_write_byte(DS18B20_CMD_CONVERT_T);

    return true;
}

void hn_temperature_finish(hn_temperature_reading_t &r)
{
    if (r.status == HN_STATUS_ABSENT || r.status == HN_STATUS_FAULT) {
        temp_power_down();          /* start() already failed and depowered;
                                     * calling again is harmless and keeps every
                                     * exit path identical. */
        return;
    }

    /*
     * The sensor has its own VDD, so it signals completion by releasing the
     * bus. Polling that is faster than waiting out the datasheet's worst case
     * and it detects a sensor that dies mid-conversion. The timeout is the
     * backstop, not the plan.
     */
    uint16_t waited = 0;
    while (hn_ow_read_bit() == 0) {
        if (waited >= HN_TEMP_CONVERT_TIMEOUT_MS) {
            r.status = HN_STATUS_FAULT;
            temp_power_down();
            return;
        }
        hn_delay_ms(1);
        waited++;
    }

    if (hn_ow_reset() != HN_OW_PRESENCE) {
        r.status = HN_STATUS_FAULT;
        temp_power_down();
        return;
    }

    uint8_t sp[9];
    hn_ow_write_byte(HN_OW_CMD_SKIP_ROM);
    hn_ow_write_byte(DS18B20_CMD_READ_SCRATCH);
    hn_ow_read_bytes(sp, 9);

    temp_power_down();

    r.crc_ok = (hn_crc8(sp, 8) == sp[8]);
    if (!r.crc_ok) {
        /* A CRC failure on a bus this short is noise, a marginal connector or a
         * sensor on its way out. It is a fault, not a reading. */
        r.status = HN_STATUS_FAULT;
        return;
    }

    /* An all-ones scratchpad is what an open bus reads as; an all-zero one is
     * what a bus stuck low reads as. Either can pass CRC by coincidence
     * (0xFF... does not, but the check is cheap and the failure is ugly). */
    bool all_ff = true, all_00 = true;
    for (uint8_t i = 0; i < 9; ++i) {
        if (sp[i] != 0xFF) all_ff = false;
        if (sp[i] != 0x00) all_00 = false;
    }
    if (all_ff || all_00) {
        r.status = HN_STATUS_FAULT;
        return;
    }

    /* The sensor echoes back the config byte we wrote. If it does not match,
     * the write did not land and the conversion we just read was taken at an
     * unknown resolution. */
    if (sp[4] != HN_TEMP_CONFIG_BYTE) {
        r.status = HN_STATUS_FAULT;
        return;
    }

    r.raw = (int16_t)(((uint16_t)sp[1] << 8) | sp[0]);

    /* 0x0550 is the DS18B20's power-on default (+85.0 C). Seeing it means the
     * conversion never completed - the classic signature of a sensor that lost
     * power part-way through. It is not an 85 degree tank. */
    if (r.raw == (int16_t)HN_TEMP_POR_RAW) {
        r.status = HN_STATUS_FAULT;
        return;
    }

    int16_t whole_c = (int16_t)(r.raw >> 4);
    if (whole_c < HN_TEMP_MIN_C || whole_c > HN_TEMP_MAX_C) {
        /* Still a real measurement, and it is still transmitted raw. Only
         * flagged, because the Hub is entitled to decide what an implausible
         * reading means. */
        r.status = HN_STATUS_OUT_OF_RANGE;
        return;
    }

    r.status = HN_STATUS_OK;
}

void hn_temperature_read(hn_temperature_reading_t &r)
{
    if (hn_temperature_start(r)) {
        hn_delay_ms(HN_TEMP_CONVERT_MS);
    }
    hn_temperature_finish(r);
}

float hn_temperature_celsius(int16_t raw)
{
    return (float)raw / 16.0f;
}
