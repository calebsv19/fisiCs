#ifndef PROBE_UNITS_MULTITU_CALLBACK_RESERVE_MESH_SHARED_H
#define PROBE_UNITS_MULTITU_CALLBACK_RESERVE_MESH_SHARED_H

#include "15__probe_units_multitu_shared.h"

typedef struct {
    double total_distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]];
    double total_energy_j [[fisics::dim(energy)]] [[fisics::unit(joule)]];
    double total_charge_ah [[fisics::dim(charge)]] [[fisics::unit(ampere_hour)]];
} ProbeCallbackReserveState;

typedef ProbeCallbackReserveState (*ProbeCallbackReserveLane)(
    ProbeCallbackReserveState state,
    double speed_fps [[fisics::dim(speed)]] [[fisics::unit(foot_per_second)]],
    double dt_ms [[fisics::dim(time)]] [[fisics::unit(millisecond)]],
    double thrust_lbf [[fisics::dim(force)]] [[fisics::unit(pound_force)]]);

ProbeCallbackReserveState probe_callback_reserve_seed(void);
ProbeCallbackReserveLane probe_callback_reserve_pick(int lane_id);
ProbeCallbackReserveState probe_callback_reserve_push_charge(
    ProbeCallbackReserveState state,
    double reserve_mah [[fisics::dim(charge)]] [[fisics::unit(milliampere_hour)]]);
double probe_callback_reserve_score(
    ProbeCallbackReserveState state,
    double window_ft [[fisics::dim(length)]] [[fisics::unit(foot)]]);

#endif
