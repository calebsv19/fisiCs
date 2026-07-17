#include "line_drawing_stage_g_common.h"

#include "Layout/scene/layout_scene_camera_authoring.h"
#include "Layout/scene/layout_scene_light_authoring.h"
#include "Layout/scene/layout_scene_path_geometry.h"
#include "Layout/scene/layout_scene_path_traversal.h"

#include <math.h>
#include <string.h>

int main(void) {
    LineDrawingSceneAuthoringState state;
    LineDrawingScenePathGeometry geometry;
    LineDrawingScenePathTraversalTable table;
    LineDrawingScenePathTraversalSample sample;
    LineDrawingSceneCameraPose pose;
    Vec3 light_position;
    size_t camera_path_index = 0u;
    size_t light_path_index = 0u;
    size_t light_index = 0u;
    size_t material_index = 0u;
    uint64_t digest;
    LineDrawingScenePath invalid_path;

    Layout_SceneAuthoringState_Init(&state);
    if (!Layout_SceneAuthoringState_AddDefaultCameraPath(&state, &camera_path_index) ||
        !Layout_SceneAuthoringState_AddDefaultLightPath(&state, &light_path_index) ||
        !Layout_SceneAuthoringState_AddDefaultLight(&state, &light_index) ||
        !Layout_SceneAuthoringState_AddDefaultMaterial(&state, &material_index)) return 11;
    if (state.camera_count != 2u || state.path_count != 4u ||
        state.light_count != 2u || state.material_count != 2u) return 12;
    digest = ld_stageg_authoring_digest(&state);
    ld_stageg_trace("b1_initialized", "camera_light_paths", digest);

    if (!Layout_SceneAuthoringState_Select(&state,
            LINE_DRAWING_SCENE_AUTHORING_SELECTION_PATH, camera_path_index)) return 13;
    if (!Layout_SceneAuthoringState_SetPathControlPoint(&state, camera_path_index, 0u,
                                                        (Vec3){-3.0f, 1.0f, 2.0f}) ||
        !Layout_SceneAuthoringState_SetPathControlPoint(&state, camera_path_index, 3u,
                                                        (Vec3){4.0f, 2.0f, -1.0f})) return 14;
    state.paths[camera_path_index].duration_seconds = 8.0f;
    state.paths[camera_path_index].playback_mode = LINE_DRAWING_SCENE_PATH_PLAYBACK_LOOP;
    state.paths[camera_path_index].playing = true;
    digest = ld_stageg_authoring_digest(&state);
    ld_stageg_trace("b1_mutated", "deterministic_controls", digest);

    if (!Layout_ScenePathGeometry_Build(&state.paths[camera_path_index], &geometry) ||
        geometry.sample_count < 2u || geometry.source_segment_count == 0u) return 15;
    ld_stageg_trace("b1_geometry", Layout_ScenePathGeometry_KindName(geometry.kind),
                    (uint64_t)geometry.sample_count);

    if (!Layout_ScenePathTraversal_Build(&state.paths[camera_path_index], &table) ||
        table.sample_count < 2u || table.total_distance <= 0.0f) return 16;
    if (!Layout_ScenePathTraversal_EvaluateNormalized(
            &table, 0.5f, LINE_DRAWING_SCENE_PATH_PLAYBACK_LOOP, &sample)) return 17;
    if (sample.normalized_distance < 0.49f || sample.normalized_distance > 0.51f) return 18;
    ld_stageg_trace("b1_traversal", "normalized_midpoint",
                    (uint64_t)(table.total_distance * 1000000.0f));

    if (!Layout_ScenePathTraversal_EvaluateTime(
            &table, 10.0f, 8.0f, LINE_DRAWING_SCENE_PATH_PLAYBACK_LOOP, &sample)) return 19;
    if (sample.normalized_distance < 0.24f || sample.normalized_distance > 0.26f) return 20;
    if (!Layout_ScenePathTraversal_Advance(&state.paths[camera_path_index], 2.0f) ||
        fabsf(state.paths[camera_path_index].normalized_distance - 0.25f) > 0.0001f) return 21;
    ld_stageg_trace("b1_playback", "loop_time_and_advance",
                    (uint64_t)(sample.normalized_distance * 1000000.0f));

    if (!Layout_SceneCamera_EvaluatePoseAtNormalizedDistance(
            &state.cameras[0], &state.paths[camera_path_index], 0.5f, &pose)) return 22;
    if (!Layout_SceneLight_EvaluatePositionAtNormalizedDistance(
            &state.lights[0], &state.paths[light_path_index], 0.5f, &light_position)) return 23;
    ld_stageg_trace("b1_bound_actors", "camera_and_light",
                    (uint64_t)((pose.position.x + light_position.x + 32.0f) * 1000000.0f));

    memset(&invalid_path, 0, sizeof(invalid_path));
    if (Layout_ScenePathGeometry_Build(&invalid_path, &geometry) ||
        Layout_ScenePathTraversal_Build(&invalid_path, &table) ||
        Layout_ScenePathTraversal_EvaluateTime(&table, 1.0f, 0.0f,
                                               LINE_DRAWING_SCENE_PATH_PLAYBACK_ONCE,
                                               &sample)) return 24;
    ld_stageg_trace("b1_invalid", "empty_and_zero_duration_rejected", 0u);

    if (!ld_stageg_write_state(&state, "scene_state.canonical", "line_drawing_bite1")) return 25;
    digest = ld_stageg_authoring_digest(&state);
    ld_stageg_trace("b1_canonical", "artifact_written", digest);
    ld_stageg_trace("b1_shutdown", "stack_state_complete", 0u);
    return 0;
}
