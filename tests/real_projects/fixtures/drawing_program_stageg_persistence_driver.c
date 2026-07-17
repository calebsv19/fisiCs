#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core_pack.h"
#include "drawing_program/drawing_program_app_main.h"
#include "drawing_program/drawing_program_authoring_host.h"
#include "drawing_program/drawing_program_color_model.h"
#include "drawing_program/drawing_program_snapshot_shell.h"
#include "drawing_program/drawing_program_snapshot_ui_settings.h"
#include "drawing_program_snapshot_internal.h"

static uint64_t mix64(uint64_t hash, uint64_t value) {
    uint32_t hash32 = (uint32_t)hash;
    hash32 ^= (uint32_t)value;
    hash32 *= UINT32_C(16777619);
    return (uint64_t)hash32;
}

static int require_ok(CoreResult result, const char *operation) {
    if (result.code == CORE_OK) return 1;
    fprintf(stderr, "stage_g bite2 failed: %s code=%d message=%s\n",
            operation, (int)result.code, result.message ? result.message : "");
    return 0;
}

CoreResult drawing_program_snapshot_invalid(const char *message) {
    CoreResult result = { CORE_ERR_INVALID_ARG, message };
    return result;
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
        return drawing_program_snapshot_invalid("invalid fixture pane export");
    }
    *out_layout_state = ctx->pane_host.layout_state;
    memcpy(out_nodes, ctx->pane_host.nodes, sizeof(ctx->pane_host.nodes));
    memcpy(out_module_bindings, ctx->pane_host.module_bindings, sizeof(ctx->pane_host.module_bindings));
    *out_node_count = ctx->pane_host.node_count;
    *out_root_index = ctx->pane_host.root_index;
    *out_module_binding_count = ctx->pane_host.module_binding_count;
    return core_result_ok();
}

static uint64_t document_digest(const DrawingProgramAppContext *ctx) {
    uint64_t hash = UINT64_C(1469598103934665603);
    uint32_t i;
    hash = mix64(hash, ctx->document.schema_version);
    hash = mix64(hash, ctx->document.logical_width);
    hash = mix64(hash, ctx->document.logical_height);
    hash = mix64(hash, ctx->document.layer_count);
    for (i = 0u; i < ctx->document.layer_count; ++i) {
        hash = mix64(hash, ctx->document.layers[i].layer_id);
        hash = mix64(hash, ctx->document.layers[i].visible);
        hash = mix64(hash, ctx->document.layers[i].locked);
    }
    return hash;
}

static uint64_t raster_digest(const DrawingProgramAppContext *ctx) {
    const DrawingProgramRasterSample *samples = 0;
    uint32_t sample_count = 0u;
    uint64_t hash = UINT64_C(1469598103934665603);
    uint32_t i;
    if (drawing_program_layer_raster_store_export_layer_or_legacy_base(
            &ctx->layer_rasters, &ctx->document, ctx->editor.active_layer_id,
            &samples, &sample_count).code != CORE_OK || !samples) return 0u;
    hash = mix64(hash, sample_count);
    for (i = 0u; i < sample_count; ++i) hash = mix64(hash, samples[i]);
    return hash;
}

static uint64_t history_digest(const DrawingProgramAppContext *ctx) {
    uint64_t hash = UINT64_C(1469598103934665603);
    uint32_t i;
    hash = mix64(hash, (uint64_t)ctx->history.count);
    hash = mix64(hash, (uint64_t)ctx->history.cursor);
    hash = mix64(hash, (uint64_t)ctx->history.raster_delta_count);
    for (i = 0u; i < ctx->history.raster_delta_count; ++i) {
        hash = mix64(hash, (uint64_t)ctx->history.raster_delta_entries[i].sample_index);
        hash = mix64(hash, (uint64_t)ctx->history.raster_delta_entries[i].previous_sample_value);
        hash = mix64(hash, (uint64_t)ctx->history.raster_delta_entries[i].new_sample_value);
    }
    return hash;
}

