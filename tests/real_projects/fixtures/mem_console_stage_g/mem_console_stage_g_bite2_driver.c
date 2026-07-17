#include "mem_console_stage_g_common.h"

#include "mem_console_prefs.h"

#include <errno.h>
#include <sys/stat.h>

static int make_runtime_dir(void) {
    return mkdir("runtime", 0755) == 0 || errno == EEXIST;
}

static long milli(float value) {
    return (long)(value * 1000.0f + (value >= 0.0f ? 0.5f : -0.5f));
}

static void format_preference_semantics(const MemConsoleState *state,
                                        char *out_text,
                                        size_t out_cap) {
    snprintf(out_text, out_cap,
             "theme=%d,font=%d,text_zoom=%d,ratios=%ld,%ld,%ld,%ld,%ld,"
             "graph=%d,%d,%d,%d,%d,%d,viewport=%ld,%ld,%ld",
             (int)state->theme_preset_id, (int)state->font_preset_id,
             state->text_zoom_step, milli(state->pane_left_ratio),
             milli(state->pane_right_split_ratio),
             milli(state->pane_detail_split_ratio),
             milli(state->pane_detail_top_split_ratio),
             milli(state->left_panel_top_ratio), state->graph_mode_enabled,
             state->graph_query_edge_limit, state->graph_query_hops,
             state->graph_layout_mode, state->graph_sort_mode,
             state->graph_scope_full_mode_enabled,
             milli(state->graph_viewport.pan_x),
             milli(state->graph_viewport.pan_y),
             milli(state->graph_viewport.zoom));
}

int main(void) {
    const uint64_t basis = UINT64_C(1469598103934665603);
    MemConsoleState state;
    MemConsoleState loaded;
    CoreResult result;
    uint64_t digest = basis;
    char detail[768];
    char before_semantics[768];
    char after_semantics[768];
    char db_path[1024];
    char input_root[1024];
    char output_root[1024];
    char active_db_path[1024];

    if (!make_runtime_dir()) return 20;
    memset(&state, 0, sizeof(state));
    state.theme_preset_id = (CoreThemePresetId)0;
    state.font_preset_id = (CoreFontPresetId)0;
    state.text_zoom_step = 3;
    state.pane_left_ratio = 0.31f;
    state.pane_right_split_ratio = 0.62f;
    state.pane_detail_split_ratio = 0.41f;
    state.pane_detail_top_split_ratio = 0.57f;
    state.left_panel_top_ratio = 0.44f;
    state.graph_mode_enabled = 1;
    state.graph_query_edge_limit = 256;
    state.graph_query_hops = 4;
    state.graph_layout_mode = MEM_CONSOLE_GRAPH_LAYOUT_TREE;
    state.graph_sort_mode = MEM_CONSOLE_GRAPH_SORT_OLDEST_FIRST;
    state.graph_scope_full_mode_enabled = 1;
    state.graph_viewport.pan_x = 11.5f;
    state.graph_viewport.pan_y = -7.25f;
    state.graph_viewport.zoom = 1.75f;
    format_preference_semantics(&state, before_semantics, sizeof(before_semantics));
    snprintf(detail, sizeof(detail), "%s", before_semantics);
    digest = mc_stageg_hash_text(digest, detail);
    mc_stageg_trace("b2_prepared", detail, digest);

    result = mem_console_prefs_save("runtime/ui.pack", &state);
    if (result.code != CORE_OK) return 21;
    result = mem_console_app_prefs_save("runtime/app.pack", "runtime/workflow.sqlite",
                                        "runtime/input", "runtime/output",
                                        "runtime/workflow.sqlite");
    if (result.code != CORE_OK) return 22;
    digest = mc_stageg_hash_text(digest, "saved=ui.pack,app.pack");
    mc_stageg_trace("b2_saved", "artifacts=ui.pack,app.pack", digest);

    memset(&state, 0, sizeof(state));
    mc_stageg_trace("b2_destroyed", "state_zeroed=1", digest);

    memset(&loaded, 0, sizeof(loaded));
    result = mem_console_prefs_load("runtime/ui.pack", &loaded);
    if (result.code != CORE_OK) return 23;
    result = mem_console_app_prefs_load("runtime/app.pack", db_path, sizeof(db_path),
                                        input_root, sizeof(input_root), output_root,
                                        sizeof(output_root), active_db_path,
                                        sizeof(active_db_path));
    if (result.code != CORE_OK) return 24;
    format_preference_semantics(&loaded, after_semantics, sizeof(after_semantics));
    snprintf(detail, sizeof(detail), "state=%s,db=%s,input=%s,output=%s",
             after_semantics, db_path, input_root, output_root);
    digest = mc_stageg_hash_text(digest, detail);
    mc_stageg_trace("b2_reloaded", detail, digest);

    if (strcmp(before_semantics, after_semantics) != 0 ||
        strcmp(db_path, "runtime/workflow.sqlite") != 0 ||
        strcmp(input_root, "runtime/input") != 0 ||
        strcmp(output_root, "runtime/output") != 0 ||
        strcmp(active_db_path, "runtime/workflow.sqlite") != 0) return 25;
    snprintf(detail, sizeof(detail), "match=1,zoom=%.3f,hops=%d,edge_limit=%d",
             loaded.graph_viewport.zoom, loaded.graph_query_hops,
             loaded.graph_query_edge_limit);
    digest = mc_stageg_hash_text(digest, detail);
    mc_stageg_trace("b2_compared", detail, digest);

    snprintf(detail, sizeof(detail),
             "schema=mem_console_stage_g_bite2_v1\ndigest=%016llx\nstate=%s\n",
             (unsigned long long)digest, after_semantics);
    if (!mc_stageg_write_text("persistence.canonical", detail)) return 26;
    mc_stageg_trace("b2_canonical", "artifact=persistence.canonical", digest);
    mc_stageg_trace("b2_shutdown", "loaded_state_destroyed=1", digest);
    return 0;
}
