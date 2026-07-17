#include "app/app_state.h"
#include "render/render_view_internal.h"

#include <stdio.h>
#include <string.h>

static void checkpoint(const char *name, const char *detail, long value) {
    printf("TRACE|1|%s|detail=%s|value=%ld|result=1\n",
           name,
           detail ? detail : "ok",
           value);
}

static void set_file(DatalabPackPanelCache *cache, size_t index, const char *name) {
    if (cache && index < DATALAB_PANEL_MAX_FILES && name) {
        snprintf(cache->files[index], sizeof(cache->files[index]), "%s", name);
    }
}

int main(void) {
    DatalabAppState state;
    DatalabPackPanelCache cache;
    char requested[DATALAB_APP_PATH_CAP] = {0};
    char joined[DATALAB_APP_PATH_CAP] = {0};
    FILE *canonical = NULL;
    float fit_zoom = 0.0f;

    datalab_app_state_init(&state, "library/beta.pack", DATALAB_PROFILE_IMAGE);
    state.text_zoom_step = 2;
    state.workspace_authoring_theme_preset_id = DATALAB_WORKSPACE_AUTHORING_THEME_SOFT_LIGHT;
    datalab_workspace_authoring_begin_takeover(&state);
    if (!state.workspace_authoring_stub_active || state.workspace_authoring_entry_count != 1u) return 11;
    checkpoint("b3_authoring_begin", "snapshot_captured", state.text_zoom_step);

    datalab_workspace_authoring_cycle_overlay(&state);
    state.text_zoom_step = 5;
    state.workspace_authoring_theme_preset_id = DATALAB_WORKSPACE_AUTHORING_THEME_GREYSCALE;
    datalab_workspace_authoring_apply_takeover(&state);
    if (state.workspace_authoring_pending_stub || state.workspace_authoring_apply_count != 1u ||
        state.workspace_authoring_entry_text_zoom_step != 5) return 12;
    checkpoint("b3_authoring_apply", "baseline_advanced", state.workspace_authoring_apply_count);

    state.text_zoom_step = -3;
    state.workspace_authoring_theme_preset_id = DATALAB_WORKSPACE_AUTHORING_THEME_DAW_DEFAULT;
    state.workspace_authoring_pending_stub = 1u;
    if (!datalab_workspace_authoring_cancel_and_exit(&state) || state.text_zoom_step != 5 ||
        state.workspace_authoring_theme_preset_id != DATALAB_WORKSPACE_AUTHORING_THEME_GREYSCALE ||
        state.workspace_authoring_stub_active) return 13;
    checkpoint("b3_authoring_cancel", "applied_state_restored", state.workspace_authoring_cancel_count);

    datalab_raster_viewport_sync_state(&state.raster_viewport, 800, 600, 400u, 200u);
    if (!state.raster_viewport.valid || !state.raster_viewport.fit_mode) return 14;
    fit_zoom = state.raster_viewport.viewport.zoom;
    checkpoint("b3_viewport_fit", "fit_initialized", (long)(fit_zoom * 1000.0f));

    if (!datalab_raster_viewport_zoom_at_screen_anchor(&state.raster_viewport, 400, 300, 1.5f) ||
        !datalab_raster_viewport_begin_drag(&state.raster_viewport, 400, 300) ||
        !datalab_raster_viewport_drag_to(&state.raster_viewport, 460, 270)) return 15;
    datalab_raster_viewport_end_drag(&state.raster_viewport);
    datalab_raster_viewport_sync_state(&state.raster_viewport, 1024, 768, 400u, 200u);
    if (state.raster_viewport.fit_mode || state.raster_viewport.drag_active ||
        state.raster_viewport.view_width != 1024 || state.raster_viewport.view_height != 768) return 16;
    checkpoint("b3_viewport_free", "zoom_drag_resize", (long)(state.raster_viewport.viewport.zoom * 1000.0f));

    datalab_raster_viewport_request_reset(&state.raster_viewport);
    datalab_raster_viewport_sync_state(&state.raster_viewport, 1024, 768, 512u, 256u);
    if (!state.raster_viewport.fit_mode || state.raster_viewport.reset_requested ||
        state.raster_viewport.content_width != 512u) return 17;
    checkpoint("b3_viewport_reset", "content_change_refit", (long)(state.raster_viewport.viewport.zoom * 1000.0f));

    memset(&cache, 0, sizeof(cache));
    cache.file_count = 3u;
    set_file(&cache, 0u, "alpha.pack");
    set_file(&cache, 1u, "beta.pack");
    set_file(&cache, 2u, "gamma.pack");
    datalab_panel_apply_state(&state, &cache, "library", 1, 10u);
    if (state.panel_selected_index != 1u) return 18;
    state.panel_selection_delta = 1;
    state.panel_open_selected_requested = 1;
    datalab_panel_apply_state(&state, &cache, "library", 0, 20u);
    if (state.panel_selected_index != 2u ||
        !datalab_panel_consume_requested_pack_path(&state, requested, sizeof(requested)) ||
        strcmp(requested, "library/gamma.pack") != 0) return 19;
    checkpoint("b3_panel_select", "gamma_requested", (long)state.panel_selected_index);

    state.playback_active = 1;
    state.playback_mode = DATALAB_PLAYBACK_MODE_BOUNCE;
    state.playback_direction = 1;
    state.playback_interval_ms = 100u;
    state.playback_last_advance_ticks = 0u;
    state.panel_selected_index = 2u;
    datalab_panel_apply_state(&state, &cache, "library", 0, 100u);
    if (state.panel_selected_index != 1u || state.playback_direction != -1 ||
        strcmp(state.panel_requested_pack_path, "library/beta.pack") != 0) return 20;
    checkpoint("b3_playback", "bounce_reversed", state.playback_direction);

    if (datalab_input_root_join_child_file("library", "../escape.pack", joined, sizeof(joined)) ||
        datalab_panel_request_pack_under_root(&state, "library", "nested/escape.pack")) return 21;
    checkpoint("b3_invalid", "non_child_rejected", 1L);

    canonical = fopen("interaction.canonical", "wb");
    if (!canonical) return 22;
    fprintf(canonical,
            "text_zoom=%d\ntheme=%u\nauthoring_entries=%u\napply=%u\ncancel=%u\n"
            "overlay_cycles=%u\nviewport=%dx%d\ncontent=%ux%u\nzoom=%.6f\npan=%.6f,%.6f\n"
            "panel_index=%zu\nplayback_mode=%d\ndirection=%d\nrequested=%s\n",
            state.text_zoom_step,
            (unsigned int)state.workspace_authoring_theme_preset_id,
            state.workspace_authoring_entry_count,
            state.workspace_authoring_apply_count,
            state.workspace_authoring_cancel_count,
            state.workspace_authoring_overlay_cycle_count,
            state.raster_viewport.view_width,
            state.raster_viewport.view_height,
            state.raster_viewport.content_width,
            state.raster_viewport.content_height,
            (double)state.raster_viewport.viewport.zoom,
            (double)state.raster_viewport.viewport.pan_x,
            (double)state.raster_viewport.viewport.pan_y,
            state.panel_selected_index,
            (int)state.playback_mode,
            state.playback_direction,
            state.panel_requested_pack_path);
    if (fclose(canonical) != 0) return 23;
    checkpoint("b3_canonical", "artifact_written", 1L);
    checkpoint("b3_shutdown", "state_complete", 0L);
    return 0;
}