static uint64_t object_digest(const DrawingProgramAppContext *ctx) {
    uint64_t hash = UINT64_C(1469598103934665603);
    uint32_t i;
    hash = mix64(hash, (uint64_t)ctx->object_store.object_count);
    hash = mix64(hash, (uint64_t)ctx->object_store.next_object_id);
    for (i = 0u; i < ctx->object_store.object_count; ++i) {
        const DrawingProgramObjectRecord *object = &ctx->object_store.objects[i];
        hash = mix64(hash, (uint64_t)object->object_id);
        hash = mix64(hash, (uint64_t)object->layer_id);
        hash = mix64(hash, (uint64_t)object->type);
        hash = mix64(hash, (uint64_t)(uint32_t)object->origin_x);
        hash = mix64(hash, (uint64_t)(uint32_t)object->origin_y);
        hash = mix64(hash, (uint64_t)object->width);
        hash = mix64(hash, (uint64_t)object->height);
        hash = mix64(hash, (uint64_t)object->stroke_color_value);
        hash = mix64(hash, (uint64_t)object->fill_color_value);
    }
    return hash;
}

static uint64_t ui_selection_digest(const DrawingProgramAppContext *ctx) {
    uint64_t hash = UINT64_C(1469598103934665603);
    hash = mix64(hash, ctx->ui.theme_preset_id);
    hash = mix64(hash, ctx->ui.font_preset_id);
    hash = mix64(hash, (uint8_t)ctx->ui.font_zoom_step);
    hash = mix64(hash, ctx->ui.active_color_index);
    hash = mix64(hash, ctx->ui.tool_brush_size);
    hash = mix64(hash, ctx->ui.canvas_control_mode);
    hash = mix64(hash, ctx->selection.has_payload);
    hash = mix64(hash, ctx->selection.origin_x);
    hash = mix64(hash, ctx->selection.origin_y);
    hash = mix64(hash, ctx->selection.width);
    hash = mix64(hash, ctx->selection.height);
    return hash;
}

static uint64_t state_digest(const DrawingProgramAppContext *ctx) {
    uint64_t hash = UINT64_C(1469598103934665603);
    hash = mix64(hash, document_digest(ctx));
    hash = mix64(hash, raster_digest(ctx));
    hash = mix64(hash, history_digest(ctx));
    hash = mix64(hash, object_digest(ctx));
    hash = mix64(hash, ui_selection_digest(ctx));
    return hash;
}

