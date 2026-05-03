#include <stdio.h>

#include "15__probe_units_runtime_multitu_gpu_partition_blend_shared.h"

int main(void) {
    double speeds_fps[4] = {31.0, 37.5, 43.0, 46.5};
    double dt_ms[4] = {420.0, 560.0, 710.0, 860.0};
    double thrust_lbf[4] = {5.9, 6.8, 7.7, 8.6};
    double reserve_wh[4] = {2.8, 3.4, 4.2, 5.1};
    double reserve_mah[4] = {460.0, 590.0, 720.0, 840.0};
    ProbeGpuPartitionBlendState total = {0.0, 0.0, 0.0};

    for (int i = 0; i < 4; ++i) {
        ProbeGpuPartitionBlendState tile =
            probe_gpu_partition_blend_seed(i, speeds_fps[i], dt_ms[i]);
        tile = probe_gpu_partition_blend_push(
            tile, i, thrust_lbf[i], reserve_wh[i], reserve_mah[i]);
        total.distance_m += tile.distance_m;
        total.energy_j += tile.energy_j;
        total.charge_ah += tile.charge_ah;
    }

    printf("%.6f %.6f %.6f %.6f\n",
           total.distance_m,
           total.energy_j,
           total.charge_ah,
           probe_gpu_partition_blend_score(total, 210.0, 1480.0));
    return 0;
}
