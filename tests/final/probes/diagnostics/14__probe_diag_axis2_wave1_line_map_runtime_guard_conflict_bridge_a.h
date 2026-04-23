#ifndef AXIS2_WAVE1_GUARD_BRIDGE_A_H
#define AXIS2_WAVE1_GUARD_BRIDGE_A_H

#line 140101 "axis2_wave1_guard_layer_a.h"
#include "14__probe_diag_axis2_wave1_line_map_runtime_guard_conflict_bridge_b.h"

static int axis2_wave1_guard_bridge_entry(void) {
    if (AX2_W1_GUARD_FROM_LAYER_B()) {
        return 1;
    }
    return 0;
}

#endif
