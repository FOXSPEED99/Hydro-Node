#include "hn_report.h"
#include "hn_board.h"
#include "hn_config.h"
#include "hn_packet.h"
#include "hn_temperature.h"
#include "hn_lora.h"

#include <Arduino.h>

#if HN_SERIAL_ENABLED

/*
 * Diagnostic distance, for the human block only.
 *
 * This deliberately does NOT go into hn_reading_t and is never transmitted.
 * The Hub owns the conversion, because three of the corrections it needs are
 * things the Node cannot know or should not have frozen into it:
 *
 *   - the split-transducer parallax term, d = sqrt((L/2)^2 - (s/2)^2), which is
 *     a per-module build constant measured with callipers;
 *   - the saturated-air humidity correction, worth +0.35 to 0.6 %;
 *   - the tank geometry and volume curve.
 *
 * What is here is the plain first-order relation, so a technician can sanity
 * check a reading against a tape measure. It will read a few millimetres long
 * at close range, and that is expected.
 */
static float diagnostic_mm(uint16_t echo_us, float temp_c)
{
    /* Speed of sound in air: c = 331.3 + 0.606*T  [m/s], T in Celsius. */
    float c = 331.3f + 0.606f * temp_c;
    return ((float)echo_us * c) / 2000.0f;
}

void hn_report_begin()
{
    Serial.begin(HN_SERIAL_BAUD);
    while (!Serial) { /* no-op on the ATmega328P; harmless if ported */ }
}

void hn_report_banner()
{
    Serial.println();
    Serial.println(F("=================================================================="));
    Serial.print(F(HN_FW_NAME " " HN_FW_VERSION "  -  " HN_FW_SECTION));
    Serial.println();
    Serial.println(F("Arduino Pro Mini 3.3V / 8MHz (ATmega328P), battery-fed VCC"));
    Serial.println(F("------------------------------------------------------------------"));
    Serial.println(F("Pin map (configured firmware):"));
    Serial.print(F("  D")); Serial.print((int)HN_PIN_US_TRIG);
    Serial.println(F("  ultrasonic TRIG"));
    Serial.print(F("  D")); Serial.print((int)HN_PIN_US_ECHO);
    Serial.print(F("  ultrasonic ECHO"));
#if HN_US_ECHO_HAS_INPUT_CAPTURE
    Serial.print(F("   (PB0/ICP1, input capture)"));
#else
    Serial.print(F("   (software pulse timing)"));
#endif
    Serial.println();
    Serial.println(F("  D3  DS18B20 power     J2.3, 4k7 pull-up referenced here"));
    Serial.println(F("  D4  DS18B20 data      J2.1 via R4 100R"));
    Serial.println(F("  D5  flow digital      J1.2 via R5 100R   (1M pull-up on board)"));
    Serial.println(F("  A2  flow analogue     J1.2 via R3 330R"));
    Serial.println(F("  D7  buzzer            LS1 via R9 100R    (idle low, Section 4)"));
    Serial.println(F("  A1  LATCH CLEAR       74HC74 1CLR via R11 100k - INPUT ONLY"));
    Serial.println(F("------------------------------------------------------------------"));
    Serial.print(F("Cycle "));
    Serial.print((unsigned long)HN_CYCLE_INTERVAL_MS);
    Serial.print(F(" ms   ultrasonic "));
    Serial.print((int)HN_US_SAMPLES);
    Serial.print(F(" samples   DS18B20 "));
    Serial.print((int)HN_TEMP_RESOLUTION_BITS);
    Serial.println(F("-bit"));
#if HN_LORA_ENABLED
    Serial.print(F("LoRa  "));
    Serial.print((unsigned long)(HN_LORA_FREQ_HZ / 1000000UL));
    Serial.print(F(" MHz  SF"));
    Serial.print((int)HN_LORA_SPREADING_FACTOR);
    Serial.print(F("  BW"));
    Serial.print((int)HN_LORA_BW_KHZ);
    Serial.print(F("k  CR4/"));
    Serial.print((int)HN_LORA_CODING_RATE);
    Serial.print(F("  sync 0x"));
    Serial.print((int)HN_LORA_SYNC_WORD, HEX);
    Serial.print(F("  +"));
    Serial.print((int)HN_LORA_TX_POWER_DBM);
    Serial.println(F(" dBm"));
    Serial.print(F("      pair "));
    Serial.print(F(HN_PAIR_ID));
    Serial.print(F("  node "));
    Serial.print((int)HN_NODE_ID);
    Serial.print(F("  packet "));
    Serial.print((int)HN_PACKET_BYTES);
    Serial.println(F(" bytes"));
    Serial.println(F("      !! antenna must be connected before transmitting !!"));
#elif HN_PARK_LORA_PINS
    Serial.println(F("LoRa pins parked (Ra-02 held in reset) - Section 2 takes these over"));
#endif
    Serial.println(F("Raw values only: the Hub owns every conversion."));
    Serial.println(F("=================================================================="));
}

