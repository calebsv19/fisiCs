#include <stdio.h>
#include <string.h>

#ifndef __FISICS__
static double fisics_convert_unit(double value, const char* target_unit) {
    if (strcmp(target_unit, "meter") == 0) return value * 1609.344;
    if (strcmp(target_unit, "joule") == 0) return value * 3600.0;
    if (strcmp(target_unit, "kilovolt") == 0) return value / 1000000.0;
    if (strcmp(target_unit, "ampere_hour") == 0) return value / 1000.0;
    if (strcmp(target_unit, "bar") == 0) return value * 0.06894757293168361;
    return value;
}
#endif

static double normalize_leg_distance(
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

static double normalize_charge_draw(
    double draw_mah [[fisics::dim(charge)]] [[fisics::unit(milliampere_hour)]]) {
    double draw_ah
        [[fisics::dim(charge)]]
        [[fisics::unit(ampere_hour)]] = fisics_convert_unit(draw_mah, "ampere_hour");
    return draw_ah;
}

static double normalize_pressure_drop(
    double drop_psi [[fisics::dim(pressure)]] [[fisics::unit(psi)]]) {
    double drop_bar
        [[fisics::dim(pressure)]]
        [[fisics::unit(bar)]] = fisics_convert_unit(drop_psi, "bar");
    return drop_bar;
}

int main(void) {
    double leg_a_miles
        [[fisics::dim(length)]]
        [[fisics::unit(mile)]] = 0.25;
    double leg_b_miles
        [[fisics::dim(length)]]
        [[fisics::unit(mile)]] = 0.125;
    double leg_c_meters
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = 320.0;

    double stage_a_wh
        [[fisics::dim(energy)]]
        [[fisics::unit(watt_hour)]] = 80.0;
    double stage_b_wh
        [[fisics::dim(energy)]]
        [[fisics::unit(watt_hour)]] = 45.0;
    double stage_c_j
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = 1250.0;

    double cell_a_mv
        [[fisics::dim(voltage)]]
        [[fisics::unit(millivolt)]] = 3500.0;
    double cell_b_mv
        [[fisics::dim(voltage)]]
        [[fisics::unit(millivolt)]] = 3475.0;
    double cell_c_mv
        [[fisics::dim(voltage)]]
        [[fisics::unit(millivolt)]] = 3490.0;

    double draw_a_mah
        [[fisics::dim(charge)]]
        [[fisics::unit(milliampere_hour)]] = 1800.0;
    double draw_b_mah
        [[fisics::dim(charge)]]
        [[fisics::unit(milliampere_hour)]] = 900.0;

    double drop_a_psi
        [[fisics::dim(pressure)]]
        [[fisics::unit(psi)]] = 22.0;
    double drop_b_psi
        [[fisics::dim(pressure)]]
        [[fisics::unit(psi)]] = 18.5;

    double leg_a_meters
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = normalize_leg_distance(leg_a_miles);
    double leg_b_meters
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = normalize_leg_distance(leg_b_miles);
    double route_meters
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = leg_a_meters + leg_b_meters + leg_c_meters;

    double stage_a_j
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = normalize_stage_energy(stage_a_wh);
    double stage_b_j
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = normalize_stage_energy(stage_b_wh);
    double total_energy
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = stage_a_j + stage_b_j + stage_c_j;

    double cell_a_kv
        [[fisics::dim(voltage)]]
        [[fisics::unit(kilovolt)]] = normalize_cell_drop(cell_a_mv);
    double cell_b_kv
        [[fisics::dim(voltage)]]
        [[fisics::unit(kilovolt)]] = normalize_cell_drop(cell_b_mv);
    double cell_c_kv
        [[fisics::dim(voltage)]]
        [[fisics::unit(kilovolt)]] = normalize_cell_drop(cell_c_mv);
    double pack_kv
        [[fisics::dim(voltage)]]
        [[fisics::unit(kilovolt)]] = cell_a_kv + cell_b_kv + cell_c_kv;

    double draw_a_ah
        [[fisics::dim(charge)]]
        [[fisics::unit(ampere_hour)]] = normalize_charge_draw(draw_a_mah);
    double draw_b_ah
        [[fisics::dim(charge)]]
        [[fisics::unit(ampere_hour)]] = normalize_charge_draw(draw_b_mah);
    double total_draw_ah
        [[fisics::dim(charge)]]
        [[fisics::unit(ampere_hour)]] = draw_a_ah + draw_b_ah;

    double drop_a_bar
        [[fisics::dim(pressure)]]
        [[fisics::unit(bar)]] = normalize_pressure_drop(drop_a_psi);
    double drop_b_bar
        [[fisics::dim(pressure)]]
        [[fisics::unit(bar)]] = normalize_pressure_drop(drop_b_psi);
    double total_drop_bar
        [[fisics::dim(pressure)]]
        [[fisics::unit(bar)]] = drop_a_bar + drop_b_bar;

    printf("%.6f %.6f %.6f %.6f %.6f\n",
           route_meters,
           total_energy,
           pack_kv,
           total_draw_ah,
           total_drop_bar);
    return 0;
}
