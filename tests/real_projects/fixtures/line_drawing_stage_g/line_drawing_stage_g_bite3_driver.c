#include "line_drawing_stage_g_common.h"

#include "Layout/scene/layout_scene_path_edit.h"
#include "Layout/scene/layout_scene_path_geometry.h"

#include <string.h>

int main(void) {
    LineDrawingSceneAuthoringState state;
    LineDrawingScenePath before;
    LineDrawingScenePath after;
    LineDrawingScenePath* path;
    LineDrawingScenePathElementRef element;
    LineDrawingScenePathElementRef inserted;
    LineDrawingScenePathElementRef selected;
    LineDrawingScenePathGeometry geometry;
    size_t path_index = 0u;
    uint64_t digest;

    Layout_SceneAuthoringState_Init(&state);
    if (!Layout_SceneAuthoringState_AddDefaultCameraPath(&state, &path_index) ||
        !Layout_SceneAuthoringState_Select(&state,
            LINE_DRAWING_SCENE_AUTHORING_SELECTION_PATH, path_index)) return 11;
    path = &state.paths[path_index];
    if (!Layout_ScenePathGeometry_IsCompleteCubic(path) ||
        Layout_ScenePathEdit_AnchorCount(path) < 2u) return 12;
    before = *path;
    digest = ld_stageg_authoring_digest(&state);
    ld_stageg_trace("b3_begin", "selected_camera_path", digest);

    if (!Layout_ScenePathEdit_SetAnchorMode(path, 0u,
            LINE_DRAWING_SCENE_PATH_TANGENT_LINKED)) return 13;
    element = Layout_ScenePathEdit_ElementForControl(path, 1u);
    if (element.kind != LINE_DRAWING_SCENE_PATH_ELEMENT_OUTGOING_TANGENT ||
        !Layout_ScenePathEdit_SetElementWorldPoint(path, element,
                                                   (Vec3){2.5f, 4.0f, -1.5f})) return 14;
    ld_stageg_trace("b3_handle_move", "linked_tangent", ld_stageg_authoring_digest(&state));

    memset(&inserted, 0, sizeof(inserted));
    if (!Layout_ScenePathEdit_SplitSegment(path, 0u, 0.5f, &inserted) ||
        inserted.kind != LINE_DRAWING_SCENE_PATH_ELEMENT_ANCHOR ||
        !Layout_ScenePathGeometry_IsCompleteCubic(path)) return 15;
    ld_stageg_trace("b3_split", "segment_half", ld_stageg_authoring_digest(&state));

    if (!Layout_ScenePathEdit_SetElementWorldPoint(path, inserted,
                                                   (Vec3){-1.25f, 0.5f, 3.0f})) return 16;
    if (!Layout_ScenePathEdit_CycleAnchorMode(path, inserted.anchor_index)) return 17;
    ld_stageg_trace("b3_reverse", "anchor_repositioned", ld_stageg_authoring_digest(&state));

    after = *path;
    *path = before;
    if (memcmp(path->control_points, after.control_points,
               sizeof(path->control_points)) == 0) return 18;
    ld_stageg_trace("b3_undo", "snapshot_before_edit", ld_stageg_authoring_digest(&state));
    *path = after;
    if (!Layout_ScenePathGeometry_Build(path, &geometry) || geometry.sample_count < 2u) return 19;
    ld_stageg_trace("b3_redo", "snapshot_after_edit", ld_stageg_authoring_digest(&state));

    memset(&selected, 0, sizeof(selected));
    if (Layout_ScenePathEdit_DeleteElement(
            path, Layout_ScenePathEdit_Segment(99u), &selected) ||
        Layout_ScenePathEdit_SetElementWorldPoint(
            path, Layout_ScenePathEdit_ElementForControl(path, 99u), (Vec3){0})) return 20;
    ld_stageg_trace("b3_invalid", "out_of_range_rejected", 0u);

    if (!Layout_ScenePathEdit_DeleteElement(path, inserted, &selected) ||
        !Layout_ScenePathGeometry_IsCompleteCubic(path)) return 21;
    ld_stageg_trace("b3_delete", "inserted_anchor_removed", ld_stageg_authoring_digest(&state));

    if (!ld_stageg_write_state(&state, "interaction.canonical", "line_drawing_bite3")) return 22;
    digest = ld_stageg_authoring_digest(&state);
    ld_stageg_trace("b3_canonical", "artifact_written", digest);
    ld_stageg_trace("b3_shutdown", "interaction_complete", 0u);
    return 0;
}
