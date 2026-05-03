#ifndef PROBE_UNITS_RUNTIME_MULTITU_GPU_CHECKPOINT_FANOUT_SHARED_H
#define PROBE_UNITS_RUNTIME_MULTITU_GPU_CHECKPOINT_FANOUT_SHARED_H

#include "15__probe_units_multitu_shared.h"

typedef struct ProbeGpuCheckpointFanoutState {
    double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]];
    double energy_j [[fisics::dim(energy)]] [[fisics::unit(joule)]];
    double charge_ah [[fisics::dim(charge)]] [[fisics::unit(ampere_hour)]];
} ProbeGpuCheckpointFanoutState;

ProbeGpuCheckpointFanoutState probe_gpu_checkpoint_fanout_seed(
    int gpu_tile,
    double speed_fps [[fisics::dim(speed)]] [[fisics::unit(foot_per_second)]],
    double dt_ms [[fisics::dim(time)]] [[fisics::unit(millisecond)]]);
ProbeGpuCheckpointFanoutState probe_gpu_checkpoint_fanout_push(
    ProbeGpuCheckpointFanoutState state,
    int gpu_tile,
    double thrust_lbf [[fisics::dim(force)]] [[fisics::unit(pound_force)]],
    double reserve_wh [[fisics::dim(energy)]] [[fisics::unit(watt_hour)]],
    double reserve_mah [[fisics::dim(charge)]] [[fisics::unit(milliampere_hour)]]);
double probe_gpu_checkpoint_fanout_score(
    ProbeGpuCheckpointFanoutState state,
    double window_ft [[fisics::dim(length)]] [[fisics::unit(foot)]],
    double tick_ms [[fisics::dim(time)]] [[fisics::unit(millisecond)]]);

#endif
