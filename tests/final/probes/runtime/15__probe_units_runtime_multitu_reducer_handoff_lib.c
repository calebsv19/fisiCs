#include "15__probe_units_runtime_multitu_reducer_handoff_shared.h"

ProbeReducerState probe_reducer_seed(void) {
    ProbeReducerState state = {0};
    return state;
}

ProbeReducerState probe_reducer_push_motion(
    ProbeReducerState state,
    double speed_fps [[fisics::dim(speed)]] [[fisics::unit(foot_per_second)]],
    double dt_ms [[fisics::dim(time)]] [[fisics::unit(millisecond)]],
    double thrust_lbf [[fisics::dim(force)]] [[fisics::unit(pound_force)]]) {
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
    double thrust_n
        [[fisics::dim(force)]]
        [[fisics::unit(newton)]] = fisics_convert_unit(thrust_lbf, "newton");
    double energy_j
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = thrust_n * distance_m;

    state.total_distance_m += distance_m;
    state.total_energy_j += energy_j;
    return state;
}

ProbeReducerState probe_reducer_push_reserve(
    ProbeReducerState state,
    double reserve_mah [[fisics::dim(charge)]] [[fisics::unit(milliampere_hour)]],
    double reserve_wh [[fisics::dim(energy)]] [[fisics::unit(watt_hour)]]) {
    double charge_ah
        [[fisics::dim(charge)]]
        [[fisics::unit(ampere_hour)]] =
            fisics_convert_unit(reserve_mah, "ampere_hour");
    double reserve_j
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = fisics_convert_unit(reserve_wh, "joule");

    state.total_charge_ah += charge_ah;
    state.total_energy_j += reserve_j;
    return state;
}

double probe_reducer_score(
    ProbeReducerState state,
    double window_ft [[fisics::dim(length)]] [[fisics::unit(foot)]]) {
    double window_m
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = fisics_convert_unit(window_ft, "meter");
    return state.total_distance_m + state.total_energy_j + state.total_charge_ah + window_m;
}
