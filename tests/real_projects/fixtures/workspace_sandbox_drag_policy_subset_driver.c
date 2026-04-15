#include <assert.h>
#include <math.h>
#include <stdint.h>

#include "workspace_sandbox/workspace_sandbox_drag_policy.h"

static int nearf_local(float a, float b, float eps) {
    return fabsf(a - b) <= eps;
}

int main(void) {
    WorkspaceSandboxCornerNode corners[3] = {
        { .id = 11u, .x = 10.0f, .y = 10.0f, .degree = 2u, .on_boundary = 0u },
        { .id = 12u, .x = 18.0f, .y = 10.0f, .degree = 3u, .on_boundary = 1u },
        { .id = 13u, .x = 30.0f, .y = 20.0f, .degree = 1u, .on_boundary = 0u },
    };
    uint8_t corner_dirs[3] = { 8u, 2u, 1u };
    uint32_t snapped_corner_id = 0u;
    float snapped_axis = 0.0f;
    int nearest_idx;

    assert(workspace_sandbox_corner_dir_opposition_score(4u, 8u) == 1);
    assert(workspace_sandbox_corner_dir_opposition_score(4u | 1u, 8u | 2u) == 2);

    nearest_idx = workspace_sandbox_find_nearest_corner_idx(corners, 3u, 17.9f, 10.1f, 5.0f);
    assert(nearest_idx == 1);

    assert(workspace_sandbox_select_snap_corner(corners,
                                                3u,
                                                corner_dirs,
                                                17.8f,
                                                10.1f,
                                                10.0f,
                                                1,
                                                4u,
                                                5.0f,
                                                &snapped_corner_id,
                                                &snapped_axis) == 1);
    assert(snapped_corner_id == 12u);
    assert(nearf_local(snapped_axis, 18.0f, 0.001f));

    return 0;
}
