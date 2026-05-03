#include <stdio.h>

#include "15__probe_units_runtime_multitu_cpu_partition_checkpoint_mesh_shared.h"

int main(void) {
    double speeds_fps[3] = {30.0, 36.5, 42.0};
    double dt_ms[3] = {540.0, 690.0, 870.0};
    double thrust_lbf[3] = {6.8, 8.1, 9.25};
    double reserve_wh[3] = {3.25, 4.5, 5.75};
    double reserve_mah[3] = {510.0, 680.0, 910.0};
    ProbeCpuPartitionCheckpointState total = {0.0, 0.0, 0.0};

    for (int i = 0; i < 3; ++i) {
        ProbeCpuPartitionCheckpointState lane =
            probe_cpu_partition_checkpoint_seed(i, speeds_fps[i], dt_ms[i]);
        lane = probe_cpu_partition_checkpoint_push(
            lane, i, thrust_lbf[i], reserve_wh[i], reserve_mah[i]);
        total.distance_m += lane.distance_m;
        total.energy_j += lane.energy_j;
        total.charge_ah += lane.charge_ah;
    }

    printf("%.6f %.6f %.6f %.6f\n",
           total.distance_m,
           total.energy_j,
           total.charge_ah,
           probe_cpu_partition_checkpoint_score(total, 180.0, 1250.0));
    return 0;
}
