#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core_scene.h"
#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_project_state.h"
#include "drawing_program/drawing_program_texture_canvas_move.h"
#include "drawing_program/drawing_program_texture_canvas_resize.h"
#include "drawing_program/drawing_program_texture_project.h"
#include "drawing_program/drawing_program_texture_project_session.h"
#include "drawing_program/drawing_program_texture_project_template.h"
#include "drawing_program/drawing_program_texture_scene_import.h"
#include "drawing_program/drawing_program_texture_workspace.h"
#include "drawing_program/drawing_program_visual_state.h"

static CoreResult unsupported_fixture_path(const char *message) {
    CoreResult result = { CORE_ERR_INVALID_ARG, message };
    return result;
}

/* The selected session object references scene-import helpers from an unrelated
 * production path. Bite 3 never calls that path; these stubs keep the link seam
 * explicit without replacing move, resize, workspace, or project state logic. */
void core_scene_root_contract_init(CoreSceneRootContract *root) {
    if (root) memset(root, 0, sizeof(*root));
}

void core_scene_object_contract_init(CoreSceneObjectContract *object) {
    if (object) memset(object, 0, sizeof(*object));
}

CoreResult drawing_program_texture_scene_import_load_object(
    const char *scene_json_path,
    const char *object_id,
    CoreSceneRootContract *out_scene_root,
    CoreSceneObjectContract *out_scene_object) {
    (void)scene_json_path;
    (void)object_id;
    (void)out_scene_root;
    (void)out_scene_object;
    return unsupported_fixture_path("scene import outside bite 3");
}

CoreResult drawing_program_texture_project_init_from_scene_object(
    DrawingProgramTextureProject *project,
    const CoreSceneRootContract *scene_root,
    const CoreSceneObjectContract *scene_object,
    uint32_t quality_preset) {
    (void)project;
    (void)scene_root;
    (void)scene_object;
    (void)quality_preset;
    return unsupported_fixture_path("scene template outside bite 3");
}

CoreResult drawing_program_project_state_prepare_texture_import_path(
    DrawingProgramAppContext *ctx,
    const char *scene_id,
    const char *scene_path,
    const char *object_id) {
    (void)ctx;
    (void)scene_id;
    (void)scene_path;
    (void)object_id;
    return unsupported_fixture_path("project import path outside bite 3");
}

static int require_ok(CoreResult result, const char *operation) {
    if (result.code == CORE_OK) return 1;
    fprintf(stderr, "stage_g bite3 failed: %s code=%d message=%s\n",
            operation, (int)result.code, result.message ? result.message : "");
    return 0;
}

static uint32_t mix32(uint32_t hash, uint32_t value) {
    hash ^= value;
    return hash * UINT32_C(16777619);
}

static int32_t quantize_milli(float value) {
    return (int32_t)lroundf(value * 1000.0f);
}

static uint32_t semantic_digest(const DrawingProgramAppContext *ctx) {
    uint32_t hash = UINT32_C(2166136261);
    uint32_t i;
    hash = mix32(hash, ctx->texture_project.surface_count);
    hash = mix32(hash, ctx->texture_project.active_surface_index);
    hash = mix32(hash, ctx->document.logical_width);
    hash = mix32(hash, ctx->document.logical_height);
    hash = mix32(hash, ctx->history.count);
    hash = mix32(hash, ctx->history.cursor);
    for (i = 0u; i < ctx->texture_project.surface_count; ++i) {
        const DrawingProgramTextureSurface *surface =
            drawing_program_texture_project_surface_at(&ctx->texture_project, i);
        if (!surface || !surface->storage) return 0u;
        hash = mix32(hash, surface->surface_id);
        hash = mix32(hash, surface->storage->document.logical_width);
        hash = mix32(hash, surface->storage->document.logical_height);
        hash = mix32(hash, (uint32_t)quantize_milli(surface->layout_offset_x));
        hash = mix32(hash, (uint32_t)quantize_milli(surface->layout_offset_y));
        hash = mix32(hash, surface->is_blank);
        hash = mix32(hash, surface->resize_locked);
    }
    return hash;
}