static void checkpoint(const char *name, const DrawingProgramAppContext *ctx, uint64_t artifact_digest) {
    printf("TRACE|1|%s|state=%016" PRIx64 "|parts=%016" PRIx64 ",%016" PRIx64 ",%016" PRIx64 ",%016" PRIx64 ",%016" PRIx64 "|history=%u,%u,%u|objects=%u|ui=%u,%u,%d,%u|artifact=%016" PRIx64 "\n",
           name, state_digest(ctx), document_digest(ctx), raster_digest(ctx),
           history_digest(ctx), object_digest(ctx), ui_selection_digest(ctx),
           ctx->history.cursor, ctx->history.count,
           ctx->history.raster_delta_count, ctx->object_store.object_count,
           ctx->ui.theme_preset_id, ctx->ui.font_preset_id,
           (int)ctx->ui.font_zoom_step, ctx->ui.active_color_index, artifact_digest);
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
    if (!require_ok(drawing_program_layer_raster_store_init_from_document(&ctx->layer_rasters, &ctx->document), "load_raster_init")) goto done;
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

static int write_canonical(const DrawingProgramAppContext *ctx, uint64_t digest) {
    FILE *file = fopen("snapshot.canonical", "wb");
    if (!file) return 0;
    fprintf(file, "schema=stage-g-persistence-v1\n");
    fprintf(file, "chunks=DPS3,DPHD,DPLR,DPOB,DPUI\n");
    fprintf(file, "state=%016" PRIx64 "\n", digest);
    fprintf(file, "shape=%ux%u layers=%u objects=%u history=%u/%u deltas=%u\n",
            ctx->document.raster_width, ctx->document.raster_height,
            ctx->document.layer_count, ctx->object_store.object_count,
            ctx->history.cursor, ctx->history.count, ctx->history.raster_delta_count);
    return fclose(file) == 0;
}

int main(int argc, char **argv) {
    DrawingProgramAppContext *saved = 0;
    DrawingProgramAppContext *loaded = 0;
    DrawingProgramHistoryRasterDeltaEntry deltas[3];
    DrawingProgramObjectRecord object;
    uint32_t layer_id = 0u;
    uint32_t object_id = 0u;
    uint64_t saved_digest;
    uint64_t loaded_digest;
    uint64_t saved_parts[5];
    uint32_t saved_ui_selection[12];
    (void)argc;
    (void)argv;
    saved = (DrawingProgramAppContext *)calloc(1u, sizeof(*saved));
    if (!saved) return 70;
    if (!require_ok(drawing_program_document_init_with_shape(&saved->document, 64u, 64u, 1u), "document_init") ||
        !require_ok(drawing_program_document_add_layer(&saved->document, "Persisted", &layer_id), "add_layer") ||
        !require_ok(drawing_program_layer_raster_store_init_from_document(&saved->layer_rasters, &saved->document), "raster_init")) return 71;
    saved->editor.active_layer_id = layer_id;
    drawing_program_history_init(&saved->history);
    drawing_program_object_store_reset(&saved->object_store);
    drawing_program_selection_reset(&saved->selection);
    memset(deltas, 0, sizeof(deltas));
    deltas[0].sample_index = 65u; deltas[0].new_sample_value = drawing_program_color_value_from_index(2u);
    deltas[1].sample_index = 129u; deltas[1].new_sample_value = drawing_program_color_value_from_index(5u);
    deltas[2].sample_index = 4030u; deltas[2].new_sample_value = drawing_program_color_value_from_index(9u);
    deltas[0].previous_sample_value = deltas[1].previous_sample_value = deltas[2].previous_sample_value = drawing_program_color_eraser_value();
    if (!require_ok(drawing_program_history_apply_raster_delta_block(
                        &saved->history, &saved->document, &saved->layer_rasters,
                        layer_id, deltas, 3u), "mutate_raster")) return 72;
    memset(&object, 0, sizeof(object));
    object.layer_id = layer_id;
    object.type = DRAWING_PROGRAM_OBJECT_TYPE_RECT;
    object.visible = 1u;
    object.stroke_width = 3u;
    object.origin_x = 7;
    object.origin_y = 11;
    object.width = 19u;
    object.height = 23u;
    object.stroke_color_value = drawing_program_color_value_from_index(4u);
    object.fill_color_value = drawing_program_color_value_from_index(8u);
    memcpy(object.name, "Persisted Rect", sizeof("Persisted Rect"));
    if (!require_ok(drawing_program_object_store_add(&saved->object_store, &object, &object_id), "add_object")) return 73;
    saved->ui.theme_preset_id = 1u;
    saved->ui.font_preset_id = 1u;
    saved->ui.font_zoom_step = -1;
    saved->ui.active_color_index = 6u;
    saved->ui.tool_brush_size = 13u;
    saved->ui.canvas_control_mode = 2u;
    if (!drawing_program_selection_capture_from_rect(&saved->document, &saved->layer_rasters,
                                                     layer_id, &saved->selection, 1, 1, 4u, 3u)) return 74;
    saved_digest = state_digest(saved);
    saved_parts[0] = document_digest(saved);
    saved_parts[1] = raster_digest(saved);
    saved_parts[2] = history_digest(saved);
    saved_parts[3] = object_digest(saved);
    saved_parts[4] = ui_selection_digest(saved);
    saved_ui_selection[0] = saved->ui.theme_preset_id;
    saved_ui_selection[1] = saved->ui.font_preset_id;
    saved_ui_selection[2] = (uint8_t)saved->ui.font_zoom_step;
    saved_ui_selection[3] = saved->ui.active_color_index;
    saved_ui_selection[4] = saved->ui.tool_brush_size;
    saved_ui_selection[5] = saved->ui.canvas_control_mode;
    saved_ui_selection[6] = saved->selection.has_payload;
    saved_ui_selection[7] = saved->selection.origin_x;
    saved_ui_selection[8] = saved->selection.origin_y;
    saved_ui_selection[9] = saved->selection.width;
    saved_ui_selection[10] = saved->selection.height;
    saved_ui_selection[11] = saved->editor.active_layer_id;
    checkpoint("b2_mutated", saved, saved_digest);
    if (!write_pack(saved, "snapshot.pack")) return 75;
    checkpoint("b2_saved", saved, saved_digest);
    drawing_program_layer_raster_store_dispose(&saved->layer_rasters);
    free(saved);
    saved = 0;
    loaded = (DrawingProgramAppContext *)calloc(1u, sizeof(*loaded));
    if (!loaded || !load_pack(loaded, "snapshot.pack")) return 76;
    loaded_digest = state_digest(loaded);
    if (loaded_digest != saved_digest) {
        fprintf(stderr, "stage_g bite2 roundtrip mismatch saved=%016" PRIx64 " loaded=%016" PRIx64 "\n", saved_digest, loaded_digest);
        fprintf(stderr, "stage_g bite2 saved_parts=%016" PRIx64 ",%016" PRIx64 ",%016" PRIx64 ",%016" PRIx64 ",%016" PRIx64 "\n",
                saved_parts[0], saved_parts[1], saved_parts[2], saved_parts[3], saved_parts[4]);
        fprintf(stderr, "stage_g bite2 loaded_parts=%016" PRIx64 ",%016" PRIx64 ",%016" PRIx64 ",%016" PRIx64 ",%016" PRIx64 "\n",
                document_digest(loaded), raster_digest(loaded), history_digest(loaded),
                object_digest(loaded), ui_selection_digest(loaded));
        fprintf(stderr, "stage_g bite2 saved_ui=%u,%u,%u,%u,%u,%u sel=%u,%u,%u,%u,%u layer=%u\n",
                saved_ui_selection[0], saved_ui_selection[1], saved_ui_selection[2],
                saved_ui_selection[3], saved_ui_selection[4], saved_ui_selection[5],
                saved_ui_selection[6], saved_ui_selection[7], saved_ui_selection[8],
                saved_ui_selection[9], saved_ui_selection[10], saved_ui_selection[11]);
        fprintf(stderr, "stage_g bite2 loaded_ui=%u,%u,%u,%u,%u,%u sel=%u,%u,%u,%u,%u layer=%u\n",
                loaded->ui.theme_preset_id, loaded->ui.font_preset_id,
                (uint8_t)loaded->ui.font_zoom_step, loaded->ui.active_color_index,
                loaded->ui.tool_brush_size, loaded->ui.canvas_control_mode,
                loaded->selection.has_payload, loaded->selection.origin_x,
                loaded->selection.origin_y, loaded->selection.width,
                loaded->selection.height, loaded->editor.active_layer_id);
        return 77;
    }
    checkpoint("b2_reloaded", loaded, loaded_digest);
    if (!require_ok(drawing_program_history_undo(&loaded->history, &loaded->document, &loaded->layer_rasters, &loaded->object_store), "undo_after_load")) return 78;
    checkpoint("b2_undo", loaded, loaded_digest);
    if (!require_ok(drawing_program_history_redo(&loaded->history, &loaded->document, &loaded->layer_rasters, &loaded->object_store), "redo_after_load")) return 79;
    if (state_digest(loaded) != loaded_digest) return 80;
    checkpoint("b2_redo", loaded, loaded_digest);
    if (!write_canonical(loaded, loaded_digest)) return 81;
    drawing_program_layer_raster_store_dispose(&loaded->layer_rasters);
    free(loaded);
    return 0;
}
