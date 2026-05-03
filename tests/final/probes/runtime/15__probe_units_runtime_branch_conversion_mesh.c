#include <stdio.h>
#include <string.h>

#ifndef __FISICS__
static double fisics_convert_unit(double value, const char* target_unit) {
    if (strcmp(target_unit, "newton") == 0) return value * 4.4482216152605;
    if (strcmp(target_unit, "meter") == 0) return value * 0.3048;
    if (strcmp(target_unit, "ampere_hour") == 0) return value / 1000.0;
    return value;
}
#endif

static double select_force_newton(
    int metric_lane,
    double thrust_n [[fisics::dim(force)]] [[fisics::unit(newton)]],
    double thrust_lbf [[fisics::dim(force)]] [[fisics::unit(pound_force)]]) {
    if (metric_lane) return thrust_n;

    double converted
        [[fisics::dim(force)]]
        [[fisics::unit(newton)]] = fisics_convert_unit(thrust_lbf, "newton");
    return converted;
}

static double select_distance_meter(
    int metric_lane,
    double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]],
    double distance_ft [[fisics::dim(length)]] [[fisics::unit(foot)]]) {
    if (metric_lane) return distance_m;

    double converted
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = fisics_convert_unit(distance_ft, "meter");
    return converted;
}

static double select_charge_ah(
    int direct_lane,
    double reserve_ah [[fisics::dim(charge)]] [[fisics::unit(ampere_hour)]],
    double reserve_mah [[fisics::dim(charge)]] [[fisics::unit(milliampere_hour)]]) {
    if (direct_lane) return reserve_ah;

    double converted
        [[fisics::dim(charge)]]
        [[fisics::unit(ampere_hour)]] = fisics_convert_unit(reserve_mah, "ampere_hour");
    return converted;
}

int main(void) {
    double total_force
        [[fisics::dim(force)]]
        [[fisics::unit(newton)]] = 0.0;
    double total_distance
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = 0.0;
    double total_charge
        [[fisics::dim(charge)]]
        [[fisics::unit(ampere_hour)]] = 0.0;

    total_force = total_force + select_force_newton(1, 25.0, 0.0);
    total_force = total_force + select_force_newton(0, 0.0, 8.0);
    total_force = total_force + select_force_newton(0, 0.0, 6.5);

    total_distance = total_distance + select_distance_meter(1, 120.0, 0.0);
    total_distance = total_distance + select_distance_meter(0, 0.0, 42.0);
    total_distance = total_distance + select_distance_meter(0, 0.0, 18.5);

    total_charge = total_charge + select_charge_ah(1, 3.25, 0.0);
    total_charge = total_charge + select_charge_ah(0, 0.0, 1800.0);
    total_charge = total_charge + select_charge_ah(0, 0.0, 950.0);

    printf("%.6f %.6f %.6f\n", total_force, total_distance, total_charge);
    return 0;
}
