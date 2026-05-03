#ifndef PROBE_UNITS_RUNTIME_MULTITU_GPU_FEEDBACK_LADDER_SHARED_H
#define PROBE_UNITS_RUNTIME_MULTITU_GPU_FEEDBACK_LADDER_SHARED_H

#include "15__probe_units_multitu_shared.h"

typedef struct ProbeGpuFeedbackLadderState {
    double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]];
    double energy_j [[fisics::dim(energy)]] [[fisics::unit(joule)]];
    double charge_ah [[fisics::dim(charge)]] [[fisics::unit(ampere_hour)]];
} ProbeGpuFeedbackLadderState;

ProbeGpuFeedbackLadderState probe_gpu_feedback_ladder_seed(
    double speed_fps [[fisics::dim(speed)]] [[fisics::unit(foot_per_second)]],
    double dt_ms [[fisics::dim(time)]] [[fisics::unit(millisecond)]]);
ProbeGpuFeedbackLadderState probe_gpu_feedback_ladder_push(
    ProbeGpuFeedbackLadderState state,
    int gpu_tile,
    double thrust_lbf [[fisics::dim(force)]] [[fisics::unit(pound_force)]]);
ProbeGpuFeedbackLadderState probe_gpu_feedback_ladder_merge(
    ProbeGpuFeedbackLadderState left,
    ProbeGpuFeedbackLadderState right,
    double reserve_wh [[fisics::dim(energy)]] [[fisics::unit(watt_hour)]],
    double reserve_mah [[fisics::dim(charge)]] [[fisics::unit(milliampere_hour)]]);
double probe_gpu_feedback_ladder_score(
    ProbeGpuFeedbackLadderState state,
    double window_ms [[fisics::dim(time)]] [[fisics::unit(millisecond)]],
    double window_ft [[fisics::dim(length)]] [[fisics::unit(foot)]]);

#endif