static void print_rom(const uint8_t *rom)
{
    for (uint8_t i = 0; i < 8; ++i) {
        if (i) Serial.print('-');
        if (rom[i] < 0x10) Serial.print('0');
        Serial.print(rom[i], HEX);
    }
}

static void print_sensor_head(const __FlashStringHelper *label,
                              hn_status_t st, hn_presence_t pr)
{
    Serial.print(F("  "));
    Serial.print(label);
    Serial.print(F("  "));
    Serial.print(hn_status_name(st));
    Serial.print(F("  ["));
    Serial.print(hn_presence_name(pr));
    Serial.print(F("]"));
}

void hn_report_selftest(const hn_reading_t &r)
{
    Serial.println();
    Serial.println(F("--- start-up sensor check ---------------------------------------"));

    Serial.print(F("  Ultrasonic   "));
    Serial.print(hn_presence_name(r.ultrasonic.presence));
    Serial.println(F("   (echo-line probe only, not yet triggered)"));

    Serial.print(F("  Temperature  "));
    Serial.print(hn_presence_name(r.temperature.presence));
    if (r.temperature.rom_valid) {
        Serial.print(F("   ROM "));
        print_rom(r.temperature.rom);
    } else if (r.temperature.presence == HN_PRESENCE_CONFIRMED) {
        Serial.print(F("   ROM unreadable or wrong family - not a DS18B20?"));
    }
    Serial.println();

    Serial.print(F("  Flow switch  "));
    Serial.print(hn_presence_name(r.flow.presence));
    Serial.print(F("   A2="));
    Serial.print(r.flow.level_adc);
    Serial.println(F("/1023"));
    if (r.flow.presence == HN_PRESENCE_UNCONFIRMED) {
        Serial.println(F("    note: an open dry contact and an unplugged harness are"));
        Serial.println(F("    the same circuit - presence cannot be proven while open."));
    }
    Serial.println(F("-----------------------------------------------------------------"));
}

