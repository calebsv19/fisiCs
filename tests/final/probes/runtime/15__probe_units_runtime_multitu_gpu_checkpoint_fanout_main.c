#include <stdio.h>

#include "15__probe_units_runtime_multitu_gpu_checkpoint_fanout_shared.h"

int main(void) {
    double speeds_fps[3] = {29.5, 36.0, 41.5};
    double dt_ms[3] = {510.0, 680.0, 840.0};
    double thrust_lbf[3] = {6.2, 7.1, 8.4};
    double reserve_wh[3] = {3.1, 4.0, 5.2};
    double reserve_mah[3] = {490.0, 660.0, 830.0};
    ProbeGpuCheckpointFanoutState total = {0.0, 0.0, 0.0};

    for (int i = 0; i < 3; ++i) {
        ProbeGpuCheckpointFanoutState tile =
            probe_gpu_checkpoint_fanout_seed(i, speeds_fps[i], dt_ms[i]);
        tile = probe_gpu_checkpoint_fanout_push(
            tile, i, thrust_lbf[i], reserve_wh[i], reserve_mah[i]);
        total.distance_m += tile.distance_m;
        total.energy_j += tile.energy_j;
        total.charge_ah += tile.charge_ah;
    }

    printf("%.6f %.6f %.6f %.6f\n",
           total.distance_m,
           total.energy_j,
           total.charge_ah,
           probe_gpu_checkpoint_fanout_score(total, 196.0, 1360.0));
    return 0;
}
