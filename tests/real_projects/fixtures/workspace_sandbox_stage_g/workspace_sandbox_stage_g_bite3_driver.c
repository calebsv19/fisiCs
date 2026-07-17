#include "workspace_sandbox_stage_g_common.h"

#include <string.h>

int main(void) {
    WorkspaceSandboxRuntime runtime;
    WorkspaceSandboxRuntime paths_reload;
    CorePaneSplitterHit hit = {0};
    CorePaneId selected = 3u;
    WorkspaceSandboxModuleInstance instance = {0};
    const char *action_id = 0;
    char session_path[WORKSPACE_SANDBOX_PATH_CAPACITY] = {0};
    char text_zoom_path[WORKSPACE_SANDBOX_PATH_CAPACITY] = {0};
    char font_theme_path[WORKSPACE_SANDBOX_PATH_CAPACITY] = {0};
    int pending_apply = 0;
    int32_t clamped_ratio;
    int32_t final_ratio;
    FILE *canonical;
    CoreResult result;

    result = workspace_sandbox_runtime_init(&runtime);
    if (result.code != CORE_OK || runtime.actions.action_count == 0u) return 11;
    ws_stageg_trace("b3_initialized", "actions_registered", runtime.actions.action_count);

    result = workspace_sandbox_resolve_trigger(&runtime, "tab", &action_id);
    if (result.code != CORE_OK || strcmp(action_id, "workspace.cycle_overlay") != 0) return 12;
    result = workspace_sandbox_execute_action(&runtime, "workspace.toggle_mode", &selected, &pending_apply);
    if (result.code != CORE_OK || runtime.layout_state.mode != CORE_LAYOUT_MODE_AUTHORING) return 13;
    result = workspace_sandbox_execute_action(&runtime, action_id, &selected, &pending_apply);
    if (result.code != CORE_OK || runtime.authoring_overlay_mode != WORKSPACE_SANDBOX_AUTHORING_OVERLAY_FONT_THEME) return 14;
    result = workspace_sandbox_resolve_trigger(&runtime, "h", &action_id);
    if (result.code != CORE_OK || strcmp(action_id, "workspace.split_horizontal") != 0) return 15;
    result = workspace_sandbox_execute_action(&runtime, action_id, &selected, &pending_apply);
    if (result.code != CORE_OK || pending_apply != 0 || selected != 3u) return 16;
    result = workspace_sandbox_cycle_authoring_overlay(&runtime);
    if (result.code != CORE_OK || runtime.authoring_overlay_mode != WORKSPACE_SANDBOX_AUTHORING_OVERLAY_PANE) return 17;
    ws_stageg_trace("b3_actions", "overlay_gate", selected);

    result = workspace_sandbox_hit_test_splitter(&runtime,
                                                 (CorePaneRect){0.0f, 0.0f, 1200.0f, 800.0f},
                                                 10.0f,
                                                 420.0f,
                                                 100.0f,
                                                 &hit);
    if (result.code != CORE_OK || !hit.active) return 19;
    ws_stageg_trace("b3_splitter", "hit_resolved", hit.node_index);

    result = workspace_sandbox_set_splitter_ratio(&runtime, &hit, 1.0f);
    clamped_ratio = ws_stageg_quantize(runtime.draft_layout.nodes[hit.node_index].ratio_01);
    if (result.code != CORE_OK || clamped_ratio < 0 || clamped_ratio > 1000000) return 20;
    result = workspace_sandbox_set_splitter_ratio(&runtime, &hit, 0.5f);
    if (result.code != CORE_OK) return 21;
    ws_stageg_trace("b3_clamped", "edge_and_restore", (uint64_t)(uint32_t)clamped_ratio);

    result = workspace_sandbox_drag_splitter(&runtime, &hit, 120.0f, 0.0f);
    if (result.code != CORE_OK) return 22;
    result = workspace_sandbox_drag_splitter(&runtime, &hit, -60.0f, 0.0f);
    if (result.code != CORE_OK) return 23;
    result = workspace_sandbox_drag_splitter(&runtime, &hit, -1000.0f, 0.0f);
    if (result.code != CORE_OK) return 24;
    result = workspace_sandbox_drag_splitter(&runtime, &hit, 50.0f, 0.0f);
    if (result.code != CORE_OK) return 25;
    final_ratio = ws_stageg_quantize(runtime.draft_layout.nodes[hit.node_index].ratio_01);
    if (final_ratio < 0 || final_ratio > 1000000) return 26;
    ws_stageg_trace("b3_reversed", "delta_replay", (uint64_t)(uint32_t)final_ratio);

    result = workspace_sandbox_undo(&runtime);
    if (result.code != CORE_OK || !workspace_sandbox_can_redo(&runtime)) return 27;
    result = workspace_sandbox_redo(&runtime);
    if (result.code != CORE_OK) return 28;
    ws_stageg_trace("b3_history", "undo_redo", runtime.draft_undo_redo.cursor);

    result = workspace_sandbox_execute_action(&runtime, action_id, &selected, &pending_apply);
    if (result.code != CORE_OK || !pending_apply || selected == 3u) return 29;
    result = workspace_sandbox_resolve_trigger(&runtime, "5", &action_id);
    if (result.code != CORE_OK || strcmp(action_id, "workspace.assign_temp_metrics") != 0) return 30;
    result = workspace_sandbox_execute_action(&runtime, action_id, &selected, &pending_apply);
    if (result.code != CORE_OK) return 31;
    result = workspace_sandbox_get_module_for_pane(&runtime, selected, &instance);
    if (result.code != CORE_OK || instance.kind != WORKSPACE_SANDBOX_MODULE_TEMP_METRICS) return 32;
    ws_stageg_trace("b3_module", "routed_assignment", instance.instance_id);

    result = workspace_sandbox_apply_authoring(&runtime);
    if (result.code != CORE_OK || runtime.layout_state.mode != CORE_LAYOUT_MODE_RUNTIME) return 33;
    ws_stageg_trace("b3_applied", "interaction_committed", ws_stageg_semantic_hash(&runtime));

    snprintf(runtime.data_paths.input_root, sizeof(runtime.data_paths.input_root), "runtime/input");
    snprintf(runtime.data_paths.output_root, sizeof(runtime.data_paths.output_root), "runtime/output");
    snprintf(runtime.data_paths.selected_ingest_path,
             sizeof(runtime.data_paths.selected_ingest_path),
             "runtime/input/layout.pack");
    snprintf(runtime.data_paths.ingest_status, sizeof(runtime.data_paths.ingest_status), "stage-g-ready");
    result = workspace_sandbox_data_paths_save(&runtime, "runtime");
    if (result.code != CORE_OK) return 34;
    ws_stageg_trace("b3_paths_saved", "relative_runtime", 1u);

    result = workspace_sandbox_runtime_init(&paths_reload);
    if (result.code != CORE_OK) return 35;
    result = workspace_sandbox_data_paths_load(&paths_reload, "runtime");
    if (result.code != CORE_OK || strcmp(paths_reload.data_paths.input_root, "runtime/input") != 0 ||
        strcmp(paths_reload.data_paths.output_root, "runtime/output") != 0 ||
        strcmp(paths_reload.data_paths.selected_ingest_path, "runtime/input/layout.pack") != 0 ||
        strcmp(paths_reload.data_paths.ingest_status, "stage-g-ready") != 0) return 36;
    result = workspace_sandbox_data_paths_build_session_pack_path(&paths_reload, session_path, sizeof(session_path));
    if (result.code != CORE_OK) return 37;
    result = workspace_sandbox_data_paths_build_text_zoom_path(&paths_reload, text_zoom_path, sizeof(text_zoom_path));
    if (result.code != CORE_OK) return 38;
    result = workspace_sandbox_data_paths_build_font_theme_state_path(&paths_reload, font_theme_path, sizeof(font_theme_path));
    if (result.code != CORE_OK) return 39;
    ws_stageg_trace("b3_paths_loaded", "derived_paths", 3u);

    snprintf(paths_reload.data_paths.input_root,
             sizeof(paths_reload.data_paths.input_root),
             "/tmp/Fake.app/Contents/Resources/input");
    snprintf(paths_reload.data_paths.output_root,
             sizeof(paths_reload.data_paths.output_root),
             "/tmp/Fake.app/Contents/Resources/output");
    snprintf(paths_reload.data_paths.selected_ingest_path,
             sizeof(paths_reload.data_paths.selected_ingest_path),
             "/tmp/Fake.app/Contents/Resources/input/layout.pack");
    paths_reload.data_paths.ingest_status[0] = '\0';
    result = workspace_sandbox_data_paths_normalize(&paths_reload, "runtime");
    if (result.code != CORE_OK || strstr(paths_reload.data_paths.input_root, ".app/Contents/") ||
        strstr(paths_reload.data_paths.output_root, ".app/Contents/") ||
        paths_reload.data_paths.selected_ingest_path[0] != '\0' ||
        strcmp(paths_reload.data_paths.ingest_status, "path fallback applied") != 0) return 40;
    ws_stageg_trace("b3_normalized", "bundle_paths_rejected", 1u);

    canonical = fopen("interaction.canonical", "wb");
    if (!canonical) return 41;
    fprintf(canonical,
            "semantic_hash=%016" PRIx64 "\nratio=%d\ninput_root=runtime/input\noutput_root=runtime/output\n"
            "selected=runtime/input/layout.pack\nstatus=stage-g-ready\nsession=%s\nzoom=%s\nfont_theme=%s\n"
            "fallback_input=%s\nfallback_output=%s\n",
            ws_stageg_semantic_hash(&runtime),
            final_ratio,
            session_path,
            text_zoom_path,
            font_theme_path,
            paths_reload.data_paths.input_root,
            paths_reload.data_paths.output_root);
    if (fclose(canonical) != 0) return 42;
    ws_stageg_trace("b3_canonical", "artifact_written", ws_stageg_semantic_hash(&runtime));
    ws_stageg_trace("b3_shutdown", "interaction_complete", 0u);
    return 0;
}
