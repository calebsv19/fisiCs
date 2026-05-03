#ifndef PROBE_UNITS_MULTITU_REDUCER_HANDOFF_SHARED_H
#define PROBE_UNITS_MULTITU_REDUCER_HANDOFF_SHARED_H

#include "15__probe_units_multitu_shared.h"

typedef struct {
    double total_distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]];
    double total_energy_j [[fisics::dim(energy)]] [[fisics::unit(joule)]];
    double total_charge_ah [[fisics::dim(charge)]] [[fisics::unit(ampere_hour)]];
} ProbeReducerState;

ProbeReducerState probe_reducer_seed(void);
ProbeReducerState probe_reducer_push_motion(
    ProbeReducerState state,
    double speed_fps [[fisics::dim(speed)]] [[fisics::unit(foot_per_second)]],
    double dt_ms [[fisics::dim(time)]] [[fisics::unit(millisecond)]],
    double thrust_lbf [[fisics::dim(force)]] [[fisics::unit(pound_force)]]);
ProbeReducerState probe_reducer_push_reserve(
    ProbeReducerState state,
    double reserve_mah [[fisics::dim(charge)]] [[fisics::unit(milliampere_hour)]],
    double reserve_wh [[fisics::dim(energy)]] [[fisics::unit(watt_hour)]]);
double probe_reducer_score(
    ProbeReducerState state,
    double window_ft [[fisics::dim(length)]] [[fisics::unit(foot)]]);

#endif