static void report_human(const hn_reading_t &r)
{
    Serial.println();
    Serial.print(F("--- cycle "));
    Serial.print(r.seq);
    Serial.print(F("  t="));
    Serial.print(r.uptime_ms);
    Serial.println(F(" ms ------------------------------------------"));

    /* --- ultrasonic --- */
    const hn_ultrasonic_reading_t &u = r.ultrasonic;
    print_sensor_head(F("Ultrasonic "), u.status, u.presence);
    if (u.samples_valid) {
        Serial.print(F("  echo "));
        Serial.print(u.echo_us);
        Serial.print(F(" us  spread "));
        Serial.print(u.spread_us);
        Serial.print(F(" us"));
    }
    Serial.print(F("  samples "));
    Serial.print(u.samples_taken);   Serial.print('/');
    Serial.print(u.samples_valid);   Serial.print('/');
    Serial.print(u.samples_accepted);
    Serial.println();

    if (u.echo_stuck_high) {
        Serial.println(F("      ! echo line was already high before triggering -"));
        Serial.println(F("        check the harness; TRIG and ECHO may be swapped."));
    }
    if (u.rise_without_fall) {
        Serial.println(F("      ! echo pulse started and never ended."));
    }
    if (u.status == HN_STATUS_NO_TARGET) {
        Serial.println(F("      sensor answered, nothing came back: tank full and"));
        Serial.println(F("      inside the blind zone, or the surface is absorbing."));
    }
    if (u.samples_valid) {
        float t_c = hn_status_is_usable(r.temperature.status)
                        ? hn_temperature_celsius(r.temperature.raw)
                        : 20.0f;
        Serial.print(F("      ~ "));
        Serial.print(diagnostic_mm(u.echo_us, t_c), 1);
        Serial.print(F(" mm at "));
        Serial.print(t_c, 1);
        Serial.print(F(" C"));
        if (!hn_status_is_usable(r.temperature.status)) {
            Serial.print(F(" (ASSUMED - no temperature)"));
        }
        Serial.println(F("  [diagnostic only, uncorrected]"));
    }

    /* --- temperature --- */
    const hn_temperature_reading_t &t = r.temperature;
    print_sensor_head(F("Temperature"), t.status, t.presence);
    if (t.status != HN_STATUS_ABSENT && t.crc_ok) {
        Serial.print(F("  raw "));
        Serial.print(t.raw);
        Serial.print(F(" ("));
        Serial.print(hn_temperature_celsius(t.raw), 2);
        Serial.print(F(" C)  "));
        Serial.print(t.resolution_bits);
        Serial.print(F("-bit  CRC ok"));
    } else if (t.presence == HN_PRESENCE_CONFIRMED) {
        Serial.print(F("  no valid scratchpad"));
    }
    Serial.println();

    /* --- flow --- */
    const hn_flow_reading_t &f = r.flow;
    print_sensor_head(F("Flow switch"), f.status, f.presence);
    Serial.print(F("  "));
    switch (f.state) {
    case HN_FLOW_FILLING: Serial.print(F("FILLING")); break;
    case HN_FLOW_IDLE:    Serial.print(F("idle"));    break;
    default:              Serial.print(F("unknown")); break;
    }
    Serial.print(F("  D5="));
    Serial.print(f.level_digital ? '1' : '0');
    Serial.print(F("  A2="));
    Serial.print(f.level_adc);
    Serial.print(F("/1023  agree "));
    Serial.print(f.agree);
    Serial.print('/');
    Serial.print(f.samples);
    Serial.println();
    if (f.status == HN_STATUS_FAULT) {
        Serial.println(F("      ! node is not at either rail, or D5 and A2 disagree:"));
        Serial.println(F("        water in the contacts, a corroded pin, or a damaged cable."));
    }

    if (r.level_gated_by_flow) {
        Serial.println(F("  NOTE: water was running in - the level sample is not trustworthy."));
    }
}

#if HN_REPORT_MACHINE_LINE
static void print_pair(const __FlashStringHelper *k, long v)
{
    Serial.print(' ');
    Serial.print(k);
    Serial.print('=');
    Serial.print(v);
}

static void report_machine(const hn_reading_t &r)
{
    /* seq and uptime are printed unsigned: millis() passes 2^31 after 24.8
     * days, and a field device reporting a negative uptime is the kind of
     * detail that costs an afternoon to chase down. */
    Serial.print(F("#HN seq="));
    Serial.print((unsigned long)r.seq);
    Serial.print(F(" up="));
    Serial.print((unsigned long)r.uptime_ms);

    Serial.print(F(" us.st="));   Serial.print(hn_status_code(r.ultrasonic.status));
    Serial.print(F(" us.pr="));   Serial.print(hn_presence_code(r.ultrasonic.presence));
    print_pair(F("us.echo"), (long)r.ultrasonic.echo_us);
    print_pair(F("us.spr"),  (long)r.ultrasonic.spread_us);
    Serial.print(F(" us.n="));
    Serial.print(r.ultrasonic.samples_taken);    Serial.print(',');
    Serial.print(r.ultrasonic.samples_valid);    Serial.print(',');
    Serial.print(r.ultrasonic.samples_accepted);

    Serial.print(F(" tp.st="));   Serial.print(hn_status_code(r.temperature.status));
    Serial.print(F(" tp.pr="));   Serial.print(hn_presence_code(r.temperature.presence));
    print_pair(F("tp.raw"), (long)r.temperature.raw);
    print_pair(F("tp.res"), (long)r.temperature.resolution_bits);
    print_pair(F("tp.crc"), r.temperature.crc_ok ? 1L : 0L);

    Serial.print(F(" fl.st="));   Serial.print(hn_status_code(r.flow.status));
    Serial.print(F(" fl.pr="));   Serial.print(hn_presence_code(r.flow.presence));
    Serial.print(F(" fl.state="));
    switch (r.flow.state) {
    case HN_FLOW_FILLING: Serial.print(F("FILL")); break;
    case HN_FLOW_IDLE:    Serial.print(F("IDLE")); break;
    default:              Serial.print(F("UNK"));  break;
    }
    print_pair(F("fl.d"),   r.flow.level_digital ? 1L : 0L);
    print_pair(F("fl.adc"), (long)r.flow.level_adc);
    print_pair(F("gate"),   r.level_gated_by_flow ? 1L : 0L);
    Serial.println();
}
#endif

