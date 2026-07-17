#include "15__probe_units_runtime_wave129_nested_payload_callback.h"
#include <string.h>

#ifndef __FISICS__
static double fisics_convert_unit(double value, const char* target_unit) {
    if (strcmp(target_unit, "meter") == 0) return value * 0.3048;
    if (strcmp(target_unit, "second") == 0) return value / 1000.0;
    if (strcmp(target_unit, "joule") == 0) return value * 3600.0;
    if (strcmp(target_unit, "ampere_hour") == 0) return value / 1000.0;
    return value;
}
#endif

struct Wave129UnitsPayload wave129_units_seed(double feet, double reserve_wh, double draw_mah) {
    double raw_distance
        [[fisics::dim(length)]]
        [[fisics::unit(foot)]] = feet;
    double distance_m
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = fisics_convert_unit(raw_distance, "meter");

    double reserve
        [[fisics::dim(energy)]]
        [[fisics::unit(watt_hour)]] = reserve_wh;
    double reserve_j
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = fisics_convert_unit(reserve, "joule");

    double draw
        [[fisics::dim(charge)]]
        [[fisics::unit(milliampere_hour)]] = draw_mah;
    double charge_ah
        [[fisics::dim(charge)]]
        [[fisics::unit(ampere_hour)]] = fisics_convert_unit(draw, "ampere_hour");

    struct Wave129UnitsPayload payload = {{distance_m, distance_m * 0.5}, reserve_j, charge_ah};
    return payload;
}

struct Wave129UnitsPayload wave129_units_apply(struct Wave129UnitsPayload payload,
                                               Wave129UnitsCallback callback,
                                               double tick_ms) {
    double tick
        [[fisics::dim(time)]]
        [[fisics::unit(millisecond)]] = tick_ms;
    double tick_s
        [[fisics::dim(time)]]
        [[fisics::unit(second)]] = fisics_convert_unit(tick, "second");
    double raw_adjust
        [[fisics::dim(length)]]
        [[fisics::unit(foot)]] = tick_ms / 100.0;
    double adjust_m
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = fisics_convert_unit(raw_adjust, "meter");

    payload.distance_m[0] += adjust_m;
    payload.distance_m[1] += adjust_m * 0.5;
    payload.reserve_j += payload.reserve_j * (tick_ms / 10000.0);
    return callback(payload, tick_s);
}

double wave129_units_score(struct Wave129UnitsPayload payload, int lane) {
    return payload.distance_m[0] * (double)(lane + 4) +
           payload.distance_m[1] * (double)(lane + 7) +
           payload.reserve_j / (double)(lane + 11) +
           payload.charge_ah * (double)(lane + 13);
}
