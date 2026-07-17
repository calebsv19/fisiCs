#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core_pack.h"
#include "core_scene.h"
#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_authoring_host.h"
#include "drawing_program/drawing_program_color_model.h"
#include "drawing_program/drawing_program_project_state.h"
#include "drawing_program/drawing_program_snapshot_shell.h"
#include "drawing_program/drawing_program_snapshot_ui_settings.h"
#include "drawing_program/drawing_program_texture_export.h"
#include "drawing_program/drawing_program_texture_project.h"
#include "drawing_program/drawing_program_texture_project_template.h"
#include "drawing_program/drawing_program_texture_scene_import.h"
#include "drawing_program_snapshot_internal.h"

static CoreResult fixture_invalid(const char *message) {
    CoreResult result = { CORE_ERR_INVALID_ARG, message };
    return result;
}

CoreResult drawing_program_snapshot_invalid(const char *message) {
    return fixture_invalid(message);
}

void drawing_program_authoring_host_export_accepted_ui_state(
    const DrawingProgramAppContext *ctx,
    DrawingProgramAppUiState *out_ui) {
    if (ctx && out_ui) *out_ui = ctx->ui;
}

CoreResult drawing_program_authoring_host_export_accepted_pane_state(
    const DrawingProgramAppContext *ctx,
    CoreLayoutState *out_layout_state,
    CorePaneNode out_nodes[DRAWING_PROGRAM_PANE_NODE_CAPACITY],
    uint32_t *out_node_count,
    uint32_t *out_root_index,
    CorePaneModuleBinding out_module_bindings[DRAWING_PROGRAM_MODULE_BINDING_CAPACITY],
    uint32_t *out_module_binding_count) {
    if (!ctx || !out_layout_state || !out_nodes || !out_node_count || !out_root_index ||
        !out_module_bindings || !out_module_binding_count) {
        return fixture_invalid("invalid fixture pane export");
    }
    *out_layout_state = ctx->pane_host.layout_state;
    memcpy(out_nodes, ctx->pane_host.nodes, sizeof(ctx->pane_host.nodes));
    memcpy(out_module_bindings, ctx->pane_host.module_bindings, sizeof(ctx->pane_host.module_bindings));
    *out_node_count = ctx->pane_host.node_count;
    *out_root_index = ctx->pane_host.root_index;
    *out_module_binding_count = ctx->pane_host.module_binding_count;
    return core_result_ok();
}

/* The session object references scene-import paths that are not part of this
 * scripted workflow. These explicit stubs do not replace project commit,
 * snapshot, export, or any state transition exercised below. */
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
    return fixture_invalid("scene import outside bite 4");
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
    return fixture_invalid("scene template outside bite 4");
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
    return fixture_invalid("project import path outside bite 4");
}

static int require_ok(CoreResult result, const char *operation) {
    if (result.code == CORE_OK) return 1;
    fprintf(stderr, "stage_g bite4 failed: %s code=%d message=%s\n",
            operation, (int)result.code, result.message ? result.message : "");
    return 0;
}

static uint32_t mix32(uint32_t hash, uint32_t value) {
    hash ^= value;
    return hash * UINT32_C(16777619);
}

