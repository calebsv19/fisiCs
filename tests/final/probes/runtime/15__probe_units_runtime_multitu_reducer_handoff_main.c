#include <stdio.h>

#include "15__probe_units_runtime_multitu_reducer_handoff_shared.h"

int main(void) {
    double speeds_fps[2] = {32.0, 40.0};
    double dt_ms[2] = {600.0, 850.0};
    double thrust_lbf[2] = {9.0, 10.5};
    double reserve_mah[2] = {1800.0, 2200.0};
    double reserve_wh[2] = {12.0, 18.0};
    ProbeReducerState state = probe_reducer_seed();

    for (int i = 0; i < 2; ++i) {
        state = probe_reducer_push_motion(state, speeds_fps[i], dt_ms[i], thrust_lbf[i]);
        state = probe_reducer_push_reserve(state, reserve_mah[i], reserve_wh[i]);
    }

    printf("%.6f %.6f %.6f %.6f\n",
           state.total_distance_m,
           state.total_energy_j,
           state.total_charge_ah,
           probe_reducer_score(state, 15.0));
    return 0;
}
