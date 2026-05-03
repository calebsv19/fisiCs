#include "15__probe_units_runtime_multitu_checkpoint_charge_bridge_shared.h"

ProbeCheckpointChargeState probe_checkpoint_charge_seed(
    double speed_fps [[fisics::dim(speed)]] [[fisics::unit(foot_per_second)]],
    double dt_ms [[fisics::dim(time)]] [[fisics::unit(millisecond)]]) {
    double speed_mps
        [[fisics::dim(speed)]]
        [[fisics::unit(meter_per_second)]] =
            fisics_convert_unit(speed_fps, "meter_per_second");
    double dt_s
        [[fisics::dim(time)]]
        [[fisics::unit(second)]] = fisics_convert_unit(dt_ms, "second");
    ProbeCheckpointChargeState state = {
        speed_mps * dt_s,
        0.0,
        0.0,
    };
    return state;
}

ProbeCheckpointChargeState probe_checkpoint_charge_push_force(
    ProbeCheckpointChargeState state,
    double thrust_lbf [[fisics::dim(force)]] [[fisics::unit(pound_force)]]) {
    double thrust_n
        [[fisics::dim(force)]]
        [[fisics::unit(newton)]] = fisics_convert_unit(thrust_lbf, "newton");
    state.energy_j += thrust_n * state.distance_m;
    return state;
}

ProbeCheckpointChargeState probe_checkpoint_charge_push_reserve(
    ProbeCheckpointChargeState state,
    double reserve_mah [[fisics::dim(charge)]] [[fisics::unit(milliampere_hour)]],
    double reserve_wh [[fisics::dim(energy)]] [[fisics::unit(watt_hour)]]) {
    double reserve_ah
        [[fisics::dim(charge)]]
        [[fisics::unit(ampere_hour)]] =
            fisics_convert_unit(reserve_mah, "ampere_hour");
    double reserve_j
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = fisics_convert_unit(reserve_wh, "joule");
    state.charge_ah += reserve_ah;
    state.energy_j += reserve_j;
    return state;
}

double probe_checkpoint_charge_score(
    ProbeCheckpointChargeState state,
    double window_ft [[fisics::dim(length)]] [[fisics::unit(foot)]]) {
    double window_m
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = fisics_convert_unit(window_ft, "meter");
    return (state.energy_j / window_m) + state.charge_ah;
}
