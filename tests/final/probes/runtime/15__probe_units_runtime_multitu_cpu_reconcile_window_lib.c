#include "15__probe_units_runtime_multitu_cpu_reconcile_window_shared.h"

ProbeCpuReconcileState probe_cpu_reconcile_seed(
    double speed_fps [[fisics::dim(speed)]] [[fisics::unit(foot_per_second)]],
    double dt_ms [[fisics::dim(time)]] [[fisics::unit(millisecond)]]) {
    double speed_mps
        [[fisics::dim(speed)]]
        [[fisics::unit(meter_per_second)]] =
            fisics_convert_unit(speed_fps, "meter_per_second");
    double dt_s
        [[fisics::dim(time)]]
        [[fisics::unit(second)]] = fisics_convert_unit(dt_ms, "second");
    ProbeCpuReconcileState state = {
        speed_mps * dt_s,
        0.0,
        0.0,
    };
    return state;
}

ProbeCpuReconcileState probe_cpu_reconcile_push_force(
    ProbeCpuReconcileState state,
    double thrust_lbf [[fisics::dim(force)]] [[fisics::unit(pound_force)]]) {
    double thrust_n
        [[fisics::dim(force)]]
        [[fisics::unit(newton)]] = fisics_convert_unit(thrust_lbf, "newton");
    state.energy_j += thrust_n * state.distance_m;
    return state;
}

ProbeCpuReconcileState probe_cpu_reconcile_merge(
    ProbeCpuReconcileState left,
    ProbeCpuReconcileState right,
    double reserve_wh [[fisics::dim(energy)]] [[fisics::unit(watt_hour)]],
    double reserve_mah [[fisics::dim(charge)]] [[fisics::unit(milliampere_hour)]]) {
    double reserve_j
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = fisics_convert_unit(reserve_wh, "joule");
    double reserve_ah
        [[fisics::dim(charge)]]
        [[fisics::unit(ampere_hour)]] =
            fisics_convert_unit(reserve_mah, "ampere_hour");
    ProbeCpuReconcileState merged = {
        left.distance_m + right.distance_m,
        left.energy_j + right.energy_j + reserve_j,
        left.charge_ah + right.charge_ah + reserve_ah,
    };
    return merged;
}

double probe_cpu_reconcile_score(
    ProbeCpuReconcileState state,
    double window_ms [[fisics::dim(time)]] [[fisics::unit(millisecond)]],
    double window_ft [[fisics::dim(length)]] [[fisics::unit(foot)]]) {
    double window_s
        [[fisics::dim(time)]]
        [[fisics::unit(second)]] = fisics_convert_unit(window_ms, "second");
    double window_m
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = fisics_convert_unit(window_ft, "meter");
    return (state.energy_j / window_s) + (state.charge_ah / window_m);
}
