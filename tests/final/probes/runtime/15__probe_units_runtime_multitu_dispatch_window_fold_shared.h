#ifndef PROBE_UNITS_MULTITU_DISPATCH_WINDOW_FOLD_SHARED_H
#define PROBE_UNITS_MULTITU_DISPATCH_WINDOW_FOLD_SHARED_H

#include "15__probe_units_multitu_shared.h"

typedef double (*ProbeDispatchWindowLane)(
    double speed_fps [[fisics::dim(speed)]] [[fisics::unit(foot_per_second)]],
    double dt_ms [[fisics::dim(time)]] [[fisics::unit(millisecond)]]);

ProbeDispatchWindowLane probe_dispatch_window_pick(int lane_id);
double probe_dispatch_window_fold_energy(
    double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]],
    double thrust_lbf [[fisics::dim(force)]] [[fisics::unit(pound_force)]],
    double reserve_wh [[fisics::dim(energy)]] [[fisics::unit(watt_hour)]]);
double probe_dispatch_window_score(
    double total_energy_j [[fisics::dim(energy)]] [[fisics::unit(joule)]],
    double total_distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]],
    double window_ms [[fisics::dim(time)]] [[fisics::unit(millisecond)]]);

#endif
