#include "app/datalab_app_internal.h"
#include "app/datalab_runtime_prefs.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void checkpoint(const char *name, const char *detail, long value) {
    printf("TRACE|1|%s|detail=%s|value=%ld|result=1\n",
           name,
           detail ? detail : "ok",
           value);
}

static int workflow_dispatch(const DatalabDispatchRequest *request,
                             DatalabDispatchOutcome *outcome) {
    DatalabAppState state;
    if (!request || !request->runtime || !outcome) return 1;
    memset(outcome, 0, sizeof(*outcome));
    datalab_runtime_copy_to_app_state(request->runtime, &state, 1);
    state.text_zoom_step = 3;
    state.workspace_authoring_theme_preset_id = DATALAB_WORKSPACE_AUTHORING_THEME_SOFT_LIGHT;
    datalab_workspace_authoring_begin_takeover(&state);
    datalab_workspace_authoring_cycle_overlay(&state);
    datalab_workspace_authoring_apply_takeover(&state);
    snprintf(state.input_root, sizeof(state.input_root), "library/workflow");
    state.recent_input_root_count = 1u;
    snprintf(state.recent_input_roots[0], sizeof(state.recent_input_roots[0]), "library/workflow");
    datalab_playback_set_mode(&state, DATALAB_PLAYBACK_MODE_BOUNCE);
    datalab_playback_set_speed_index(&state, 3, 100u);
    state.session_hud_collapsed = 1;
    datalab_runtime_copy_from_app_state(request->runtime, &state);
    outcome->exit_code = 0;
    outcome->dispatched = 1;
    outcome->runtime_started = 1;
    return 0;
}

static int state_seed_with_captured_summary(DatalabAppContext *ctx) {
    int saved_stdout = -1;
    int capture_fd = -1;
    int rc = 1;
    fflush(stdout);
    saved_stdout = dup(STDOUT_FILENO);
    capture_fd = open("lifecycle.summary.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (saved_stdout < 0 || capture_fd < 0 || dup2(capture_fd, STDOUT_FILENO) < 0) {
        if (capture_fd >= 0) close(capture_fd);
        if (saved_stdout >= 0) close(saved_stdout);
        return 1;
    }
    close(capture_fd);
    rc = datalab_app_state_seed_ctx(ctx);
    fflush(stdout);
    if (dup2(saved_stdout, STDOUT_FILENO) < 0) rc = 1;
    close(saved_stdout);
    clearerr(stdout);
    return rc;
}

