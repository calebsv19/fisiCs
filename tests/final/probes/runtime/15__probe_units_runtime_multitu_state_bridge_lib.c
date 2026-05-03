#include "15__probe_units_multitu_shared.h"

double probe_state_bridge_distance(
    double speed_fps [[fisics::dim(speed)]] [[fisics::unit(foot_per_second)]],
    double dt_ms [[fisics::dim(time)]] [[fisics::unit(millisecond)]]) {
    double speed_mps
        [[fisics::dim(speed)]]
        [[fisics::unit(meter_per_second)]] =
            fisics_convert_unit(speed_fps, "meter_per_second");
    double dt_s
        [[fisics::dim(time)]]
        [[fisics::unit(second)]] = fisics_convert_unit(dt_ms, "second");
    double distance_m
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = speed_mps * dt_s;
    return distance_m;
}

double probe_state_bridge_energy(
    double thrust_lbf [[fisics::dim(force)]] [[fisics::unit(pound_force)]],
    double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]]) {
    double thrust_n
        [[fisics::dim(force)]]
        [[fisics::unit(newton)]] = fisics_convert_unit(thrust_lbf, "newton");
    double work_j
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = thrust_n * distance_m;
    return work_j;
}

double probe_state_bridge_charge(
    double reserve_mah [[fisics::dim(charge)]] [[fisics::unit(milliampere_hour)]]) {
    double reserve_ah
        [[fisics::dim(charge)]]
        [[fisics::unit(ampere_hour)]] =
            fisics_convert_unit(reserve_mah, "ampere_hour");
    return reserve_ah;
}
