#include "15__probe_units_runtime_multitu_dispatch_window_fold_shared.h"

static double probe_dispatch_window_lane_core(
    double speed_fps [[fisics::dim(speed)]] [[fisics::unit(foot_per_second)]],
    double dt_ms [[fisics::dim(time)]] [[fisics::unit(millisecond)]],
    double gain) {
    double speed_mps
        [[fisics::dim(speed)]]
        [[fisics::unit(meter_per_second)]] =
            fisics_convert_unit(speed_fps, "meter_per_second");
    double dt_s
        [[fisics::dim(time)]]
        [[fisics::unit(second)]] = fisics_convert_unit(dt_ms, "second");
    double distance_m
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = (speed_mps * dt_s) * gain;
    return distance_m;
}

static double probe_dispatch_window_lane_full(
    double speed_fps [[fisics::dim(speed)]] [[fisics::unit(foot_per_second)]],
    double dt_ms [[fisics::dim(time)]] [[fisics::unit(millisecond)]]) {
    return probe_dispatch_window_lane_core(speed_fps, dt_ms, 1.0);
}

static double probe_dispatch_window_lane_trim(
    double speed_fps [[fisics::dim(speed)]] [[fisics::unit(foot_per_second)]],
    double dt_ms [[fisics::dim(time)]] [[fisics::unit(millisecond)]]) {
    return probe_dispatch_window_lane_core(speed_fps, dt_ms, 0.92);
}

ProbeDispatchWindowLane probe_dispatch_window_pick(int lane_id) {
    return lane_id == 0 ? probe_dispatch_window_lane_full : probe_dispatch_window_lane_trim;
}

double probe_dispatch_window_fold_energy(
    double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]],
    double thrust_lbf [[fisics::dim(force)]] [[fisics::unit(pound_force)]],
    double reserve_wh [[fisics::dim(energy)]] [[fisics::unit(watt_hour)]]) {
    double thrust_n
        [[fisics::dim(force)]]
        [[fisics::unit(newton)]] = fisics_convert_unit(thrust_lbf, "newton");
    double reserve_j
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = fisics_convert_unit(reserve_wh, "joule");
    return (thrust_n * distance_m) + reserve_j;
}

double probe_dispatch_window_score(
    double total_energy_j [[fisics::dim(energy)]] [[fisics::unit(joule)]],
    double total_distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]],
    double window_ms [[fisics::dim(time)]] [[fisics::unit(millisecond)]]) {
    (void)total_distance_m;
    double window_s
        [[fisics::dim(time)]]
        [[fisics::unit(second)]] = fisics_convert_unit(window_ms, "second");
    return total_energy_j / window_s;
}
