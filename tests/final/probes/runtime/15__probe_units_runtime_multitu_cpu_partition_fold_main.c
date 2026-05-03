#include <stdio.h>

#include "15__probe_units_runtime_multitu_cpu_partition_fold_shared.h"

int main(void) {
    double speeds_fps[4] = {28.0, 33.5, 41.0, 46.5};
    double dt_ms[4] = {420.0, 610.0, 760.0, 980.0};
    double thrust_lbf[4] = {6.25, 7.5, 8.4, 9.8};
    double reserve_wh[4] = {2.25, 3.5, 4.0, 5.75};
    double reserve_mah[4] = {420.0, 600.0, 730.0, 880.0};
    ProbeCpuPartitionState total = {0.0, 0.0, 0.0};

    for (int i = 0; i < 4; ++i) {
        ProbeCpuPartitionState lane =
            probe_cpu_partition_seed(i, speeds_fps[i], dt_ms[i]);
        lane = probe_cpu_partition_push_force(lane, i, thrust_lbf[i]);
        lane = probe_cpu_partition_push_reserve(lane, reserve_wh[i], reserve_mah[i]);
        total.distance_m += lane.distance_m;
        total.energy_j += lane.energy_j;
        total.charge_ah += lane.charge_ah;
    }

    printf("%.6f %.6f %.6f %.6f\n",
           total.distance_m,
           total.energy_j,
           total.charge_ah,
           probe_cpu_partition_score(total, 210.0));
    return 0;
}
