#include <stdio.h>
#include <string.h>

#ifndef __FISICS__
static double fisics_convert_unit(double value, const char* target_unit) {
    if (strcmp(target_unit, "meter_per_second") == 0) return value * 0.3048;
    if (strcmp(target_unit, "meter") == 0) return value * 1609.344;
    if (strcmp(target_unit, "newton") == 0) return value * 4.4482216152605;
    if (strcmp(target_unit, "ampere_hour") == 0) return value / 1000.0;
    if (strcmp(target_unit, "joule") == 0) return value * 3600000.0;
    return value;
}
#endif

int main(void) {
    double launch_speed
        [[fisics::dim(speed)]]
        [[fisics::unit(foot_per_second)]] = 10.0;
    double launch_speed_mps
        [[fisics::dim(speed)]]
        [[fisics::unit(meter_per_second)]] =
            fisics_convert_unit(launch_speed, "meter_per_second");

    double coast_time
        [[fisics::dim(time)]]
        [[fisics::unit(second)]] = 2.0;
    double launch_offset
        [[fisics::dim(length)]]
        [[fisics::unit(mile)]] = 0.5;
    double launch_offset_m
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] =
            fisics_convert_unit(launch_offset, "meter");
    double travel_step
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = launch_speed_mps * coast_time;
    double total_distance
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = launch_offset_m + travel_step;

    double thrust_lbf
        [[fisics::dim(force)]]
        [[fisics::unit(pound_force)]] = 10.0;
    double thrust_n
        [[fisics::dim(force)]]
        [[fisics::unit(newton)]] =
            fisics_convert_unit(thrust_lbf, "newton");

    double battery_mah
        [[fisics::dim(charge)]]
        [[fisics::unit(milliampere_hour)]] = 2500.0;
    double battery_ah
        [[fisics::dim(charge)]]
        [[fisics::unit(ampere_hour)]] =
            fisics_convert_unit(battery_mah, "ampere_hour");

    double reserve_energy
        [[fisics::dim(energy)]]
        [[fisics::unit(kilowatt_hour)]] = 0.25;
    double reserve_energy_j
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] =
            fisics_convert_unit(reserve_energy, "joule");

    printf("%.6f %.6f %.6f %.6f\n",
           total_distance,
           thrust_n,
           battery_ah,
           reserve_energy_j);
    return 0;
}
