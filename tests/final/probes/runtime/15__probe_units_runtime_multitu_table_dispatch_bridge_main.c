#include <stdio.h>

#include "15__probe_units_multitu_shared.h"

typedef double (*ProbeDistanceLane)(
    double speed_fps [[fisics::dim(speed)]] [[fisics::unit(foot_per_second)]],
    double dt_ms [[fisics::dim(time)]] [[fisics::unit(millisecond)]]);

ProbeDistanceLane probe_select_distance_lane(int lane_id);
double probe_dispatch_energy(
    double thrust_lbf [[fisics::dim(force)]] [[fisics::unit(pound_force)]],
    double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]]);

int main(void) {
    double speeds_fps[3] = {30.0, 36.0, 42.0};
    double dt_ms[3] = {500.0, 750.0, 1000.0};
    double thrust_lbf[3] = {8.0, 9.5, 11.0};
    double total_distance_m
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = 0.0;
    double total_energy_j
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = 0.0;

    for (int i = 0; i < 3; ++i) {
        ProbeDistanceLane lane = probe_select_distance_lane(i & 1);
        double leg_distance_m
            [[fisics::dim(length)]]
            [[fisics::unit(meter)]] = lane(speeds_fps[i], dt_ms[i]);
        double leg_energy_j
            [[fisics::dim(energy)]]
            [[fisics::unit(joule)]] =
                probe_dispatch_energy(thrust_lbf[i], leg_distance_m);
        total_distance_m += leg_distance_m;
        total_energy_j += leg_energy_j;
    }

    printf("%.6f %.6f\n", total_distance_m, total_energy_j);
    return 0;
}
