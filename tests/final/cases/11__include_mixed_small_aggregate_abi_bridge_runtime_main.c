#include <stdio.h>

#include "11__include_mixed_small_aggregate_abi_bridge_runtime.h"

static int nearly_equal(float a, float b) {
    float diff = a - b;
    if (diff < 0.0f) diff = -diff;
    return diff < 0.001f;
}

int main(void) {
    probe_mixed_view_set((ProbeViewPlane){ .axis = PROBE_VIEW_YZ, .offset = 4.25f });
    if (!nearly_equal(probe_mixed_view_offset(), 4.25f)) {
        printf("set %.2f\n", probe_mixed_view_offset());
        return 1;
    }

    ProbeViewPlane plane = probe_mixed_view_get();
    if (plane.axis != PROBE_VIEW_YZ || !nearly_equal(plane.offset, 4.25f)) {
        printf("get %d %.2f\n", (int)plane.axis, plane.offset);
        return 2;
    }

    probe_mixed_view_set_parts(PROBE_VIEW_XZ, 6.5f);
    ProbeViewContext ctx = probe_mixed_view_context();
    if (ctx.plane.axis != PROBE_VIEW_XZ || !nearly_equal(ctx.plane.offset, 6.5f) || ctx.pad != 7) {
        printf("ctx %d %.2f %d\n", (int)ctx.plane.axis, ctx.plane.offset, ctx.pad);
        return 3;
    }

    printf("%d %.2f %d %.2f %d\n",
           (int)plane.axis,
           plane.offset,
           (int)ctx.plane.axis,
           ctx.plane.offset,
           ctx.pad);
    return 0;
}
