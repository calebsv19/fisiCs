#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "mem_console_state.h"

int main(void) {
    MemConsoleState state;

    memset(&state, 0, sizeof(state));

    mem_console_graph_kind_select_all(&state);
    assert(mem_console_graph_kind_filter_all_mask() != 0u);
    assert(mem_console_graph_kind_is_enabled(&state, "supports") == 1);
    assert(mem_console_graph_kind_toggle_enabled(&state, "supports") == 1);
    assert(mem_console_graph_kind_is_enabled(&state, "supports") == 0);

    mem_console_graph_node_kind_select_all(&state);
    assert(mem_console_graph_node_kind_filter_all_mask() != 0u);
    assert(mem_console_graph_node_kind_is_enabled(&state, "plan") == 1);
    assert(mem_console_graph_node_kind_toggle_enabled(&state, "plan") == 1);
    assert(mem_console_graph_node_kind_is_enabled(&state, "plan") == 0);

    assert(mem_console_project_filter_toggle(&state, "fisiCs") == 1);
    assert(mem_console_project_filter_toggle(&state, "mem_console") == 1);
    assert(state.selected_project_count == 2);

    state.project_filter_option_count = 1;
    (void)snprintf(state.project_filter_keys[0], sizeof(state.project_filter_keys[0]), "%s", "mem_console");
    mem_console_project_filter_prune_to_options(&state);
    assert(state.selected_project_count == 1);
    assert(strcmp(state.selected_project_keys[0], "mem_console") == 0);

    return selected_id_in_visible_items(&state) == 0 ? 0 : 1;
}
