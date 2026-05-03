#ifndef PROBE_UNITS_RUNTIME_MULTITU_CPU_RECONCILE_PARTITION_LADDER_SHARED_H
#define PROBE_UNITS_RUNTIME_MULTITU_CPU_RECONCILE_PARTITION_LADDER_SHARED_H

#include "15__probe_units_multitu_shared.h"

typedef struct ProbeCpuReconcilePartitionState {
    double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]];
    double energy_j [[fisics::dim(energy)]] [[fisics::unit(joule)]];
    double charge_ah [[fisics::dim(charge)]] [[fisics::unit(ampere_hour)]];
} ProbeCpuReconcilePartitionState;

ProbeCpuReconcilePartitionState probe_cpu_reconcile_partition_seed(
    double speed_fps [[fisics::dim(speed)]] [[fisics::unit(foot_per_second)]],
    double dt_ms [[fisics::dim(time)]] [[fisics::unit(millisecond)]]);
ProbeCpuReconcilePartitionState probe_cpu_reconcile_partition_push_force(
    ProbeCpuReconcilePartitionState state,
    double thrust_lbf [[fisics::dim(force)]] [[fisics::unit(pound_force)]]);
ProbeCpuReconcilePartitionState probe_cpu_reconcile_partition_merge(
    ProbeCpuReconcilePartitionState left,
    ProbeCpuReconcilePartitionState right,
    double reserve_wh [[fisics::dim(energy)]] [[fisics::unit(watt_hour)]],
    double reserve_mah [[fisics::dim(charge)]] [[fisics::unit(milliampere_hour)]]);
double probe_cpu_reconcile_partition_score(
    ProbeCpuReconcilePartitionState state,
    double window_ms [[fisics::dim(time)]] [[fisics::unit(millisecond)]],
    double window_ft [[fisics::dim(length)]] [[fisics::unit(foot)]]);

#endif
