#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "mem_console_state.h"

int main(void) {
    MemConsoleState state;
    int out_hidden = 0;

    memset(&state, 0, sizeof(state));

    assert(mem_console_graph_edge_limit_clamp(-1) == MEM_CONSOLE_GRAPH_EDGE_LIMIT_MIN);
    assert(mem_console_graph_edge_limit_clamp(99999) == MEM_CONSOLE_GRAPH_EDGE_LIMIT);
    assert(mem_console_graph_hops_clamp(0) == MEM_CONSOLE_GRAPH_HOPS_MIN);
    assert(mem_console_graph_hops_clamp(99) == MEM_CONSOLE_GRAPH_HOPS_MAX);
    assert(mem_console_graph_layout_mode_clamp(999) == MEM_CONSOLE_GRAPH_LAYOUT_DAG);
    assert(mem_console_graph_sort_mode_clamp(999) == MEM_CONSOLE_GRAPH_SORT_RECENT_FIRST);

    mem_console_graph_kind_select_all(&state);
    mem_console_graph_kind_set_single(&state, "depends_on");
    assert(mem_console_graph_kind_is_enabled(&state, "depends_on") == 1);
    assert(mem_console_graph_kind_is_enabled(&state, "supports") == 0);
    assert(mem_console_graph_kind_toggle_all_override(&state) == 1);
    assert(state.graph_kind_filter_all_override == 1);

    mem_console_graph_node_kind_select_all(&state);
    assert(mem_console_graph_node_kind_toggle_enabled(&state, "issue") == 1);
    assert(mem_console_graph_node_kind_is_enabled(&state, "issue") == 0);
    assert(mem_console_graph_node_kind_toggle_all_override(&state) == 1);
    assert(state.graph_node_kind_filter_all_override == 1);

    assert(mem_console_graph_anchor_hidden_set(&state, 42, 1) == 1);
    assert(mem_console_graph_anchor_hidden_is_set(&state, 42) == 1);
    assert(mem_console_graph_anchor_hidden_toggle(&state, 42, &out_hidden) == 1);
    assert(out_hidden == 0);
    assert(mem_console_graph_anchor_hidden_is_set(&state, 42) == 0);

    return 0;
}
