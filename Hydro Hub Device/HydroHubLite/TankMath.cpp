#include "TankMath.h"
#include "hn_packet.h"

#include <math.h>

float tank_speed_of_sound(float temp_c)
{
    /* c = 331.3 + 0.606*T m/s. Note this wants the temperature of the AIR the
     * pulse travels through - the tank headspace - not the water. If the
     * DS18B20 is immersed, this is using a proxy and the error goes straight
     * into the distance. */
    return 331.3f + 0.606f * temp_c;
}

float tank_distance_cm(uint16_t echo_us, float temp_c, uint16_t transducer_sep_mm)
{
    /* c m/s -> cm/us is c/10000. Half of the round trip is the path length to
     * the surface. */
    const float half_path_cm = ((float)echo_us * tank_speed_of_sound(temp_c)) / 20000.0f;

    if (transducer_sep_mm == 0) return half_path_cm;

    /*
     * The RCWL-1670 transmits and receives from two transducers set apart by
     * s, so the sound travels down one side of a triangle and back up the
     * other. The measured half-path is the hypotenuse; the perpendicular
     * distance is the other side:
     *
     *      d = sqrt( (L/2)^2 - (s/2)^2 )
     *
     * It is a systematic over-read that grows as the target gets closer -
     * about 4 mm at 50 mm with a 40 mm spacing, negligible past 300 mm. Since
     * "nearly full" is where this product is most often read, it is worth the
     * square root.
     */
    const float half_sep_cm = (float)transducer_sep_mm / 20.0f;
    const float sq = half_path_cm * half_path_cm - half_sep_cm * half_sep_cm;
    if (sq <= 0.0f) return 0.0f;   /* closer than the geometry allows */
    return sqrtf(sq);
}

float tank_temp_c(int16_t raw)
{
    return (float)raw / 16.0f;
}

bool tank_compute(const tank_config_t *cfg, uint16_t echo_us, float temp_c,
                  tank_result_t *out)
{
    out->valid = false;
    out->distance_cm = 0.0f;
    out->water_height_cm = 0.0f;
    out->level_pct = 0.0f;
    out->volume_liters = 0;
    out->clamped_full = false;
    out->clamped_empty = false;

    if (echo_us == HN_ECHO_NONE) return false;      /* the Node had no reading */
    if (cfg->water_height_cm == 0) return false;    /* unusable configuration  */

    const float d = tank_distance_cm(echo_us, temp_c, cfg->transducer_sep_mm);
    out->distance_cm = d;

    /* The sensor reads blind_cm when full and blind_cm + height when empty, so
     * the water depth is the height minus however far past "full" it has
     * fallen. */
    float h = (float)cfg->water_height_cm - (d - (float)cfg->blind_cm);

    /*
     * Clamp, but record that we did. An out-of-range reading is information:
     * consistently past 100% means the blind zone is set too large or the
     * sensor has been remounted; past 0% (empty) usually means it is seeing the
     * tank floor. Silently clamping hides a commissioning error that would
     * otherwise be obvious.
     */
    if (h > (float)cfg->water_height_cm) { h = (float)cfg->water_height_cm; out->clamped_full = true; }
    if (h < 0.0f)                        { h = 0.0f;                        out->clamped_empty = true; }

    out->water_height_cm = h;
    out->level_pct = 100.0f * h / (float)cfg->water_height_cm;

    /* The tanks are connected, so one level applies to all of them and the
     * volume is that fraction of the summed capacity. */
    out->volume_liters = (uint32_t)(((float)cfg->total_liters * out->level_pct / 100.0f) + 0.5f);

    out->valid = true;
    return true;
}