static int checkpoint(const char *name,
                      const DrawingProgramAppContext *ctx,
                      SDL_Rect pane_rect,
                      const VisualCanvasInteractionState *interaction,
                      int result_code) {
    const DrawingProgramTextureSurface *surface =
        drawing_program_texture_project_surface_at(&ctx->texture_project, 0u);
    VisualCanvasSheetMetrics metrics;
    if (!surface || !surface->storage) {
        fprintf(stderr, "stage_g bite3 checkpoint %s missing surface storage count=%u\n",
                name, ctx->texture_project.surface_count);
        return 0;
    }
    if (!drawing_program_texture_workspace_surface_sheet_metrics(ctx, pane_rect, 0u, &metrics)) {
        fprintf(stderr,
                "stage_g bite3 checkpoint %s metrics failed frame=%d,%d,%d,%d doc=%ux%u surfaces=%u active=%u viewport=%d,%d,%u\n",
                name, pane_rect.x, pane_rect.y, pane_rect.w, pane_rect.h,
                ctx->document.raster_width, ctx->document.raster_height,
                ctx->texture_project.surface_count, ctx->texture_project.active_surface_index,
                quantize_milli(ctx->editor.viewport.pan_x),
                quantize_milli(ctx->editor.viewport.pan_y),
                (unsigned)quantize_milli(ctx->editor.viewport.zoom));
        return 0;
    }
    printf("TRACE|1|%s|state=%08" PRIx32 "|active=%u|surfaces=%u|doc=%ux%u"
           "|surface=%ux%u,%d,%d,%u,%u|sheet=%d,%d,%d,%d"
           "|interaction=%u,%u|history=%u,%u|result=%d\n",
           name,
           semantic_digest(ctx),
           ctx->texture_project.active_surface_index,
           ctx->texture_project.surface_count,
           ctx->document.logical_width,
           ctx->document.logical_height,
           surface->storage->document.logical_width,
           surface->storage->document.logical_height,
           quantize_milli(surface->layout_offset_x),
           quantize_milli(surface->layout_offset_y),
           surface->is_blank,
           surface->resize_locked,
           metrics.sheet_rect.x,
           metrics.sheet_rect.y,
           metrics.sheet_rect.w,
           metrics.sheet_rect.h,
           interaction->canvas_move_active,
           interaction->canvas_resize_active,
           ctx->history.cursor,
           ctx->history.count,
           result_code);
    return 1;
}

