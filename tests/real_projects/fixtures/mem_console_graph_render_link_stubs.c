#include "kit_render.h"
#include "mem_console_workspace_authoring.h"

#include <string.h>

/*
 * The Stage-B graph-filter subsets exercise state transitions only.  The
 * production graph-structure object also contains draw helpers, so provide
 * inert link seams for those unreachable renderer calls without substituting
 * any graph-filter or viewport behavior.
 */
CoreResult kit_render_push_rect(KitRenderFrame *frame,
                                const KitRenderRectCommand *cmd) {
    (void)frame;
    (void)cmd;
    return core_result_ok();
}

CoreResult kit_render_push_line(KitRenderFrame *frame,
                                const KitRenderLineCommand *cmd) {
    (void)frame;
    (void)cmd;
    return core_result_ok();
}

CoreResult kit_render_push_text(KitRenderFrame *frame,
                                const KitRenderTextCommand *cmd) {
    (void)frame;
    (void)cmd;
    return core_result_ok();
}

int kit_ui_point_in_rect(KitRenderRect bounds, float x, float y) {
    return x >= bounds.x && y >= bounds.y &&
           x <= bounds.x + bounds.width && y <= bounds.y + bounds.height;
}

void mem_console_workspace_authoring_host_reset(MemConsoleWorkspaceAuthoringHost *host) {
    if (!host) return;
    memset(host, 0, sizeof(*host));
    host->overlay_mode = MEM_CONSOLE_WORKSPACE_AUTHORING_OVERLAY_PANES;
    host->entry_chord_armed_key = KIT_WORKSPACE_AUTHORING_KEY_UNKNOWN;
}
