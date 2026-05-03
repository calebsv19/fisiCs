#include "15__probe_units_multitu_shared.h"

typedef double (*ProbeDistanceLane)(
    double speed_fps [[fisics::dim(speed)]] [[fisics::unit(foot_per_second)]],
    double dt_ms [[fisics::dim(time)]] [[fisics::unit(millisecond)]]);

static double probe_distance_lane_short(
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

static double probe_distance_lane_long(
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
        [[fisics::unit(meter)]] = (speed_mps * dt_s) * 2.0;
    return distance_m;
}

ProbeDistanceLane probe_select_distance_lane(int lane_id) {
    return lane_id ? probe_distance_lane_long : probe_distance_lane_short;
}

double probe_dispatch_energy(
    double thrust_lbf [[fisics::dim(force)]] [[fisics::unit(pound_force)]],
    double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]]) {
    double thrust_n
        [[fisics::dim(force)]]
        [[fisics::unit(newton)]] = fisics_convert_unit(thrust_lbf, "newton");
    double energy_j
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = thrust_n * distance_m;
    return energy_j;
}
