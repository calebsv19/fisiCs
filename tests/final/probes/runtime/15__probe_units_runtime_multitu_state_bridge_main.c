#include <stdio.h>

#include "15__probe_units_multitu_shared.h"

double probe_state_bridge_distance(
    double speed_fps [[fisics::dim(speed)]] [[fisics::unit(foot_per_second)]],
    double dt_ms [[fisics::dim(time)]] [[fisics::unit(millisecond)]]);
double probe_state_bridge_energy(
    double thrust_lbf [[fisics::dim(force)]] [[fisics::unit(pound_force)]],
    double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]]);
double probe_state_bridge_charge(
    double reserve_mah [[fisics::dim(charge)]] [[fisics::unit(milliampere_hour)]]);

int main(void) {
    double speed_fps
        [[fisics::dim(speed)]]
        [[fisics::unit(foot_per_second)]] = 96.0;
    double dt_ms
        [[fisics::dim(time)]]
        [[fisics::unit(millisecond)]] = 1250.0;
    double thrust_lbf
        [[fisics::dim(force)]]
        [[fisics::unit(pound_force)]] = 12.5;
    double reserve_mah
        [[fisics::dim(charge)]]
        [[fisics::unit(milliampere_hour)]] = 3600.0;

    double burn_time_s
        [[fisics::dim(time)]]
        [[fisics::unit(second)]] = fisics_convert_unit(dt_ms, "second");
    double distance_m
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = probe_state_bridge_distance(speed_fps, dt_ms);
    double energy_j
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = probe_state_bridge_energy(thrust_lbf, distance_m);
    double charge_ah
        [[fisics::dim(charge)]]
        [[fisics::unit(ampere_hour)]] = probe_state_bridge_charge(reserve_mah);
    double average_power
        [[fisics::dim(power)]]
        [[fisics::unit(watt)]] = energy_j / burn_time_s;

    printf("%.6f %.6f %.6f %.6f\n",
           distance_m,
           energy_j,
           average_power,
           charge_ah);
    return 0;
}
