#include "ide_stage_g_common.h"

#include "ide/Panes/ControlPanel/control_panel_internal.h"
#include "ide/Panes/Editor/editor_state.h"
#include "ide/Panes/Editor/editor_view.h"
#include "ide/UI/Trees/tree_row_metrics.h"
#include "engine/Render/render_font.h"

#include <math.h>

static ControlPanelControllerState g_control_state;
static int g_visible_refreshes = 0;
static int g_projection_syncs = 0;
static int g_persist_requests = 0;

ControlPanelControllerState* control_panel_state(void) {
    return &g_control_state;
}

void control_panel_mark_visible_tree_dirty(void) {
    g_control_state.tree.visible_tree_dirty = true;
}

void control_panel_refresh_visible_symbol_tree(void) {
    g_visible_refreshes++;
}

void editor_sync_active_file_projection_mode(void) {
    g_projection_syncs++;
}

static bool persist_request(void) {
    g_persist_requests++;
    return true;
}

int getUIFontPointSizeByTier(CoreFontTextSizeTier tier) {
    (void)tier;
    return 11;
}

bool isEditorDraggingScrollbar(void) {
    return false;
}

static int near_float(float actual, float expected) {
    return fabsf(actual - expected) < 0.01f;
}

int main(void) {
    char fields[256];
    char canonical[768];
    memset(&g_control_state, 0, sizeof(g_control_state));
    g_control_state.filters.target_symbols_enabled = true;
    g_control_state.filters.target_units_enabled = true;
    g_control_state.filters.search_scope = CONTROL_SEARCH_SCOPE_PROJECT_FILES;
    g_control_state.filters.editor_view_mode = CONTROL_EDITOR_VIEW_PROJECTION;
    strcpy(g_control_state.ui.search_query, "energy");
    g_control_state.ui.search_cursor = 6;
    g_control_state.ui.search_focused = true;
    control_panel_set_persist_request_callback(persist_request);
    ide_g_trace("b3_begin", "query=energy|scope=project|mode=projection|targets=2");

    ide_g_expect(control_panel_focus_unit_marker_query("velocity_x"), "focus unit marker");
    ControlPanelProjectionOptions projection;
    control_panel_capture_projection_options(&projection);
    ide_g_expect(projection.query_active && projection.marker_render_enabled,
                 "marker projection active");
    ide_g_expect(projection.target_editor_enabled && projection.target_units_enabled,
                 "marker targets active");
    snprintf(fields, sizeof(fields), "query=%s|scope=%d|mode=%d|units=%d|editor=%d|syncs=%d",
             projection.query, (int)projection.search_scope, (int)projection.editor_view_mode,
             projection.target_units_enabled ? 1 : 0,
             projection.target_editor_enabled ? 1 : 0, g_projection_syncs);
    ide_g_trace("b3_control_focus", fields);

    control_panel_set_target_units_enabled(false);
    control_panel_set_search_scope(CONTROL_SEARCH_SCOPE_ACTIVE_FILE);
    ControlPanelControllerState restored;
    memset(&restored, 0, sizeof(restored));
    control_panel_copy_startup_persist_state(&restored, &g_control_state);
    ide_g_expect(restored.tree.visible_tree_dirty, "restored tree marked dirty");
    snprintf(fields, sizeof(fields), "units=%d|scope=%d|refreshes=%d|syncs=%d|persists=%d",
             restored.filters.target_units_enabled ? 1 : 0,
             (int)restored.filters.search_scope, g_visible_refreshes,
             g_projection_syncs, g_persist_requests);
    ide_g_trace("b3_control_transfer", fields);

    EditorState editor;
    resetEditorState(&editor);
    editorStateFrameLineInUpperBand(&editor, 80, 300, 200);
    ide_g_expect(near_float(editor.scrollOffsetPx, 1141.0f), "frame line forward");
    snprintf(fields, sizeof(fields), "line=80|offset=%.0f|top=%d|target=%.0f",
             editor.scrollOffsetPx, editor.viewTopRow, editor.scrollTargetPx);
    ide_g_trace("b3_move_forward", fields);

    editorStateFrameLineInUpperBand(&editor, 2, 300, 200);
    ide_g_expect(near_float(editor.scrollOffsetPx, 0.0f), "frame line reverse clamp");
    snprintf(fields, sizeof(fields), "line=2|offset=%.0f|top=%d|clamped=1",
             editor.scrollOffsetPx, editor.viewTopRow);
    ide_g_trace("b3_move_reverse", fields);

    editorStateFrameLineInUpperBand(&editor, 198, 300, 200);
    float max_offset = editor_max_scroll_offset_px(&editor, 200, 300);
    ide_g_expect(near_float(editor.scrollOffsetPx, max_offset), "frame bottom clamp");
    snprintf(fields, sizeof(fields), "line=198|offset=%.0f|max=%.0f|top=%d",
             editor.scrollOffsetPx, max_offset, editor.viewTopRow);
    ide_g_trace("b3_resize_clamp", fields);

    UITreeRowMetrics metrics;
    ide_g_expect(ui_tree_row_metrics_compute(&metrics, 100, 12, 2, 14, 40, 18, 80, 14, 28),
                 "tree metrics");
    ide_g_expect(ui_tree_row_metrics_contains_text(&metrics, 134, 41), "text lower bound");
    ide_g_expect(ui_tree_row_metrics_contains_text(&metrics, 225, 56), "text upper bound");
    ide_g_expect(!ui_tree_row_metrics_contains_text(&metrics, 226, 56), "text outside bound");
    snprintf(fields, sizeof(fields), "draw_x=%d|text=%d,%d,%d,%d|prefix=%d",
             metrics.draw_x, metrics.text_bounds.x, metrics.text_bounds.y,
             metrics.text_bounds.w, metrics.text_bounds.h,
             ui_tree_row_metrics_contains_prefix(&metrics, 177, 56) ? 1 : 0);
    ide_g_trace("b3_coordinate", fields);

    ide_g_expect(!control_panel_focus_unit_marker_query(NULL), "reject null query");
    ide_g_expect(!control_panel_focus_unit_marker_query(""), "reject empty query");
    ide_g_expect(!ui_tree_row_metrics_compute(NULL, 0, 0, 0, 0, 0, 18, 0, 0, 0),
                 "reject null metrics");
    ide_g_trace("b3_invalid", "null_query=0|empty_query=0|null_metrics=0|state_preserved=1");

    snprintf(canonical, sizeof(canonical),
             "version=1\nquery=velocity_x\nscope=%d\nmode=%d\nunits=%d\nrefreshes=%d\nsyncs=%d\npersists=%d\nscroll_offset=%.0f\nscroll_max=%.0f\nrow_top=%d\ntext_bounds=%d,%d,%d,%d\n",
             (int)restored.filters.search_scope, (int)restored.filters.editor_view_mode,
             restored.filters.target_units_enabled ? 1 : 0, g_visible_refreshes,
             g_projection_syncs, g_persist_requests, editor.scrollOffsetPx,
             max_offset, editor.viewTopRow, metrics.text_bounds.x, metrics.text_bounds.y,
             metrics.text_bounds.w, metrics.text_bounds.h);
    ide_g_write_text("interaction.canonical", canonical);
    ide_g_trace("b3_canonical", "artifact=interaction.canonical|version=1");
    ide_g_trace("b3_shutdown", "controller_copied=1|editor_released=1");
    return 0;
}
