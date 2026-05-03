#include <stdio.h>

#include "15__probe_units_runtime_multitu_cpu_reconcile_window_shared.h"

int main(void) {
    ProbeCpuReconcileState first = probe_cpu_reconcile_seed(32.0, 500.0);
    ProbeCpuReconcileState second = probe_cpu_reconcile_seed(41.0, 720.0);
    ProbeCpuReconcileState third = probe_cpu_reconcile_seed(37.0, 690.0);

    first = probe_cpu_reconcile_push_force(first, 8.0);
    second = probe_cpu_reconcile_push_force(second, 9.5);
    third = probe_cpu_reconcile_push_force(third, 8.75);

    ProbeCpuReconcileState merged =
        probe_cpu_reconcile_merge(first, second, 5.0, 620.0);
    merged = probe_cpu_reconcile_merge(merged, third, 6.5, 910.0);

    printf("%.6f %.6f %.6f %.6f\n",
           merged.distance_m,
           merged.energy_j,
           merged.charge_ah,
           probe_cpu_reconcile_score(merged, 1500.0, 95.0));
    return 0;
}
