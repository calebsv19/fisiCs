#include <stdio.h>
#include <string.h>

#ifndef __FISICS__
static double fisics_convert_unit(double value, const char* target_unit) {
    if (strcmp(target_unit, "meter") == 0) return value * 1609.344;
    if (strcmp(target_unit, "joule") == 0) return value * 3600.0;
    if (strcmp(target_unit, "kilovolt") == 0) return value / 1000000.0;
    return value;
}
#endif

static double normalize_leg(
    double leg_miles [[fisics::dim(length)]] [[fisics::unit(mile)]]) {
    double leg_meters
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = fisics_convert_unit(leg_miles, "meter");
    return leg_meters;
}

static double normalize_stage_energy(
    double stage_wh [[fisics::dim(energy)]] [[fisics::unit(watt_hour)]]) {
    double stage_j
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = fisics_convert_unit(stage_wh, "joule");
    return stage_j;
}

static double normalize_cell_drop(
    double cell_mv [[fisics::dim(voltage)]] [[fisics::unit(millivolt)]]) {
    double cell_kv
        [[fisics::dim(voltage)]]
        [[fisics::unit(kilovolt)]] = fisics_convert_unit(cell_mv, "kilovolt");
    return cell_kv;
}

int main(void) {
    const double leg_miles_raw[] = {0.125, 0.25, 0.0625};
    const double stage_wh_raw[] = {48.0, 24.5, 15.25};
    const double cell_mv_raw[] = {3490.0, 3510.0, 3475.0};

    double route_meters
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = 0.0;
    double total_energy
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = 0.0;
    double pack_kv
        [[fisics::dim(voltage)]]
        [[fisics::unit(kilovolt)]] = 0.0;

    for (int i = 0; i < 3; ++i) {
        double leg_miles
            [[fisics::dim(length)]]
            [[fisics::unit(mile)]] = leg_miles_raw[i];
        double leg_meters
            [[fisics::dim(length)]]
            [[fisics::unit(meter)]] = normalize_leg(leg_miles);
        route_meters = route_meters + leg_meters;
    }

    for (int i = 0; i < 3; ++i) {
        double stage_wh
            [[fisics::dim(energy)]]
            [[fisics::unit(watt_hour)]] = stage_wh_raw[i];
        double stage_j
            [[fisics::dim(energy)]]
            [[fisics::unit(joule)]] = normalize_stage_energy(stage_wh);
        total_energy = total_energy + stage_j;
    }

    for (int i = 0; i < 3; ++i) {
        double cell_mv
            [[fisics::dim(voltage)]]
            [[fisics::unit(millivolt)]] = cell_mv_raw[i];
        double cell_kv
            [[fisics::dim(voltage)]]
            [[fisics::unit(kilovolt)]] = normalize_cell_drop(cell_mv);
        pack_kv = pack_kv + cell_kv;
    }

    printf("%.6f %.6f %.6f\n", route_meters, total_energy, pack_kv);
    return 0;
}
