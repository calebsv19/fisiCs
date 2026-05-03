#ifndef PROBE_UNITS_MULTITU_FEEDBACK_WINDOW_SHARED_H
#define PROBE_UNITS_MULTITU_FEEDBACK_WINDOW_SHARED_H

#include "15__probe_units_multitu_shared.h"

typedef struct {
    double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]];
    double energy_j [[fisics::dim(energy)]] [[fisics::unit(joule)]];
} ProbeFeedbackWindowState;

ProbeFeedbackWindowState probe_feedback_window_seed(
    double speed_fps [[fisics::dim(speed)]] [[fisics::unit(foot_per_second)]],
    double dt_ms [[fisics::dim(time)]] [[fisics::unit(millisecond)]]);
ProbeFeedbackWindowState probe_feedback_window_push(
    ProbeFeedbackWindowState state,
    double thrust_lbf [[fisics::dim(force)]] [[fisics::unit(pound_force)]],
    double reserve_wh [[fisics::dim(energy)]] [[fisics::unit(watt_hour)]]);
double probe_feedback_window_finish(
    ProbeFeedbackWindowState state,
    double score_dt_ms [[fisics::dim(time)]] [[fisics::unit(millisecond)]]);

#endif
