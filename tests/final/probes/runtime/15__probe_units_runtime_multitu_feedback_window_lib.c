#include "15__probe_units_runtime_multitu_feedback_window_shared.h"

ProbeFeedbackWindowState probe_feedback_window_seed(
    double speed_fps [[fisics::dim(speed)]] [[fisics::unit(foot_per_second)]],
    double dt_ms [[fisics::dim(time)]] [[fisics::unit(millisecond)]]) {
    double speed_mps
        [[fisics::dim(speed)]]
        [[fisics::unit(meter_per_second)]] =
            fisics_convert_unit(speed_fps, "meter_per_second");
    double dt_s
        [[fisics::dim(time)]]
        [[fisics::unit(second)]] = fisics_convert_unit(dt_ms, "second");
    ProbeFeedbackWindowState state = {
        speed_mps * dt_s,
        0.0,
    };
    return state;
}

ProbeFeedbackWindowState probe_feedback_window_push(
    ProbeFeedbackWindowState state,
    double thrust_lbf [[fisics::dim(force)]] [[fisics::unit(pound_force)]],
    double reserve_wh [[fisics::dim(energy)]] [[fisics::unit(watt_hour)]]) {
    double thrust_n
        [[fisics::dim(force)]]
        [[fisics::unit(newton)]] = fisics_convert_unit(thrust_lbf, "newton");
    double reserve_j
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = fisics_convert_unit(reserve_wh, "joule");
    double leg_energy_j
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = thrust_n * state.distance_m;

    state.energy_j += leg_energy_j + reserve_j;
    return state;
}

double probe_feedback_window_finish(
    ProbeFeedbackWindowState state,
    double score_dt_ms [[fisics::dim(time)]] [[fisics::unit(millisecond)]]) {
    double score_dt_s
        [[fisics::dim(time)]]
        [[fisics::unit(second)]] = fisics_convert_unit(score_dt_ms, "second");
    return state.energy_j / score_dt_s;
}
