#include "mem_console_stage_g_common.h"

#include "mem_console_state.h"

int main(void) {
    const uint64_t basis = UINT64_C(1469598103934665603);
    MemConsoleState state;
    uint64_t digest = basis;
    char detail[512];
    int hidden = 0;

    memset(&state, 0, sizeof(state));
    state.db_path = state.db_path_storage;
    snprintf(state.db_path_storage, sizeof(state.db_path_storage), "%s", "fixture.sqlite");
    state.graph_query_edge_limit = MEM_CONSOLE_GRAPH_EDGE_LIMIT_DEFAULT;
    state.graph_query_hops = 2;
    snprintf(detail, sizeof(detail), "db=fixture.sqlite,edge_limit=%d,hops=%d",
             state.graph_query_edge_limit, state.graph_query_hops);
    digest = mc_stageg_hash_text(digest, detail);
    mc_stageg_trace("b1_seeded", detail, digest);

    mem_console_graph_kind_select_all(&state);
    if (!mem_console_graph_kind_toggle_enabled(&state, "supports")) return 10;
    mem_console_graph_kind_set_single(&state, "depends_on");
    mem_console_graph_edge_limit_set(&state, 99999);
    state.graph_query_hops = mem_console_graph_hops_clamp(0);
    snprintf(detail, sizeof(detail), "mask=%u,depends=%d,supports=%d,edge_limit=%d,hops=%d",
             state.graph_kind_filter_mask,
             mem_console_graph_kind_is_enabled(&state, "depends_on"),
             mem_console_graph_kind_is_enabled(&state, "supports"),
             state.graph_query_edge_limit, state.graph_query_hops);
    digest = mc_stageg_hash_text(digest, detail);
    mc_stageg_trace("b1_edge_filters", detail, digest);

    mem_console_graph_node_kind_select_all(&state);
    if (!mem_console_graph_node_kind_toggle_enabled(&state, "issue")) return 11;
    if (!mem_console_graph_anchor_hidden_set(&state, 42, 1)) return 12;
    if (!mem_console_graph_anchor_hidden_toggle(&state, 42, &hidden)) return 13;
    snprintf(detail, sizeof(detail), "node_mask=%u,issue=%d,hidden_count=%d,hidden=%d",
             state.graph_node_kind_filter_mask,
             mem_console_graph_node_kind_is_enabled(&state, "issue"),
             state.graph_hidden_anchor_count, hidden);
    digest = mc_stageg_hash_text(digest, detail);
    mc_stageg_trace("b1_node_filters", detail, digest);

    if (!mem_console_project_filter_toggle(&state, "fisiCs") ||
        !mem_console_project_filter_toggle(&state, "mem_console")) return 14;
    state.project_filter_option_count = 1;
    snprintf(state.project_filter_keys[0], sizeof(state.project_filter_keys[0]), "%s", "mem_console");
    mem_console_project_filter_prune_to_options(&state);
    snprintf(detail, sizeof(detail), "selected=%d,key=%s",
             state.selected_project_count, state.selected_project_keys[0]);
    digest = mc_stageg_hash_text(digest, detail);
    mc_stageg_trace("b1_project_scope", detail, digest);

    if (!mem_console_graph_view_mode_set(&state, MEM_CONSOLE_GRAPH_VIEW_WEB)) return 15;
    state.selected_item_id = 101;
    state.graph_center_item_id = 202;
    snprintf(detail, sizeof(detail), "view=%d,pan=%.3f,%.3f,zoom=%.3f,selected=%lld,center=%lld",
             mem_console_graph_view_mode_get(&state), state.graph_viewport.pan_x,
             state.graph_viewport.pan_y, state.graph_viewport.zoom,
             (long long)state.selected_item_id, (long long)state.graph_center_item_id);
    digest = mc_stageg_hash_text(digest, detail);
    mc_stageg_trace("b1_view_selection", detail, digest);

    snprintf(detail, sizeof(detail),
             "schema=mem_console_stage_g_bite1_v1\ndigest=%016llx\nprojects=%d\nview=%d\n",
             (unsigned long long)digest, state.selected_project_count,
             mem_console_graph_view_mode_get(&state));
    if (!mc_stageg_write_text("state_transfer.canonical", detail)) return 16;
    mc_stageg_trace("b1_canonical", "artifact=state_transfer.canonical", digest);
    mc_stageg_trace("b1_shutdown", "state_destroyed=1", digest);
    return 0;
}
