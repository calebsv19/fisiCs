#include <stdio.h>

#include "15__probe_units_runtime_multitu_gpu_reconcile_mesh_shared.h"

int main(void) {
    ProbeGpuReconcileMeshState a = probe_gpu_reconcile_mesh_seed(28.0, 410.0);
    ProbeGpuReconcileMeshState b = probe_gpu_reconcile_mesh_seed(33.5, 520.0);
    ProbeGpuReconcileMeshState c = probe_gpu_reconcile_mesh_seed(40.0, 690.0);
    ProbeGpuReconcileMeshState d = probe_gpu_reconcile_mesh_seed(45.0, 830.0);

    a = probe_gpu_reconcile_mesh_push(a, 0, 6.1);
    b = probe_gpu_reconcile_mesh_push(b, 1, 7.0);
    c = probe_gpu_reconcile_mesh_push(c, 2, 8.2);
    d = probe_gpu_reconcile_mesh_push(d, 3, 9.0);

    ProbeGpuReconcileMeshState left =
        probe_gpu_reconcile_mesh_merge(a, b, 3.2, 510.0);
    ProbeGpuReconcileMeshState right =
        probe_gpu_reconcile_mesh_merge(c, d, 5.4, 860.0);
    ProbeGpuReconcileMeshState total =
        probe_gpu_reconcile_mesh_merge(left, right, 6.9, 980.0);

    printf("%.6f %.6f %.6f %.6f\n",
           total.distance_m,
           total.energy_j,
           total.charge_ah,
           probe_gpu_reconcile_mesh_score(total, 1810.0, 188.0));
    return 0;
}
