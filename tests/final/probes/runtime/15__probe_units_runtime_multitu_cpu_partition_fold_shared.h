#ifndef PROBE_UNITS_RUNTIME_MULTITU_CPU_PARTITION_FOLD_SHARED_H
#define PROBE_UNITS_RUNTIME_MULTITU_CPU_PARTITION_FOLD_SHARED_H

#include "15__probe_units_multitu_shared.h"

typedef struct ProbeCpuPartitionState {
    double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]];
    double energy_j [[fisics::dim(energy)]] [[fisics::unit(joule)]];
    double charge_ah [[fisics::dim(charge)]] [[fisics::unit(ampere_hour)]];
} ProbeCpuPartitionState;

ProbeCpuPartitionState probe_cpu_partition_seed(
    int cpu_slot,
    double speed_fps [[fisics::dim(speed)]] [[fisics::unit(foot_per_second)]],
    double dt_ms [[fisics::dim(time)]] [[fisics::unit(millisecond)]]);
ProbeCpuPartitionState probe_cpu_partition_push_force(
    ProbeCpuPartitionState state,
    int cpu_slot,
    double thrust_lbf [[fisics::dim(force)]] [[fisics::unit(pound_force)]]);
ProbeCpuPartitionState probe_cpu_partition_push_reserve(
    ProbeCpuPartitionState state,
    double reserve_wh [[fisics::dim(energy)]] [[fisics::unit(watt_hour)]],
    double reserve_mah [[fisics::dim(charge)]] [[fisics::unit(milliampere_hour)]]);
double probe_cpu_partition_score(
    ProbeCpuPartitionState state,
    double window_ft [[fisics::dim(length)]] [[fisics::unit(foot)]]);

#endif
