#ifndef PROBE_UNITS_RUNTIME_MULTITU_CPU_RECONCILE_WINDOW_SHARED_H
#define PROBE_UNITS_RUNTIME_MULTITU_CPU_RECONCILE_WINDOW_SHARED_H

#include "15__probe_units_multitu_shared.h"

typedef struct ProbeCpuReconcileState {
    double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]];
    double energy_j [[fisics::dim(energy)]] [[fisics::unit(joule)]];
    double charge_ah [[fisics::dim(charge)]] [[fisics::unit(ampere_hour)]];
} ProbeCpuReconcileState;

ProbeCpuReconcileState probe_cpu_reconcile_seed(
    double speed_fps [[fisics::dim(speed)]] [[fisics::unit(foot_per_second)]],
    double dt_ms [[fisics::dim(time)]] [[fisics::unit(millisecond)]]);
ProbeCpuReconcileState probe_cpu_reconcile_push_force(
    ProbeCpuReconcileState state,
    double thrust_lbf [[fisics::dim(force)]] [[fisics::unit(pound_force)]]);
ProbeCpuReconcileState probe_cpu_reconcile_merge(
    ProbeCpuReconcileState left,
    ProbeCpuReconcileState right,
    double reserve_wh [[fisics::dim(energy)]] [[fisics::unit(watt_hour)]],
    double reserve_mah [[fisics::dim(charge)]] [[fisics::unit(milliampere_hour)]]);
double probe_cpu_reconcile_score(
    ProbeCpuReconcileState state,
    double window_ms [[fisics::dim(time)]] [[fisics::unit(millisecond)]],
    double window_ft [[fisics::dim(length)]] [[fisics::unit(foot)]]);

#endif
