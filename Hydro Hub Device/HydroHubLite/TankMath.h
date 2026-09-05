/*
 * TankMath.h - every conversion the Node deliberately does not do.
 *
 * The Node ships raw echo microseconds and a raw DS18B20 register. Everything
 * that turns those into a water level lives here, on the device that can be
 * reflashed without climbing onto a roof: the speed of sound, the split-
 * transducer parallax correction, the tank geometry and the volume.
 *
 * Deliberately free of Arduino and TFT dependencies so it can be compiled and
 * tested on a workstation - see tests/. Geometry bugs are silent: they produce
 * a plausible number that is simply wrong, and no amount of staring at a
 * dashboard reveals them.
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t  tank_count;
    uint32_t total_liters;      /* summed across all tanks           */
    uint16_t water_height_cm;   /* water depth at 100%               */
    uint16_t blind_cm;          /* transducer face -> full water line */
    uint16_t transducer_sep_mm; /* 0 disables the parallax correction */
} tank_config_t;

typedef struct {
    bool     valid;             /* false when there was no usable echo */
    float    distance_cm;       /* transducer -> water surface         */
    float    water_height_cm;   /* depth of water, 0 .. water_height_cm */
    float    level_pct;         /* 0 .. 100, clamped                   */
    uint32_t volume_liters;
    bool     clamped_full;      /* reading implied more than 100%      */
    bool     clamped_empty;     /* reading implied less than 0%        */
} tank_result_t;

/* Speed of sound in air, m/s. The linear form is accurate to well under 0.1%
 * across any temperature a water tank sees. */
float tank_speed_of_sound(float temp_c);

/* Raw round-trip microseconds -> perpendicular distance in cm, including the
 * parallax correction when transducer_sep_mm is non-zero. */
float tank_distance_cm(uint16_t echo_us, float temp_c, uint16_t transducer_sep_mm);

/* Full conversion. Returns false and leaves out->valid false when echo_us is
 * HN_ECHO_NONE (no usable reading) or the configuration is unusable. */
bool tank_compute(const tank_config_t *cfg, uint16_t echo_us, float temp_c,
                  tank_result_t *out);

/* DS18B20 register -> Celsius. */
float tank_temp_c(int16_t raw);

#ifdef __cplusplus
}
#endif
