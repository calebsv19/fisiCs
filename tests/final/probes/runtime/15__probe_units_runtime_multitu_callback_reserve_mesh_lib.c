#include "15__probe_units_runtime_multitu_callback_reserve_mesh_shared.h"

static ProbeCallbackReserveState probe_callback_step_core(
    ProbeCallbackReserveState state,
    double speed_fps [[fisics::dim(speed)]] [[fisics::unit(foot_per_second)]],
    double dt_ms [[fisics::dim(time)]] [[fisics::unit(millisecond)]],
    double thrust_lbf [[fisics::dim(force)]] [[fisics::unit(pound_force)]],
    double gain) {
    double speed_mps
        [[fisics::dim(speed)]]
        [[fisics::unit(meter_per_second)]] =
            fisics_convert_unit(speed_fps, "meter_per_second");
    double dt_s
        [[fisics::dim(time)]]
        [[fisics::unit(second)]] = fisics_convert_unit(dt_ms, "second");
    double thrust_n
        [[fisics::dim(force)]]
        [[fisics::unit(newton)]] = fisics_convert_unit(thrust_lbf, "newton");
    double leg_distance_m
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = (speed_mps * dt_s) * gain;
    double leg_energy_j
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = thrust_n * leg_distance_m;

    state.total_distance_m += leg_distance_m;
    state.total_energy_j += leg_energy_j;
    return state;
}

static ProbeCallbackReserveState probe_callback_step_fast(
    ProbeCallbackReserveState state,
    double speed_fps [[fisics::dim(speed)]] [[fisics::unit(foot_per_second)]],
    double dt_ms [[fisics::dim(time)]] [[fisics::unit(millisecond)]],
    double thrust_lbf [[fisics::dim(force)]] [[fisics::unit(pound_force)]]) {
    return probe_callback_step_core(state, speed_fps, dt_ms, thrust_lbf, 1.0);
}

static ProbeCallbackReserveState probe_callback_step_trimmed(
    ProbeCallbackReserveState state,
    double speed_fps [[fisics::dim(speed)]] [[fisics::unit(foot_per_second)]],
    double dt_ms [[fisics::dim(time)]] [[fisics::unit(millisecond)]],
    double thrust_lbf [[fisics::dim(force)]] [[fisics::unit(pound_force)]]) {
    return probe_callback_step_core(state, speed_fps, dt_ms, thrust_lbf, 0.85);
}

ProbeCallbackReserveState probe_callback_reserve_seed(void) {
    ProbeCallbackReserveState state = {0.0, 0.0, 0.0};
    return state;
}

ProbeCallbackReserveLane probe_callback_reserve_pick(int lane_id) {
    return lane_id == 0 ? probe_callback_step_fast : probe_callback_step_trimmed;
}

ProbeCallbackReserveState probe_callback_reserve_push_charge(
    ProbeCallbackReserveState state,
    double reserve_mah [[fisics::dim(charge)]] [[fisics::unit(milliampere_hour)]]) {
    double reserve_ah
        [[fisics::dim(charge)]]
        [[fisics::unit(ampere_hour)]] =
            fisics_convert_unit(reserve_mah, "ampere_hour");
    state.total_charge_ah += reserve_ah;
    return state;
}

double probe_callback_reserve_score(
    ProbeCallbackReserveState state,
    double window_ft [[fisics::dim(length)]] [[fisics::unit(foot)]]) {
    double window_m
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = fisics_convert_unit(window_ft, "meter");
    return (state.total_energy_j / window_m) + state.total_charge_ah;
}