static uint32_t workflow_digest(const DrawingProgramAppContext *ctx) {
    const DrawingProgramRasterSample *samples = 0;
    uint32_t sample_count = 0u;
    uint32_t hash = UINT32_C(2166136261);
    uint32_t i;
    hash = mix32(hash, ctx->document.schema_version);
    hash = mix32(hash, ctx->document.logical_width);
    hash = mix32(hash, ctx->document.logical_height);
    hash = mix32(hash, ctx->document.layer_count);
    hash = mix32(hash, ctx->editor.active_layer_id);
    if (drawing_program_layer_raster_store_export_layer_or_legacy_base(
            &ctx->layer_rasters, &ctx->document, ctx->editor.active_layer_id,
            &samples, &sample_count).code != CORE_OK || !samples) return 0u;
    hash = mix32(hash, sample_count);
    for (i = 0u; i < sample_count; ++i) hash = mix32(hash, samples[i]);
    hash = mix32(hash, ctx->history.count);
    hash = mix32(hash, ctx->history.cursor);
    hash = mix32(hash, ctx->history.raster_delta_count);
    for (i = 0u; i < ctx->history.count; ++i) {
        const DrawingProgramCommand *command = &ctx->history.entries[i];
        hash = mix32(hash, command->type);
        hash = mix32(hash, command->layer_id);
        hash = mix32(hash, command->object_id);
        hash = mix32(hash, command->new_object_width);
        hash = mix32(hash, command->new_object_height);
        hash = mix32(hash, command->previous_object_width);
        hash = mix32(hash, command->previous_object_height);
    }
    for (i = 0u; i < ctx->history.raster_delta_count; ++i) {
        const DrawingProgramHistoryRasterDeltaEntry *entry = &ctx->history.raster_delta_entries[i];
        hash = mix32(hash, entry->sample_index);
        hash = mix32(hash, entry->previous_sample_value);
        hash = mix32(hash, entry->new_sample_value);
    }
    hash = mix32(hash, ctx->selection.has_payload);
    hash = mix32(hash, ctx->selection.origin_x);
    hash = mix32(hash, ctx->selection.origin_y);
    hash = mix32(hash, ctx->selection.width);
    hash = mix32(hash, ctx->selection.height);
    hash = mix32(hash, ctx->object_store.object_count);
    for (i = 0u; i < ctx->object_store.object_count; ++i) {
        const DrawingProgramObjectRecord *object = &ctx->object_store.objects[i];
        hash = mix32(hash, object->object_id);
        hash = mix32(hash, object->layer_id);
        hash = mix32(hash, object->type);
        hash = mix32(hash, (uint32_t)object->origin_x);
        hash = mix32(hash, (uint32_t)object->origin_y);
        hash = mix32(hash, object->width);
        hash = mix32(hash, object->height);
        hash = mix32(hash, object->stroke_color_value);
        hash = mix32(hash, object->fill_color_value);
    }
    hash = mix32(hash, ctx->ui.theme_preset_id);
    hash = mix32(hash, ctx->ui.font_preset_id);
    hash = mix32(hash, (uint8_t)ctx->ui.font_zoom_step);
    hash = mix32(hash, ctx->ui.active_color_index);
    return hash;
}

static void checkpoint(const char *name, const DrawingProgramAppContext *ctx, int result_code) {
    const DrawingProgramObjectRecord *object =
        ctx->object_store.object_count ? &ctx->object_store.objects[0] : 0;
    printf("TRACE|1|%s|state=%08" PRIx32 "|doc=%ux%u|history=%u,%u,%u"
           "|selection=%u,%u,%u,%u,%u|object=%u,%u,%u|result=%d\n",
           name,
           workflow_digest(ctx),
           ctx->document.logical_width,
           ctx->document.logical_height,
           ctx->history.cursor,
           ctx->history.count,
           ctx->history.raster_delta_count,
           ctx->selection.has_payload,
           ctx->selection.origin_x,
           ctx->selection.origin_y,
           ctx->selection.width,
           ctx->selection.height,
           ctx->object_store.object_count,
           object ? object->width : 0u,
           object ? object->height : 0u,
           result_code);
}

static int write_pack(const DrawingProgramAppContext *ctx, const char *path) {
    CorePackWriter writer;
    if (!require_ok(core_pack_writer_open(path, &writer), "writer_open")) return 0;
    if (!require_ok(drawing_program_snapshot_shell_write_current(&writer, ctx), "write_shell") ||
        !require_ok(drawing_program_snapshot_write_history_raster_delta_chunk(&writer, ctx), "write_history") ||
        !require_ok(drawing_program_snapshot_write_layer_raster_chunk(&writer, ctx), "write_layers") ||
        !require_ok(drawing_program_snapshot_write_object_chunk(&writer, ctx), "write_objects") ||
        !require_ok(drawing_program_snapshot_write_ui_settings_chunk(&writer, ctx), "write_ui")) {
        (void)core_pack_writer_close(&writer);
        return 0;
    }
    return require_ok(core_pack_writer_close(&writer), "writer_close");
}

static int load_chunk_bytes(CorePackReader *reader, const char id[4], void **out_data, uint64_t *out_size) {
    CorePackChunkInfo chunk;
    void *data;
    memset(&chunk, 0, sizeof(chunk));
    if (!require_ok(core_pack_reader_find_chunk(reader, id, 0u, &chunk), "find_chunk")) return 0;
    data = malloc((size_t)chunk.size);
    if (!data) return 0;
    if (!require_ok(core_pack_reader_read_chunk_data(reader, &chunk, data, chunk.size), "read_chunk")) {
        free(data);
        return 0;
    }
    *out_data = data;
    *out_size = chunk.size;
    return 1;
}

