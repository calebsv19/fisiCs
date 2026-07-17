#include "workspace_sandbox_stage_g_common.h"
#include "workspace_sandbox/workspace_sandbox_app_main.h"

#include <fcntl.h>
#include <string.h>
#include <unistd.h>

typedef CoreResult (*WsStageGAppCall)(WorkspaceSandboxAppContext *ctx);

static CoreResult ws_stageg_capture_app_call(WsStageGAppCall call,
                                             WorkspaceSandboxAppContext *ctx) {
    int saved_stdout;
    int capture_fd;
    CoreResult result = {CORE_ERR_IO, "capture failed"};
    if (!call || !ctx) return (CoreResult){CORE_ERR_INVALID_ARG, "invalid capture call"};
    fflush(stdout);
    saved_stdout = dup(STDOUT_FILENO);
    capture_fd = open("lifecycle.log", O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (saved_stdout < 0 || capture_fd < 0) {
        if (saved_stdout >= 0) close(saved_stdout);
        if (capture_fd >= 0) close(capture_fd);
        return result;
    }
    if (dup2(capture_fd, STDOUT_FILENO) < 0) {
        close(capture_fd);
        close(saved_stdout);
        return result;
    }
    close(capture_fd);
    result = call(ctx);
    fflush(stdout);
    if (dup2(saved_stdout, STDOUT_FILENO) < 0) result = (CoreResult){CORE_ERR_IO, "restore failed"};
    close(saved_stdout);
    clearerr(stdout);
    return result;
}

int main(void) {
    WorkspaceSandboxAppContext app;
    WorkspaceSandboxAppContext reloaded;
    WorkspaceSandboxAppContext invalid;
    char *argv[] = {(char *)"workspace_sandbox", (char *)"--preset", (char *)"workflow.pack"};
    char *reload_argv[] = {
        (char *)"workspace_sandbox",
        (char *)"--preset",
        (char *)"workflow.pack",
        (char *)"--export-json",
        (char *)"workflow_reload.json"
    };
    char *invalid_argv[] = {
        (char *)"workspace_sandbox",
        (char *)"--help",
        (char *)"--preset",
        (char *)"ignored.pack"
    };
    CorePaneId selected = 2u;
    int pending_apply = 0;
    const char *action_id = 0;
    FILE *capture_reset;
    CoreResult result;

    capture_reset = fopen("lifecycle.log", "wb");
    if (!capture_reset || fclose(capture_reset) != 0) return 11;

    result = workspace_sandbox_app_bootstrap(&app, 3, argv);
    if (result.code != CORE_OK || app.mode != WORKSPACE_SANDBOX_APP_MODE_WA0_ROUNDTRIP ||
        strcmp(app.preset_path, "workflow.pack") != 0) return 12;
    result = workspace_sandbox_app_config_load(&app);
    if (result.code != CORE_OK) return 13;
    ws_stageg_trace("b4_bootstrap", "wa0_route", app.mode);

    result = workspace_sandbox_app_state_seed(&app);
    if (result.code != CORE_OK) return 14;
    result = workspace_sandbox_app_subsystems_init(&app);
    if (result.code != CORE_OK) return 15;
    ws_stageg_trace("b4_seeded", "runtime_ready", app.runtime.layout_state.active_revision);

    result = workspace_sandbox_enter_authoring_layout(&app.runtime);
    if (result.code != CORE_OK) return 16;
    result = workspace_sandbox_resolve_trigger(&app.runtime, "6", &action_id);
    if (result.code != CORE_OK || strcmp(action_id, "workspace.assign_temp_events") != 0) return 17;
    result = workspace_sandbox_execute_action(&app.runtime, action_id, &selected, &pending_apply);
    if (result.code != CORE_OK) return 18;
    result = workspace_sandbox_apply_authoring(&app.runtime);
    if (result.code != CORE_OK) return 19;
    ws_stageg_trace("b4_authoring", "module_applied", ws_stageg_semantic_hash(&app.runtime));

    result = ws_stageg_capture_app_call(workspace_sandbox_runtime_start, &app);
    if (result.code != CORE_OK || app.runtime.layout_state.mode != CORE_LAYOUT_MODE_RUNTIME) return 20;
    ws_stageg_trace("b4_started", "roundtrip_persisted", ws_stageg_semantic_hash(&app.runtime));

    result = ws_stageg_capture_app_call(workspace_sandbox_app_run_loop, &app);
    if (result.code != CORE_OK || app.leaf_count != 3u) return 21;
    ws_stageg_trace("b4_loop", "layout_solved", app.leaf_count);

    result = workspace_sandbox_export_active_snapshot_debug_json(&app.runtime, "workflow.json");
    if (result.code != CORE_OK) return 22;
    ws_stageg_trace("b4_exported", "snapshot_written", app.runtime.active_layout.node_count);

    result = workspace_sandbox_app_shutdown(&app);
    if (result.code != CORE_OK || app.runtime.layout_state.rebuild_required) return 23;
    ws_stageg_trace("b4_shutdown", "rebuild_acknowledged", app.runtime.layout_state.active_revision);

    result = workspace_sandbox_app_bootstrap(&reloaded, 5, reload_argv);
    if (result.code != CORE_OK || reloaded.mode != WORKSPACE_SANDBOX_APP_MODE_EXPORT_SNAPSHOT_JSON) return 24;
    result = workspace_sandbox_app_config_load(&reloaded);
    if (result.code != CORE_OK) return 25;
    result = workspace_sandbox_app_state_seed(&reloaded);
    if (result.code != CORE_OK) return 26;
    result = workspace_sandbox_app_subsystems_init(&reloaded);
    if (result.code != CORE_OK) return 27;
    result = ws_stageg_capture_app_call(workspace_sandbox_runtime_start, &reloaded);
    if (result.code != CORE_OK || !ws_stageg_semantic_equal(&app.runtime, &reloaded.runtime)) return 28;
    result = workspace_sandbox_app_shutdown(&reloaded);
    if (result.code != CORE_OK) return 29;
    ws_stageg_trace("b4_reloaded", "export_route_equal", ws_stageg_semantic_hash(&reloaded.runtime));

    result = workspace_sandbox_app_bootstrap(&invalid, 4, invalid_argv);
    if (result.code != CORE_ERR_INVALID_ARG) return 30;
    ws_stageg_trace("b4_invalid", "mixed_route_rejected", result.code);

    if (!ws_stageg_write_canonical(&reloaded.runtime, "workflow.canonical", "workspace_sandbox_bite4")) return 31;
    ws_stageg_trace("b4_canonical", "artifact_written", ws_stageg_semantic_hash(&reloaded.runtime));
    ws_stageg_trace("b4_complete", "workflow_closed", 0u);
    return 0;
}