int main(void) {
    const char *pack_path = getenv("DATALAB_STAGEG_PACK_INPUT");
    DatalabAppRuntime runtime;
    DatalabAppRuntime reloaded;
    DatalabAppContext ctx;
    DatalabAppContext reload_ctx;
    char *argv[4];
    char *reload_argv[1];
    FILE *canonical = NULL;
    int rc = 0;
    uint32_t frame_width = 0u, frame_height = 0u, frame_crc = 0u;
    uint64_t frame_index = 0u;

    if (!pack_path) return 11;
    argv[0] = (char *)"datalab";
    argv[1] = (char *)"--pack";
    argv[2] = (char *)pack_path;
    argv[3] = (char *)"--no-gui";
    reload_argv[0] = (char *)"datalab";

    datalab_app_runtime_init(&runtime);
    memset(&ctx, 0, sizeof(ctx));
    ctx.runtime = &runtime;
    ctx.stage = DATALAB_APP_STAGE_INIT;
    ctx.runtime_dispatch = workflow_dispatch;
    ctx.exit_code = 1;

    if (datalab_app_bootstrap_ctx(&ctx, 4, argv) != 0 ||
        ctx.stage != DATALAB_APP_STAGE_BOOTSTRAPPED || !runtime.no_gui) return 12;
    checkpoint("b4_bootstrap", "headless_configured", ctx.stage);

    if (datalab_app_config_load_ctx(&ctx) != 0 || ctx.stage != DATALAB_APP_STAGE_CONFIG_LOADED) return 13;
    checkpoint("b4_config", "preferences_loaded", ctx.stage);

    if (state_seed_with_captured_summary(&ctx) != 0 ||
        ctx.stage != DATALAB_APP_STAGE_STATE_SEEDED || !runtime.frame_loaded ||
        runtime.frame.profile != DATALAB_PROFILE_PHYSICS) return 14;
    frame_width = runtime.frame.width;
    frame_height = runtime.frame.height;
    frame_crc = runtime.frame.obstacle_mask_crc32;
    frame_index = runtime.frame.frame_index;
    checkpoint("b4_loaded", "physics_frame_seeded", (long)frame_index);

    rc = datalab_app_run_loop_ctx(&ctx);
    if (rc != 0 || ctx.stage != DATALAB_APP_STAGE_LOOP_COMPLETED ||
        ctx.dispatch_summary.dispatch_count != 1u || !ctx.dispatch_summary.dispatch_succeeded ||
        runtime.text_zoom_step != 3 || runtime.workspace_authoring_theme_preset_id != DATALAB_WORKSPACE_AUTHORING_THEME_SOFT_LIGHT ||
        strcmp(runtime.input_root, "library/workflow") != 0 ||
        runtime.playback_mode != DATALAB_PLAYBACK_MODE_BOUNCE || !runtime.session_hud_collapsed) return 15;
    checkpoint("b4_dispatched", "state_mutated", ctx.dispatch_summary.dispatch_count);

    if (!datalab_runtime_prefs_save_text_zoom_step(runtime.text_zoom_step) ||
        !datalab_runtime_prefs_save_input_root(runtime.input_root) ||
        !datalab_runtime_prefs_save_recent_input_roots(
            (const char (*)[DATALAB_APP_PATH_CAP])runtime.recent_input_roots,
            runtime.recent_input_root_count) ||
        !datalab_runtime_prefs_save_theme_preset_id(runtime.workspace_authoring_theme_preset_id)) return 16;
    checkpoint("b4_persisted", "runtime_preferences", runtime.text_zoom_step);

    canonical = fopen("workflow.canonical", "wb");
    if (!canonical) return 17;
    fprintf(canonical,
            "profile=%d\nwidth=%u\nheight=%u\nframe=%llu\ncrc=%u\n"
            "text_zoom=%d\ntheme=%u\ninput_root=%s\nrecent_roots=%zu\n"
            "playback_mode=%d\nspeed=%d\ninterval=%u\nhud_collapsed=%d\n"
            "dispatch_count=%u\ndispatch_exit=%d\n",
            (int)runtime.frame.profile,
            frame_width,
            frame_height,
            (unsigned long long)frame_index,
            frame_crc,
            runtime.text_zoom_step,
            (unsigned int)runtime.workspace_authoring_theme_preset_id,
            runtime.input_root,
            runtime.recent_input_root_count,
            (int)runtime.playback_mode,
            runtime.playback_speed_index,
            runtime.playback_interval_ms,
            runtime.session_hud_collapsed,
            ctx.dispatch_summary.dispatch_count,
            ctx.dispatch_summary.last_dispatch_exit_code);
    if (fclose(canonical) != 0) return 18;
    checkpoint("b4_canonical", "artifact_written", 1L);

    datalab_app_shutdown_ctx(&ctx);
    if (ctx.stage != DATALAB_APP_STAGE_SHUTDOWN_COMPLETED || runtime.frame_loaded ||
        !ctx.ownership.shutdown_owned || ctx.ownership.runtime_owned || ctx.ownership.state_seed_owned) return 19;
    checkpoint("b4_destroyed", "ownership_released", ctx.stage);

    datalab_app_runtime_init(&reloaded);
    memset(&reload_ctx, 0, sizeof(reload_ctx));
    reload_ctx.runtime = &reloaded;
    reload_ctx.stage = DATALAB_APP_STAGE_INIT;
    reload_ctx.runtime_dispatch = workflow_dispatch;
    if (datalab_app_bootstrap_ctx(&reload_ctx, 1, reload_argv) != 0 ||
        datalab_app_config_load_ctx(&reload_ctx) != 0 ||
        reloaded.text_zoom_step != 3 ||
        reloaded.workspace_authoring_theme_preset_id != DATALAB_WORKSPACE_AUTHORING_THEME_SOFT_LIGHT ||
        strcmp(reloaded.input_root, "library/workflow") != 0) return 20;
    checkpoint("b4_reloaded", "persisted_state_restored", reloaded.text_zoom_step);
    datalab_app_shutdown_ctx(&reload_ctx);
    checkpoint("b4_shutdown", "workflow_complete", reload_ctx.stage);
    return 0;
}
