#include "workspace_sandbox_stage_g_common.h"

int main(void) {
    WorkspaceSandboxRuntime runtime;
    WorkspaceSandboxCornerNode corners[WORKSPACE_SANDBOX_DERIVED_CORNER_CAPACITY] = {{0}};
    WorkspaceSandboxSeamEdge seams[WORKSPACE_SANDBOX_DERIVED_SEAM_CAPACITY] = {{0}};
    CorePaneLeafRect leaves[WORKSPACE_SANDBOX_MAX_LEAVES] = {{0}};
    WorkspaceSandboxModuleInstance instance = {0};
    CorePaneId new_pane = 0u;
    uint32_t corner_count = 0u;
    uint32_t seam_count = 0u;
    uint32_t leaf_count = 0u;
    uint64_t hash_before_undo;
    CoreResult result;

    result = workspace_sandbox_runtime_init(&runtime);
    if (result.code != CORE_OK || runtime.layout_state.mode != CORE_LAYOUT_MODE_RUNTIME) return 11;
    result = workspace_sandbox_build_derived_graph(&runtime,
                                                   (CorePaneRect){0.0f, 0.0f, 1200.0f, 800.0f},
                                                   corners,
                                                   WORKSPACE_SANDBOX_DERIVED_CORNER_CAPACITY,
                                                   &corner_count,
                                                   seams,
                                                   WORKSPACE_SANDBOX_DERIVED_SEAM_CAPACITY,
                                                   &seam_count);
    if (result.code != CORE_OK || corner_count == 0u || seam_count == 0u) return 12;
    ws_stageg_trace("b1_initialized", "derived_graph", ((uint64_t)corner_count << 32u) | seam_count);

    result = workspace_sandbox_enter_authoring_layout(&runtime);
    if (result.code != CORE_OK || !runtime.draft_undo_redo.active) return 13;
    ws_stageg_trace("b1_authoring", "transaction_open", runtime.layout_state.active_revision);

    result = workspace_sandbox_split_pane(&runtime, 3u, CORE_PANE_AXIS_VERTICAL, 0.40f, &new_pane);
    if (result.code != CORE_OK || new_pane == 0u || !workspace_sandbox_can_undo(&runtime)) return 14;
    ws_stageg_trace("b1_split", "pane_added", new_pane);

    result = workspace_sandbox_assign_module(&runtime, 1u, WORKSPACE_SANDBOX_MODULE_TEXT, 0);
    if (result.code != CORE_OK) return 15;
    result = workspace_sandbox_assign_module(&runtime, new_pane, WORKSPACE_SANDBOX_MODULE_INSPECTOR, 0);
    if (result.code != CORE_OK) return 16;
    result = workspace_sandbox_get_module_for_pane(&runtime, new_pane, &instance);
    if (result.code != CORE_OK || instance.kind != WORKSPACE_SANDBOX_MODULE_INSPECTOR) return 17;
    ws_stageg_trace("b1_modules", "draft_assignments", runtime.draft_modules.count);

    result = workspace_sandbox_swap_panes(&runtime, 1u, 3u);
    if (result.code != CORE_OK) return 18;
    hash_before_undo = ws_stageg_semantic_hash(&(WorkspaceSandboxRuntime){
        .active_layout = runtime.draft_layout,
        .active_modules = runtime.draft_modules
    });
    ws_stageg_trace("b1_swapped", "leaf_order_changed", hash_before_undo);

    result = workspace_sandbox_undo(&runtime);
    if (result.code != CORE_OK || !workspace_sandbox_can_redo(&runtime)) return 19;
    ws_stageg_trace("b1_undo", "history_back", runtime.draft_undo_redo.cursor);
    result = workspace_sandbox_redo(&runtime);
    if (result.code != CORE_OK || runtime.draft_undo_redo.cursor + 1u != runtime.draft_undo_redo.count) return 20;
    ws_stageg_trace("b1_redo", "history_forward", runtime.draft_undo_redo.cursor);

    result = workspace_sandbox_apply_authoring(&runtime);
    if (result.code != CORE_OK || runtime.layout_state.mode != CORE_LAYOUT_MODE_RUNTIME ||
        !runtime.layout_state.rebuild_required) return 21;
    result = workspace_sandbox_build_layout(&runtime,
                                            (CorePaneRect){0.0f, 0.0f, 1200.0f, 800.0f},
                                            leaves,
                                            WORKSPACE_SANDBOX_MAX_LEAVES,
                                            &leaf_count);
    if (result.code != CORE_OK || leaf_count != 4u) return 22;
    ws_stageg_trace("b1_applied", "active_layout", ((uint64_t)leaf_count << 32u) | runtime.active_layout.node_count);

    workspace_sandbox_acknowledge_rebuild(&runtime);
    result = workspace_sandbox_set_root_ratio(&runtime, 0.2f);
    if (result.code != CORE_ERR_INVALID_ARG || runtime.layout_state.rebuild_required) return 23;
    ws_stageg_trace("b1_invalid", "runtime_mutation_rejected", result.code);

    if (!ws_stageg_write_canonical(&runtime, "layout.canonical", "workspace_sandbox_bite1")) return 24;
    ws_stageg_trace("b1_canonical", "artifact_written", ws_stageg_semantic_hash(&runtime));
    ws_stageg_trace("b1_shutdown", "state_complete", 0u);
    return 0;
}
