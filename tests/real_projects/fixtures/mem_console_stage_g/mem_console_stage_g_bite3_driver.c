#include "mem_console_stage_g_common.h"

#include "mem_console_layout_config.h"
#include "mem_console_pane_layout.h"
#include "mem_console_ui_graph.h"

static long milli(float value) {
    return (long)(value * 1000.0f + (value >= 0.0f ? 0.5f : -0.5f));
}

int main(void) {
    const uint64_t basis = UINT64_C(1469598103934665603);
    const MemConsoleLayoutConfig *cfg = mem_console_layout_config_get();
    MemConsoleState state;
    KitRenderRect splitter;
    KitRenderRect graph_bounds = { 400.0f, 220.0f, 720.0f, 420.0f };
    KitUiInputState input;
    CoreResult result;
    uint64_t digest = basis;
    char detail[640];
    int suppression;

    memset(&state, 0, sizeof(state));
    state.graph_mode_enabled = 1;
    state.pane_left_ratio = 0.30f;
    state.pane_right_split_ratio = 0.58f;
    state.pane_detail_split_ratio = 0.40f;
    state.pane_detail_top_split_ratio = 0.55f;
    result = mem_console_pane_layout_compute(&state, cfg, 1440, 900);
    if (result.code != CORE_OK) return 30;
    snprintf(detail, sizeof(detail), "left=%ld,right=%ld,detail=%ld,graph=%ld",
             milli(state.left_pane.width), milli(state.right_pane.width),
             milli(state.pane_right_detail.height), milli(state.pane_right_graph.height));
    digest = mc_stageg_hash_text(digest, detail);
    mc_stageg_trace("b3_layout", detail, digest);

    result = mem_console_pane_layout_get_splitter_bounds(
        &state, cfg, MEM_CONSOLE_PANE_SPLITTER_LEFT_RIGHT, &splitter);
    if (result.code != CORE_OK ||
        !mem_console_pane_layout_begin_drag(&state, cfg, 1440, 900,
                                            splitter.x + splitter.width * 0.5f,
                                            splitter.y + splitter.height * 0.5f)) return 31;
    snprintf(detail, sizeof(detail), "active=%d,splitter=%d,start=%ld",
             state.pane_drag_active, state.pane_drag_splitter_id,
             milli(state.pane_left_ratio));
    digest = mc_stageg_hash_text(digest, detail);
    mc_stageg_trace("b3_drag_begin", detail, digest);

    if (!mem_console_pane_layout_update_drag(&state, cfg, 1440, 900,
                                             splitter.x + 180.0f, splitter.y + 2.0f)) return 32;
    snprintf(detail, sizeof(detail), "left_ratio=%ld,dirty=%d",
             milli(state.pane_left_ratio), state.pane_prefs_dirty);
    digest = mc_stageg_hash_text(digest, detail);
    mc_stageg_trace("b3_drag_update", detail, digest);

    if (!mem_console_pane_layout_update_drag(&state, cfg, 1440, 900,
                                             splitter.x - 900.0f, splitter.y + 2.0f)) return 33;
    mem_console_pane_layout_end_drag(&state);
    snprintf(detail, sizeof(detail), "left_ratio=%ld,active=%d,clamped=1",
             milli(state.pane_left_ratio), state.pane_drag_active);
    digest = mc_stageg_hash_text(digest, detail);
    mc_stageg_trace("b3_drag_reverse", detail, digest);

    state.graph_viewport.zoom = 1.0f;
    memset(&input, 0, sizeof(input));
    input.mouse_x = 760.0f;
    input.mouse_y = 420.0f;
    (void)mem_console_ui_graph_handle_viewport_interaction(&state, &input, 2, graph_bounds);
    snprintf(detail, sizeof(detail), "pan=%ld,%ld,zoom=%ld,cache=%d",
             milli(state.graph_viewport.pan_x), milli(state.graph_viewport.pan_y),
             milli(state.graph_viewport.zoom), state.graph_layout_valid);
    digest = mc_stageg_hash_text(digest, detail);
    mc_stageg_trace("b3_graph_zoom", detail, digest);

    memset(&input, 0, sizeof(input));
    input.mouse_x = 700.0f; input.mouse_y = 400.0f; input.mouse_pressed = 1; input.mouse_down = 1;
    (void)mem_console_ui_graph_handle_viewport_interaction(&state, &input, 0, graph_bounds);
    input.mouse_pressed = 0; input.mouse_x = 745.0f; input.mouse_y = 365.0f;
    (void)mem_console_ui_graph_handle_viewport_interaction(&state, &input, 0, graph_bounds);
    input.mouse_down = 0; input.mouse_released = 1;
    suppression = mem_console_ui_graph_handle_viewport_interaction(&state, &input, 0, graph_bounds);
    snprintf(detail, sizeof(detail), "pan=%ld,%ld,suppress=%d,armed=%d",
             milli(state.graph_viewport.pan_x), milli(state.graph_viewport.pan_y),
             suppression, state.graph_click_armed);
    digest = mc_stageg_hash_text(digest, detail);
    mc_stageg_trace("b3_graph_pan", detail, digest);

    memset(&input, 0, sizeof(input));
    input.mouse_x = -100.0f; input.mouse_y = -100.0f; input.mouse_pressed = 1; input.mouse_down = 1;
    if (mem_console_ui_graph_handle_viewport_interaction(&state, &input, 0, graph_bounds) != 0 ||
        state.graph_drag_active != 0 ||
        mem_console_pane_layout_begin_drag(&state, cfg, 1440, 900, -50.0f, -50.0f) != 0) return 34;
    digest = mc_stageg_hash_text(digest, "outside_rejected=1");
    mc_stageg_trace("b3_invalid", "outside_rejected=1", digest);

    snprintf(detail, sizeof(detail),
             "schema=mem_console_stage_g_bite3_v1\ndigest=%016llx\nleft_ratio=%ld\nzoom=%ld\n",
             (unsigned long long)digest, milli(state.pane_left_ratio),
             milli(state.graph_viewport.zoom));
    if (!mc_stageg_write_text("interaction.canonical", detail)) return 35;
    mc_stageg_trace("b3_canonical", "artifact=interaction.canonical", digest);
    mc_stageg_trace("b3_shutdown", "interaction_cleared=1", digest);
    return 0;
}
