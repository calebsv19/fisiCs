#include "mem_console_state.h"

/* The selected pane-layout production code owns the drag mutation. */
void mem_console_pane_prefs_mark_dirty(MemConsoleState *state) {
    if (state) state->pane_prefs_dirty = 1;
}
