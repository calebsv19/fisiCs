#ifndef PROBE_UNITS_RUNTIME_MULTITU_GPU_RECONCILE_MESH_SHARED_H
#define PROBE_UNITS_RUNTIME_MULTITU_GPU_RECONCILE_MESH_SHARED_H

#include "15__probe_units_multitu_shared.h"

typedef struct ProbeGpuReconcileMeshState {
    double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]];
    double energy_j [[fisics::dim(energy)]] [[fisics::unit(joule)]];
    double charge_ah [[fisics::dim(charge)]] [[fisics::unit(ampere_hour)]];
} ProbeGpuReconcileMeshState;

ProbeGpuReconcileMeshState probe_gpu_reconcile_mesh_seed(
    double speed_fps [[fisics::dim(speed)]] [[fisics::unit(foot_per_second)]],
    double dt_ms [[fisics::dim(time)]] [[fisics::unit(millisecond)]]);
ProbeGpuReconcileMeshState probe_gpu_reconcile_mesh_push(
    ProbeGpuReconcileMeshState state,
    int gpu_tile,
    double thrust_lbf [[fisics::dim(force)]] [[fisics::unit(pound_force)]]);
ProbeGpuReconcileMeshState probe_gpu_reconcile_mesh_merge(
    ProbeGpuReconcileMeshState left,
    ProbeGpuReconcileMeshState right,
    double reserve_wh [[fisics::dim(energy)]] [[fisics::unit(watt_hour)]],
    double reserve_mah [[fisics::dim(charge)]] [[fisics::unit(milliampere_hour)]]);
double probe_gpu_reconcile_mesh_score(
    ProbeGpuReconcileMeshState state,
    double window_ms [[fisics::dim(time)]] [[fisics::unit(millisecond)]],
    double window_ft [[fisics::dim(length)]] [[fisics::unit(foot)]]);

#endif