void hn_report_boot(bool watchdogReset, uint16_t battery_mv)
{
    Serial.println();
    if (watchdogReset) {
        Serial.println(F("!! BOOTED FROM A WATCHDOG RESET - the last cycle hung."));
        Serial.println(F("   If this repeats, capture the cycle before it happens."));
    } else {
        Serial.println(F("Boot: power-on."));
    }
    Serial.print(F("Battery: "));
    if (battery_mv == 0) {
        Serial.println(F("not measured"));
    } else {
        Serial.print(battery_mv);
        Serial.print(F(" mV   (calibrate HN_BANDGAP_CAL against a meter -"));
        Serial.println(F(" the bandgap is only +/-10% untrimmed)"));
    }
#if HN_PRODUCTION
    Serial.println(F("PRODUCTION build: sleep on, 120 s cycle."));
#else
    Serial.println(F("BENCH build: no sleep, 5 s cycle. Set HN_PRODUCTION 1 to deploy."));
#endif
}

void hn_report_battery(uint16_t battery_mv)
{
    if (battery_mv == 0) return;
    Serial.print(F("  Battery  "));
    Serial.print(battery_mv);
    Serial.println(F(" mV"));
}

void hn_report_radio(bool present)
{
    Serial.println();
    if (present) {
        Serial.println(F("--- radio ------------------------------------------------------"));
        Serial.println(F("  Ra-02 SX1278 found and configured, sleeping until the first send."));
    } else {
        Serial.println(F("--- radio: NOT FOUND -------------------------------------------"));
        Serial.println(F("  The SX1278 did not identify itself over SPI. It is soldered to"));
        Serial.println(F("  this board, not on a connector, so this is a solder or supply"));
        Serial.println(F("  fault rather than a missing module. Sensors still run; nothing"));
        Serial.println(F("  will be transmitted."));
    }
    Serial.println(F("----------------------------------------------------------------"));
}

void hn_report_tx(bool sent, uint8_t bytes, uint16_t airtime_ms)
{
    Serial.print(F("  LoRa  "));
    if (sent) {
        Serial.print(F("sent "));
        Serial.print(bytes);
        Serial.print(F(" bytes in "));
        Serial.print(airtime_ms);
        Serial.print(F(" ms"));
        /* The airtime is the term the two-year battery estimate rests on, so
         * it is printed every cycle rather than assumed from the datasheet. */
        Serial.print(F("  (~"));
        Serial.print((unsigned long)airtime_ms / 10UL);
        Serial.println(F(" mA*s at 100 mA)"));
    } else if (!hn_lora_present()) {
        Serial.println(F("not sent - no radio"));
    } else {
        Serial.println(F("TIMED OUT - the radio stopped responding mid-transmission"));
    }
}

void hn_report_reading(const hn_reading_t &r)
{
    report_human(r);
#if HN_REPORT_MACHINE_LINE
    report_machine(r);
#endif
}

#else  /* !HN_SERIAL_ENABLED */

void hn_report_begin()                        {}
void hn_report_banner()                       {}
void hn_report_selftest(const hn_reading_t &)  {}
void hn_report_reading(const hn_reading_t &)   {}
void hn_report_radio(bool)                     {}
void hn_report_tx(bool, uint8_t, uint16_t)     {}
void hn_report_boot(bool, uint16_t)            {}
void hn_report_battery(uint16_t)               {}

#endif