int main(int argc, char **argv) {
    DrawingProgramAppContext *ctx = 0;
    VisualCanvasInteractionState interaction;
    VisualCanvasSheetMetrics metrics;
    SDL_Rect pane_rect = { 100, 60, 640, 420 };
    SDL_Rect handle_rect;
    uint32_t second_surface = 0u;
    int anchor_x;
    int anchor_y;
    int drag_x;
    int drag_y;
    int ok;
    (void)argc;
    (void)argv;

    ctx = (DrawingProgramAppContext *)calloc(1u, sizeof(*ctx));
    if (!ctx) return 90;
    memset(&interaction, 0, sizeof(interaction));
    drawing_program_viewport_state_init(&ctx->editor.viewport);
    if (!require_ok(drawing_program_texture_project_session_seed_blank(
                        ctx, 64u, 48u, DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_STANDARD),
                    "seed_blank") ||
        !require_ok(drawing_program_texture_project_session_add_surface(
                        ctx, "Locked Control", 32u, 24u, 1u,
                        DRAWING_PROGRAM_TEXTURE_FACE_ROLE_RIGHT,
                        DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_STANDARD,
                        &second_surface),
                    "add_control_surface") ||
        !drawing_program_texture_workspace_fit_all(ctx, pane_rect)) {
        return 91;
    }
    drawing_program_texture_project_surface_at_mut(&ctx->texture_project, second_surface)->resize_locked = 1u;
    if (!checkpoint("b3_init", ctx, pane_rect, &interaction, 1)) return 92;

    if (!drawing_program_texture_workspace_surface_sheet_metrics(ctx, pane_rect, 0u, &metrics)) return 93;
    anchor_x = metrics.sheet_rect.x + metrics.sheet_rect.w / 2;
    anchor_y = metrics.sheet_rect.y + metrics.sheet_rect.h / 2;
    ok = drawing_program_texture_canvas_move_begin(ctx, pane_rect, &interaction, 0u, anchor_x, anchor_y);
    if (!ok || !checkpoint("b3_move_begin", ctx, pane_rect, &interaction, ok)) return 94;
    ok = drawing_program_texture_canvas_move_update(ctx, &interaction, anchor_x + 128, anchor_y + 64);
    if (!ok || !checkpoint("b3_move_forward", ctx, pane_rect, &interaction, ok)) return 95;
    ok = drawing_program_texture_canvas_move_update(ctx, &interaction, anchor_x - 80, anchor_y - 40);
    if (!ok || !checkpoint("b3_move_reverse", ctx, pane_rect, &interaction, ok)) return 96;

    if (!require_ok(drawing_program_texture_project_select_active_surface(&ctx->texture_project, second_surface),
                    "move_change_active")) return 97;
    ok = drawing_program_texture_canvas_move_update(ctx, &interaction, anchor_x + 32, anchor_y + 16);
    if (!ok || !checkpoint("b3_move_active_change", ctx, pane_rect, &interaction, ok)) return 98;
    if (!require_ok(drawing_program_texture_project_select_active_surface(&ctx->texture_project, 0u),
                    "move_restore_active")) return 99;
    drawing_program_texture_canvas_move_end(&interaction);
    if (!checkpoint("b3_move_end", ctx, pane_rect, &interaction, 1)) return 100;

    ok = drawing_program_texture_canvas_move_begin(ctx, pane_rect, &interaction, 999u, anchor_x, anchor_y);
    if (ok) return 101;
    if (!drawing_program_texture_canvas_move_begin(ctx, pane_rect, &interaction, 0u, anchor_x, anchor_y)) return 102;
    interaction.canvas_move_zoom = NAN;
    ok = drawing_program_texture_canvas_move_update(ctx, &interaction, anchor_x + 1, anchor_y + 1);
    drawing_program_texture_canvas_move_end(&interaction);
    if (ok || !checkpoint("b3_move_invalid", ctx, pane_rect, &interaction, ok)) return 103;

    if (!drawing_program_texture_canvas_resize_handle_rect_for_surface(ctx, pane_rect, 0u, &handle_rect)) return 104;
    anchor_x = handle_rect.x + handle_rect.w / 2;
    anchor_y = handle_rect.y + handle_rect.h / 2;
    ok = drawing_program_texture_canvas_resize_begin(ctx, pane_rect, &interaction, 0u, anchor_x, anchor_y);
    if (!ok || !checkpoint("b3_resize_begin", ctx, pane_rect, &interaction, ok)) return 105;
    drag_x = (int)lroundf(16.0f * interaction.canvas_resize_pixels_per_logical);
    drag_y = (int)lroundf(24.0f * interaction.canvas_resize_pixels_per_logical);
    ok = drawing_program_texture_canvas_resize_update(ctx, &interaction, anchor_x + drag_x, anchor_y + drag_y);
    if (!ok || ctx->document.logical_width != 80u || ctx->document.logical_height != 72u ||
        !checkpoint("b3_resize_grow", ctx, pane_rect, &interaction, ok)) return 106;
    ok = drawing_program_texture_canvas_resize_update(ctx, &interaction, anchor_x - 100000, anchor_y - 100000);
    if (!ok || ctx->document.logical_width != 1u || ctx->document.logical_height != 1u ||
        !checkpoint("b3_resize_clamp", ctx, pane_rect, &interaction, ok)) return 107;
    drag_x = (int)lroundf(-24.0f * interaction.canvas_resize_pixels_per_logical);
    drag_y = (int)lroundf(-12.0f * interaction.canvas_resize_pixels_per_logical);
    ok = drawing_program_texture_canvas_resize_update(ctx, &interaction, anchor_x + drag_x, anchor_y + drag_y);
    if (!ok || ctx->document.logical_width != 40u || ctx->document.logical_height != 36u ||
        !checkpoint("b3_resize_reverse", ctx, pane_rect, &interaction, ok)) return 108;
    drawing_program_texture_canvas_resize_end(&interaction);

    if (!drawing_program_texture_canvas_resize_begin(ctx, pane_rect, &interaction, 0u, anchor_x, anchor_y)) return 109;
    if (!require_ok(drawing_program_texture_project_select_active_surface(&ctx->texture_project, second_surface),
                    "resize_change_active")) return 110;
    ok = drawing_program_texture_canvas_resize_update(ctx, &interaction, anchor_x + 10, anchor_y + 10);
    if (ok || !checkpoint("b3_resize_active_change", ctx, pane_rect, &interaction, ok)) return 111;
    if (!require_ok(drawing_program_texture_project_select_active_surface(&ctx->texture_project, 0u),
                    "resize_restore_active")) return 112;
    drawing_program_texture_canvas_resize_end(&interaction);

    ok = drawing_program_texture_canvas_resize_handle_rect_for_surface(
        ctx, pane_rect, second_surface, &handle_rect);
    if (ok) return 113;
    if (!drawing_program_texture_canvas_resize_begin(ctx, pane_rect, &interaction, 0u, anchor_x, anchor_y)) return 114;
    interaction.canvas_resize_pixels_per_logical = 0.0f;
    ok = drawing_program_texture_canvas_resize_update(ctx, &interaction, anchor_x + 5, anchor_y + 5);
    drawing_program_texture_canvas_resize_end(&interaction);
    if (ok || !checkpoint("b3_resize_invalid", ctx, pane_rect, &interaction, ok)) return 115;
    if (!checkpoint("b3_resize_end", ctx, pane_rect, &interaction, 1)) return 116;

    drawing_program_layer_raster_store_dispose(&ctx->layer_rasters);
    drawing_program_texture_project_dispose(&ctx->texture_project);
    free(ctx);
    return 0;
}
