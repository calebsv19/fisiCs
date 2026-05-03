#include <stdio.h>

#include "15__probe_units_runtime_multitu_cpu_reconcile_partition_ladder_shared.h"

int main(void) {
    ProbeCpuReconcilePartitionState a = probe_cpu_reconcile_partition_seed(27.0, 420.0);
    ProbeCpuReconcilePartitionState b = probe_cpu_reconcile_partition_seed(34.0, 580.0);
    ProbeCpuReconcilePartitionState c = probe_cpu_reconcile_partition_seed(39.5, 760.0);
    ProbeCpuReconcilePartitionState d = probe_cpu_reconcile_partition_seed(44.0, 910.0);

    a = probe_cpu_reconcile_partition_push_force(a, 6.5);
    b = probe_cpu_reconcile_partition_push_force(b, 7.4);
    c = probe_cpu_reconcile_partition_push_force(c, 8.6);
    d = probe_cpu_reconcile_partition_push_force(d, 9.2);

    ProbeCpuReconcilePartitionState left =
        probe_cpu_reconcile_partition_merge(a, b, 3.75, 540.0);
    ProbeCpuReconcilePartitionState right =
        probe_cpu_reconcile_partition_merge(c, d, 5.25, 860.0);
    ProbeCpuReconcilePartitionState total =
        probe_cpu_reconcile_partition_merge(left, right, 6.75, 990.0);

    printf("%.6f %.6f %.6f %.6f\n",
           total.distance_m,
           total.energy_j,
           total.charge_ah,
           probe_cpu_reconcile_partition_score(total, 1725.0, 140.0));
    return 0;
}
