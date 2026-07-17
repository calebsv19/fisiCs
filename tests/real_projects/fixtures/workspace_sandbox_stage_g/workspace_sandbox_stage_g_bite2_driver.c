#include "workspace_sandbox_stage_g_common.h"

#include <stdlib.h>

int main(void) {
    const char *invalid_path = getenv("WORKSPACE_SANDBOX_STAGEG_INVALID_SNAPSHOT");
    WorkspaceSandboxRuntime source;
    WorkspaceSandboxRuntime preset_reload;
    WorkspaceSandboxRuntime json_reload;
    CorePaneId new_pane = 0u;
    uint64_t source_hash;
    uint64_t before_invalid_hash;
    CoreResult result;

    if (!invalid_path || !invalid_path[0]) return 11;
    result = workspace_sandbox_runtime_init(&source);
    if (result.code != CORE_OK) return 12;
    result = workspace_sandbox_enter_authoring_layout(&source);
    if (result.code != CORE_OK) return 13;
    result = workspace_sandbox_split_pane(&source, 1u, CORE_PANE_AXIS_HORIZONTAL, 0.33f, &new_pane);
    if (result.code != CORE_OK || new_pane == 0u) return 14;
    result = workspace_sandbox_assign_module(&source, 1u, WORKSPACE_SANDBOX_MODULE_TEMP_METRICS, 0);
    if (result.code != CORE_OK) return 15;
    result = workspace_sandbox_assign_module(&source, new_pane, WORKSPACE_SANDBOX_MODULE_TEMP_EVENTS, 0);
    if (result.code != CORE_OK) return 16;
    result = workspace_sandbox_set_root_ratio(&source, 0.61f);
    if (result.code != CORE_OK) return 17;
    result = workspace_sandbox_apply_authoring(&source);
    if (result.code != CORE_OK) return 18;
    source_hash = ws_stageg_semantic_hash(&source);
    ws_stageg_trace("b2_prepared", "layout_modules_applied", source_hash);

    result = workspace_sandbox_preset_save(&source, "workspace.pack");
    if (result.code != CORE_OK) return 19;
    result = workspace_sandbox_export_active_snapshot_debug_json(&source, "snapshot.json");
    if (result.code != CORE_OK) return 20;
    ws_stageg_trace("b2_saved", "preset_and_snapshot", source.active_layout.node_count);

    result = workspace_sandbox_runtime_init(&preset_reload);
    if (result.code != CORE_OK) return 21;
    result = workspace_sandbox_preset_load(&preset_reload, "workspace.pack");
    if (result.code != CORE_OK || !ws_stageg_semantic_equal(&source, &preset_reload)) return 22;
    ws_stageg_trace("b2_preset_reloaded", "semantic_equal", ws_stageg_semantic_hash(&preset_reload));

    result = workspace_sandbox_runtime_init(&json_reload);
    if (result.code != CORE_OK) return 23;
    result = workspace_sandbox_import_snapshot_json(&json_reload, "snapshot.json");
    if (result.code != CORE_OK) return 24;
    if (!ws_stageg_semantic_equal(&source, &json_reload)) return 27;
    ws_stageg_trace("b2_snapshot_reloaded", "semantic_equal", ws_stageg_semantic_hash(&json_reload));

    before_invalid_hash = ws_stageg_semantic_hash(&json_reload);
    result = workspace_sandbox_import_snapshot_json(&json_reload, invalid_path);
    if (result.code == CORE_OK || ws_stageg_semantic_hash(&json_reload) != before_invalid_hash) return 25;
    ws_stageg_trace("b2_invalid", "malformed_rejected", (uint64_t)result.code);

    if (!ws_stageg_write_canonical(&json_reload, "persistence.canonical", "workspace_sandbox_bite2")) return 26;
    ws_stageg_trace("b2_canonical", "artifact_written", ws_stageg_semantic_hash(&json_reload));
    ws_stageg_trace("b2_shutdown", "fresh_states_complete", 0u);
    return 0;
}