static int load_pack(DrawingProgramAppContext *ctx, const char *path) {
    CorePackReader reader;
    CorePackChunkInfo ui_chunk;
    void *layer_data = 0;
    void *object_data = 0;
    uint64_t layer_size = 0u;
    uint64_t object_size = 0u;
    uint8_t found = 0u;
    int ok = 0;
    if (!require_ok(core_pack_reader_open(path, &reader), "reader_open")) return 0;
    if (!require_ok(drawing_program_snapshot_shell_load_current(ctx, &reader, &found), "load_shell") || !found) goto done;
    if (!require_ok(drawing_program_snapshot_apply_history_raster_delta_chunk(ctx, &reader), "load_history")) goto done;
    if (!require_ok(drawing_program_layer_raster_store_init_from_document(&ctx->layer_rasters, &ctx->document),
                    "load_raster_init")) goto done;
    if (!load_chunk_bytes(&reader, "DPLR", &layer_data, &layer_size) ||
        !require_ok(drawing_program_snapshot_apply_layer_raster_chunk(ctx, layer_data, layer_size), "load_layers")) goto done;
    drawing_program_object_store_reset(&ctx->object_store);
    if (!load_chunk_bytes(&reader, "DPOB", &object_data, &object_size) ||
        !require_ok(drawing_program_snapshot_apply_object_chunk(ctx, object_data, object_size), "load_objects")) goto done;
    memset(&ui_chunk, 0, sizeof(ui_chunk));
    if (!require_ok(core_pack_reader_find_chunk(&reader, "DPUI", 0u, &ui_chunk), "find_ui") ||
        !require_ok(drawing_program_snapshot_apply_ui_settings_chunk(ctx, &reader, &ui_chunk), "load_ui")) goto done;
    ok = 1;
done:
    free(layer_data);
    free(object_data);
    (void)core_pack_reader_close(&reader);
    return ok;
}

static int write_canonical(const DrawingProgramAppContext *ctx, uint32_t digest) {
    FILE *file = fopen("workflow.canonical", "wb");
    if (!file) return 0;
    fprintf(file, "schema=stage-g-workflow-v1\n");
    fprintf(file, "workflow=bootstrap,project,draw,select,move,resize,undo,redo,save,reload,export,shutdown\n");
    fprintf(file, "chunks=DPS3,DPHD,DPLR,DPOB,DPUI\n");
    fprintf(file, "state=%08" PRIx32 "\n", digest);
    fprintf(file, "shape=%ux%u objects=%u history=%u/%u deltas=%u\n",
            ctx->document.logical_width, ctx->document.logical_height,
            ctx->object_store.object_count, ctx->history.cursor,
            ctx->history.count, ctx->history.raster_delta_count);
    fprintf(file, "exports=export/stage_g_workflow_surface_01.png,export/stage_g_workflow_texture_manifest.json\n");
    return fclose(file) == 0;
}

static void dispose_context(DrawingProgramAppContext *ctx) {
    if (!ctx) return;
    drawing_program_layer_raster_store_dispose(&ctx->layer_rasters);
    drawing_program_texture_project_dispose(&ctx->texture_project);
    free(ctx);
}

