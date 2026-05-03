#include <stdio.h>
#include <string.h>

#ifndef __FISICS__
static double fisics_convert_unit(double value, const char* target_unit) {
    if (strcmp(target_unit, "meter_per_second") == 0) return value * 0.3048;
    if (strcmp(target_unit, "newton") == 0) return value * 4.4482216152605;
    if (strcmp(target_unit, "joule") == 0) return value * 3600.0;
    if (strcmp(target_unit, "ampere_hour") == 0) return value / 1000.0;
    if (strcmp(target_unit, "bar") == 0) return value * 0.06894757293168361;
    return value;
}
#endif

static double normalize_speed(
    double launch_speed_fps [[fisics::dim(speed)]] [[fisics::unit(foot_per_second)]]) {
    double launch_speed_mps
        [[fisics::dim(speed)]]
        [[fisics::unit(meter_per_second)]] =
            fisics_convert_unit(launch_speed_fps, "meter_per_second");
    return launch_speed_mps;
}

static double normalize_force(
    double thrust_lbf [[fisics::dim(force)]] [[fisics::unit(pound_force)]]) {
    double thrust_n
        [[fisics::dim(force)]]
        [[fisics::unit(newton)]] = fisics_convert_unit(thrust_lbf, "newton");
    return thrust_n;
}

static double normalize_reserve_energy(
    double reserve_wh [[fisics::dim(energy)]] [[fisics::unit(watt_hour)]]) {
    double reserve_j
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = fisics_convert_unit(reserve_wh, "joule");
    return reserve_j;
}

static double normalize_charge(
    double battery_mah [[fisics::dim(charge)]] [[fisics::unit(milliampere_hour)]]) {
    double battery_ah
        [[fisics::dim(charge)]]
        [[fisics::unit(ampere_hour)]] =
            fisics_convert_unit(battery_mah, "ampere_hour");
    return battery_ah;
}

static double normalize_pressure(
    double chamber_psi [[fisics::dim(pressure)]] [[fisics::unit(psi)]]) {
    double chamber_bar
        [[fisics::dim(pressure)]]
        [[fisics::unit(bar)]] = fisics_convert_unit(chamber_psi, "bar");
    return chamber_bar;
}

int main(void) {
    double launch_speed_fps
        [[fisics::dim(speed)]]
        [[fisics::unit(foot_per_second)]] = 48.0;
    double burn_time
        [[fisics::dim(time)]]
        [[fisics::unit(second)]] = 6.0;
    double thrust_lbf
        [[fisics::dim(force)]]
        [[fisics::unit(pound_force)]] = 18.0;
    double reserve_wh
        [[fisics::dim(energy)]]
        [[fisics::unit(watt_hour)]] = 140.0;
    double battery_mah
        [[fisics::dim(charge)]]
        [[fisics::unit(milliampere_hour)]] = 5200.0;
    double chamber_psi
        [[fisics::dim(pressure)]]
        [[fisics::unit(psi)]] = 145.0;

    double launch_speed_mps
        [[fisics::dim(speed)]]
        [[fisics::unit(meter_per_second)]] = normalize_speed(launch_speed_fps);
    double thrust_n
        [[fisics::dim(force)]]
        [[fisics::unit(newton)]] = normalize_force(thrust_lbf);
    double reserve_j
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = normalize_reserve_energy(reserve_wh);
    double battery_ah
        [[fisics::dim(charge)]]
        [[fisics::unit(ampere_hour)]] = normalize_charge(battery_mah);
    double chamber_bar
        [[fisics::dim(pressure)]]
        [[fisics::unit(bar)]] = normalize_pressure(chamber_psi);

    double travel_distance
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = launch_speed_mps * burn_time;
    double thrust_work
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = thrust_n * travel_distance;
    double total_energy
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = reserve_j + thrust_work;
    double average_power
        [[fisics::dim(power)]]
        [[fisics::unit(watt)]] = total_energy / burn_time;
    double chamber_ratio
        [[fisics::dim(scalar)]] = chamber_bar / 1.0;

    printf("%.6f %.6f %.6f %.6f %.6f %.6f\n",
           launch_speed_mps,
           travel_distance,
           thrust_work,
           average_power,
           battery_ah,
           chamber_ratio);
    return 0;
}
