#include "11__include_mixed_small_aggregate_abi_bridge_runtime.h"

static ProbeViewPlane g_probe_plane;

void probe_mixed_view_set(ProbeViewPlane plane) {
    g_probe_plane.axis = plane.axis;
    g_probe_plane.offset = plane.offset;
}

void probe_mixed_view_set_parts(ProbeViewAxis axis, float offset) {
    g_probe_plane.axis = axis;
    g_probe_plane.offset = offset;
}

ProbeViewPlane probe_mixed_view_get(void) {
    ProbeViewPlane result = { .axis = PROBE_VIEW_XY, .offset = 0.0f };
    result.axis = g_probe_plane.axis;
    result.offset = g_probe_plane.offset;
    return result;
}

float probe_mixed_view_offset(void) {
    return g_probe_plane.offset;
}

ProbeViewContext probe_mixed_view_context(void) {
    ProbeViewContext ctx = {0};
    ctx.plane = probe_mixed_view_get();
    ctx.pad = 7;
    return ctx;
}
