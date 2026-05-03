#include <stdio.h>

#include "15__probe_units_runtime_multitu_callback_reserve_mesh_shared.h"

int main(void) {
    double speeds_fps[3] = {28.0, 34.0, 41.0};
    double dt_ms[3] = {450.0, 700.0, 950.0};
    double thrust_lbf[3] = {7.5, 8.25, 9.0};
    double reserve_mah[3] = {900.0, 1200.0, 1500.0};
    ProbeCallbackReserveState state = probe_callback_reserve_seed();

    for (int i = 0; i < 3; ++i) {
        ProbeCallbackReserveLane lane = probe_callback_reserve_pick(i % 2);
        state = lane(state, speeds_fps[i], dt_ms[i], thrust_lbf[i]);
        state = probe_callback_reserve_push_charge(state, reserve_mah[i]);
    }

    printf("%.6f %.6f %.6f %.6f\n",
           state.total_distance_m,
           state.total_energy_j,
           state.total_charge_ah,
           probe_callback_reserve_score(state, 24.0));
    return 0;
}
