#ifndef PROBE_UNITS_MULTITU_CHECKPOINT_CHARGE_BRIDGE_SHARED_H
#define PROBE_UNITS_MULTITU_CHECKPOINT_CHARGE_BRIDGE_SHARED_H

#include "15__probe_units_multitu_shared.h"

typedef struct {
    double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]];
    double energy_j [[fisics::dim(energy)]] [[fisics::unit(joule)]];
    double charge_ah [[fisics::dim(charge)]] [[fisics::unit(ampere_hour)]];
} ProbeCheckpointChargeState;

ProbeCheckpointChargeState probe_checkpoint_charge_seed(
    double speed_fps [[fisics::dim(speed)]] [[fisics::unit(foot_per_second)]],
    double dt_ms [[fisics::dim(time)]] [[fisics::unit(millisecond)]]);
ProbeCheckpointChargeState probe_checkpoint_charge_push_force(
    ProbeCheckpointChargeState state,
    double thrust_lbf [[fisics::dim(force)]] [[fisics::unit(pound_force)]]);
ProbeCheckpointChargeState probe_checkpoint_charge_push_reserve(
    ProbeCheckpointChargeState state,
    double reserve_mah [[fisics::dim(charge)]] [[fisics::unit(milliampere_hour)]],
    double reserve_wh [[fisics::dim(energy)]] [[fisics::unit(watt_hour)]]);
double probe_checkpoint_charge_score(
    ProbeCheckpointChargeState state,
    double window_ft [[fisics::dim(length)]] [[fisics::unit(foot)]]);

#endif
