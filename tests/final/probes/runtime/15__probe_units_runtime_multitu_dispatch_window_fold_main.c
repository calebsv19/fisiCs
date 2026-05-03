#include <stdio.h>

#include "15__probe_units_runtime_multitu_dispatch_window_fold_shared.h"

int main(void) {
    double speeds_fps[3] = {29.0, 35.0, 43.0};
    double dt_ms[3] = {500.0, 780.0, 1025.0};
    double thrust_lbf[3] = {7.75, 9.0, 10.5};
    double reserve_wh[3] = {4.5, 6.0, 7.75};
    double total_distance_m
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = 0.0;
    double total_energy_j
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = 0.0;

    for (int i = 0; i < 3; ++i) {
        ProbeDispatchWindowLane lane = probe_dispatch_window_pick(i & 1);
        double distance_m
            [[fisics::dim(length)]]
            [[fisics::unit(meter)]] = lane(speeds_fps[i], dt_ms[i]);
        double energy_j
            [[fisics::dim(energy)]]
            [[fisics::unit(joule)]] =
                probe_dispatch_window_fold_energy(distance_m, thrust_lbf[i], reserve_wh[i]);
        total_distance_m += distance_m;
        total_energy_j += energy_j;
    }

    printf("%.6f %.6f %.6f\n",
           total_distance_m,
           total_energy_j,
           probe_dispatch_window_score(total_energy_j, total_distance_m, 1600.0));
    return 0;
}