int main(int argc, char **argv) {
    DrawingProgramAppContext *ctx = 0;
    DrawingProgramAppContext *loaded = 0;
    DrawingProgramHistoryRasterDeltaEntry deltas[4];
    DrawingProgramObjectRecord object;
    uint32_t layer_id;
    uint32_t object_id = 0u;
    uint32_t saved_digest;
    uint32_t loaded_digest;
    (void)argc;
    (void)argv;

    ctx = (DrawingProgramAppContext *)calloc(1u, sizeof(*ctx));
    if (!ctx) return 120;
    if (!require_ok(drawing_program_document_init_with_shape(&ctx->document, 32u, 24u, 1u), "bootstrap_document") ||
        !require_ok(drawing_program_layer_raster_store_init_from_document(&ctx->layer_rasters, &ctx->document),
                    "bootstrap_rasters")) return 121;
    drawing_program_editor_state_init(&ctx->editor, &ctx->document);
    drawing_program_history_init(&ctx->history);
    drawing_program_object_store_reset(&ctx->object_store);
    drawing_program_selection_reset(&ctx->selection);
    drawing_program_clipboard_reset(&ctx->clipboard);
    layer_id = ctx->editor.active_layer_id;
    ctx->ui.theme_preset_id = 1u;
    ctx->ui.font_preset_id = 1u;
    ctx->ui.font_zoom_step = -1;
    ctx->ui.active_color_index = 5u;
    checkpoint("b4_bootstrap", ctx, 1);

    if (!require_ok(drawing_program_texture_project_init_single_surface(
                        &ctx->texture_project, &ctx->document, &ctx->layer_rasters,
                        "Workflow Surface", DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_STANDARD),
                    "create_project")) return 122;
    (void)snprintf(ctx->texture_project.source_object_id,
                   sizeof(ctx->texture_project.source_object_id), "%s", "stage_g_workflow");
    checkpoint("b4_project", ctx, 1);

    memset(deltas, 0, sizeof(deltas));
    deltas[0].sample_index = 66u;
    deltas[1].sample_index = 67u;
    deltas[2].sample_index = 98u;
    deltas[3].sample_index = 99u;
    deltas[0].new_sample_value = deltas[2].new_sample_value = drawing_program_color_value_from_index(2u);
    deltas[1].new_sample_value = deltas[3].new_sample_value = drawing_program_color_value_from_index(6u);
    if (!require_ok(drawing_program_history_apply_raster_delta_block(
                        &ctx->history, &ctx->document, &ctx->layer_rasters, layer_id, deltas, 4u),
                    "draw")) return 123;
    memset(&object, 0, sizeof(object));
    object.layer_id = layer_id;
    object.type = DRAWING_PROGRAM_OBJECT_TYPE_RECT;
    object.visible = 1u;
    object.stroke_width = 2u;
    object.origin_x = 8;
    object.origin_y = 6;
    object.width = 6u;
    object.height = 5u;
    object.stroke_color_value = drawing_program_color_value_from_index(3u);
    object.fill_color_value = drawing_program_color_value_from_index(7u);
    memcpy(object.name, "Workflow Rect", sizeof("Workflow Rect"));
    if (!require_ok(drawing_program_object_store_add(&ctx->object_store, &object, &object_id), "draw_object")) return 124;
    checkpoint("b4_draw", ctx, 1);

    if (!drawing_program_selection_capture_from_rect(
            &ctx->document, &ctx->layer_rasters, layer_id, &ctx->selection, 2, 2, 2u, 2u)) return 125;
    checkpoint("b4_select", ctx, 1);
    drawing_program_selection_begin_move_tracking(&ctx->selection, 2u, 2u);
    drawing_program_selection_update_move_offset(&ctx->selection, 5u, 4u);
    if (!require_ok(drawing_program_selection_commit_move(
                        &ctx->document, &ctx->layer_rasters, layer_id, &ctx->history, &ctx->selection),
                    "move_selection")) return 126;
    checkpoint("b4_move", ctx, 1);

    if (!require_ok(drawing_program_history_apply_set_object_size(
                        &ctx->history, &ctx->object_store, object_id, 11u, 9u),
                    "resize_object")) return 127;
    checkpoint("b4_resize", ctx, 1);
    if (!require_ok(drawing_program_history_undo(
                        &ctx->history, &ctx->document, &ctx->layer_rasters, &ctx->object_store),
                    "undo")) return 128;
    checkpoint("b4_undo", ctx, 1);
    if (!require_ok(drawing_program_history_redo(
                        &ctx->history, &ctx->document, &ctx->layer_rasters, &ctx->object_store),
                    "redo")) return 129;
    checkpoint("b4_redo", ctx, 1);

    saved_digest = workflow_digest(ctx);
    if (!write_pack(ctx, "workflow.pack")) return 130;
    checkpoint("b4_saved", ctx, 1);
    dispose_context(ctx);
    ctx = 0;

    loaded = (DrawingProgramAppContext *)calloc(1u, sizeof(*loaded));
    if (!loaded || !load_pack(loaded, "workflow.pack")) return 131;
    loaded_digest = workflow_digest(loaded);
    if (loaded_digest != saved_digest) {
        fprintf(stderr, "stage_g bite4 reload mismatch saved=%08" PRIx32 " loaded=%08" PRIx32 "\n",
                saved_digest, loaded_digest);
        return 132;
    }
    checkpoint("b4_reloaded", loaded, 1);

    if (!require_ok(drawing_program_texture_project_init_single_surface(
                        &loaded->texture_project, &loaded->document, &loaded->layer_rasters,
                        "Workflow Surface", DRAWING_PROGRAM_TEXTURE_QUALITY_PRESET_STANDARD),
                    "reload_project") ||
        snprintf(loaded->texture_project.source_object_id,
                 sizeof(loaded->texture_project.source_object_id), "%s", "stage_g_workflow") < 0 ||
        !require_ok(drawing_program_texture_export_current_project(loaded, "export"), "export")) return 133;
    checkpoint("b4_exported", loaded, 1);
    if (!write_canonical(loaded, loaded_digest)) return 134;
    checkpoint("b4_shutdown", loaded, 1);
    dispose_context(loaded);
    return 0;
}
