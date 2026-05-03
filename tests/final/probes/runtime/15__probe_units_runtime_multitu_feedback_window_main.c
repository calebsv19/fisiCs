#include <stdio.h>

#include "15__probe_units_runtime_multitu_feedback_window_shared.h"

int main(void) {
    double speeds_fps[2] = {38.0, 46.0};
    double dt_ms[2] = {800.0, 1100.0};
    double thrust_lbf[2] = {10.0, 11.75};
    double reserve_wh[2] = {8.0, 11.5};
    ProbeFeedbackWindowState state = probe_feedback_window_seed(speeds_fps[0], dt_ms[0]);

    state = probe_feedback_window_push(state, thrust_lbf[0], reserve_wh[0]);
    state = probe_feedback_window_push(state, thrust_lbf[1], reserve_wh[1]);

    ProbeFeedbackWindowState tail = probe_feedback_window_seed(speeds_fps[1], dt_ms[1]);
    state.distance_m += tail.distance_m;
    state.energy_j += tail.energy_j;

    printf("%.6f %.6f %.6f\n",
           state.distance_m,
           state.energy_j,
           probe_feedback_window_finish(state, 1400.0));
    return 0;
}
