#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "drawing_program/drawing_program_color_model.h"
#include "drawing_program/drawing_program_document.h"
#include "drawing_program/drawing_program_history.h"
#include "drawing_program/drawing_program_layer_raster.h"
#include "drawing_program/drawing_program_selection.h"

typedef struct StageGState {
    DrawingProgramDocument *document;
    DrawingProgramLayerRasterStore *rasters;
    DrawingProgramHistory *history;
    DrawingProgramSelectionState *selection;
    DrawingProgramClipboardState *clipboard;
    uint32_t active_layer_id;
} StageGState;

static uint64_t digest_mix(uint64_t hash, uint64_t value) {
    hash ^= value;
    hash *= UINT64_C(1099511628211);
    return hash;
}

static uint64_t raster_digest(const StageGState *state) {
    const DrawingProgramRasterSample *samples = 0;
    uint32_t count = 0u;
    uint64_t hash = UINT64_C(1469598103934665603);
    uint32_t i;
    if (drawing_program_layer_raster_store_export_layer_or_legacy_base(
            state->rasters, state->document, state->active_layer_id, &samples, &count).code != CORE_OK ||
        !samples) {
        return 0u;
    }
    hash = digest_mix(hash, count);
    for (i = 0u; i < count; ++i) {
        hash = digest_mix(hash, samples[i]);
    }
    return hash;
}

static uint64_t selection_digest(const DrawingProgramSelectionState *selection) {
    uint64_t hash = UINT64_C(1469598103934665603);
    uint32_t i;
    hash = digest_mix(hash, selection->has_payload);
    hash = digest_mix(hash, selection->origin_x);
    hash = digest_mix(hash, selection->origin_y);
    hash = digest_mix(hash, selection->width);
    hash = digest_mix(hash, selection->height);
    hash = digest_mix(hash, (uint32_t)selection->offset_x);
    hash = digest_mix(hash, (uint32_t)selection->offset_y);
    hash = digest_mix(hash, selection->payload_count);
    for (i = 0u; i < selection->payload_count; ++i) {
        hash = digest_mix(hash, selection->payload_mask[i]);
        hash = digest_mix(hash, selection->payload_value[i]);
    }
    return hash;
}

static int checkpoint(const char *name, const StageGState *state) {
    uint32_t cursor_units = 0u;
    uint32_t count_units = 0u;
    uint64_t revision = 0u;
    uint32_t dx = 0u, dy = 0u, dw = 0u, dh = 0u;
    int dirty = drawing_program_layer_raster_store_export_layer_dirty_rect(
        state->rasters, state->active_layer_id, &dx, &dy, &dw, &dh).code == CORE_OK;
    drawing_program_history_query_units(state->history, &cursor_units, &count_units);
    (void)drawing_program_layer_raster_store_export_layer_revision(
        state->rasters, state->active_layer_id, &revision);
    printf("TRACE|1|%s|doc=%ux%u|layers=%u|revision=%" PRIu64
           "|layer_revision=%" PRIu64 "|history=%u/%u/%u/%u|dirty=%d,%u,%u,%u,%u"
           "|selection=%u,%u,%u,%u,%u,%d,%d,%u,%016" PRIx64
           "|raster=%016" PRIx64 "\n",
           name,
           state->document->raster_width,
           state->document->raster_height,
           state->document->layer_count,
           state->document->content_revision,
           revision,
           state->history->cursor,
           state->history->count,
           cursor_units,
           count_units,
           dirty, dx, dy, dw, dh,
           state->selection->has_payload,
           state->selection->origin_x,
           state->selection->origin_y,
           state->selection->width,
           state->selection->height,
           state->selection->offset_x,
           state->selection->offset_y,
           state->selection->payload_count,
           selection_digest(state->selection),
           raster_digest(state));
    return 1;
}

static int require_ok(CoreResult result, const char *operation) {
    if (result.code == CORE_OK) return 1;
    fprintf(stderr, "stage_g operation failed: %s code=%d message=%s\n",
            operation, (int)result.code, result.message ? result.message : "");
    return 0;
}

static int seed_rect(StageGState *state,
                     uint32_t x0,
                     uint32_t y0,
                     uint32_t width,
                     uint32_t height,
                     uint32_t salt) {
    uint32_t x, y;
    for (y = 0u; y < height; ++y) {
        for (x = 0u; x < width; ++x) {
            DrawingProgramRasterSample value =
                drawing_program_color_value_from_index((x * 17u + y * 31u + salt) & 15u);
            if (!require_ok(drawing_program_history_apply_set_sample_value(
                                state->history, state->document, state->rasters,
                                state->active_layer_id, x0 + x, y0 + y, value),
                            "seed_rect")) return 0;
        }
    }
    return 1;
}

static int seed_rect_direct(StageGState *state,
                            uint32_t x0,
                            uint32_t y0,
                            uint32_t width,
                            uint32_t height,
                            uint32_t salt) {
    uint32_t x, y;
    for (y = 0u; y < height; ++y) {
        for (x = 0u; x < width; ++x) {
            DrawingProgramRasterSample value =
                drawing_program_color_value_from_index((x * 17u + y * 31u + salt) & 15u);
            if (!require_ok(drawing_program_layer_raster_store_sample_write(
                                state->rasters, state->active_layer_id, x0 + x, y0 + y, value, 0),
                            "seed_rect_direct_layer") ||
                !require_ok(drawing_program_document_sample_write(
                                state->document, x0 + x, y0 + y, value, 0),
                            "seed_rect_direct_document")) return 0;
        }
    }
    return 1;
}

static int run_bite1(StageGState *state) {
    DrawingProgramHistoryRasterDeltaEntry *deltas;
    uint32_t i;
    if (!checkpoint("b1_init", state)) return 0;
    if (!seed_rect(state, 4u, 5u, 8u, 6u, 3u)) return 0;
    if (!checkpoint("b1_small_seed", state)) return 0;
    if (!drawing_program_selection_capture_from_rect(
            state->document, state->rasters, state->active_layer_id,
            state->selection, 4, 5, 8u, 6u)) return 0;
    drawing_program_selection_begin_move_tracking(state->selection, 5u, 6u);
    drawing_program_selection_update_move_offset(state->selection, 13u, 15u);
    if (!require_ok(drawing_program_selection_commit_move(
                        state->document, state->rasters, state->active_layer_id,
                        state->history, state->selection), "small_move")) return 0;
    if (!checkpoint("b1_small_move", state)) return 0;

    drawing_program_clipboard_reset(state->clipboard);
    if (!drawing_program_selection_copy_payload(state->selection, state->clipboard)) return 0;
    if (!require_ok(drawing_program_selection_cut_to_clipboard(
                        state->document, state->rasters, state->active_layer_id,
                        state->history, state->selection, state->clipboard), "small_cut")) return 0;
    if (!require_ok(drawing_program_selection_paste_from_clipboard(
                        state->document, state->rasters, state->active_layer_id,
                        state->history, state->selection, state->clipboard, 30, 40), "small_paste")) return 0;
    if (!checkpoint("b1_cut_paste", state)) return 0;

    if (!seed_rect_direct(state, 128u, 128u, 512u, 512u, 9u)) return 0;
    if (!drawing_program_selection_capture_from_rect(
            state->document, state->rasters, state->active_layer_id,
            state->selection, 128, 128, 512u, 512u)) return 0;
    if (!checkpoint("b1_pressure_capture", state)) return 0;

    deltas = (DrawingProgramHistoryRasterDeltaEntry *)calloc(4097u, sizeof(*deltas));
    if (!deltas) return 0;
    for (i = 0u; i < 4097u; ++i) {
        deltas[i].sample_index = (900u * state->document->raster_width) + i;
        deltas[i].previous_sample_value = drawing_program_color_eraser_value();
        deltas[i].new_sample_value = drawing_program_color_value_from_index(i & 15u);
    }
    if (!require_ok(drawing_program_history_apply_raster_delta_block(
                        state->history, state->document, state->rasters,
                        state->active_layer_id, deltas, 4097u), "flush_boundary_delta")) {
        free(deltas);
        return 0;
    }
    free(deltas);
    if (!checkpoint("b1_flush_boundary", state)) return 0;
    if (!require_ok(drawing_program_history_undo(
                        state->history, state->document, state->rasters, 0), "undo")) return 0;
    if (!checkpoint("b1_undo", state)) return 0;
    if (!require_ok(drawing_program_history_redo(
                        state->history, state->document, state->rasters, 0), "redo")) return 0;
    return checkpoint("b1_redo", state);
}

int main(int argc, char **argv) {
    StageGState state;
    uint32_t extra_layer_id = 0u;
    int ok;
    (void)argc;
    (void)argv;
    state.document = (DrawingProgramDocument *)calloc(1u, sizeof(*state.document));
    state.rasters = (DrawingProgramLayerRasterStore *)calloc(1u, sizeof(*state.rasters));
    state.history = (DrawingProgramHistory *)calloc(1u, sizeof(*state.history));
    state.selection = (DrawingProgramSelectionState *)calloc(1u, sizeof(*state.selection));
    state.clipboard = (DrawingProgramClipboardState *)calloc(1u, sizeof(*state.clipboard));
    if (!state.document || !state.rasters || !state.history || !state.selection || !state.clipboard) return 70;
    if (!require_ok(drawing_program_document_init_with_shape(state.document, 1024u, 1024u, 1u), "document_init")) return 71;
    state.active_layer_id = state.document->layers[0].layer_id;
    if (!require_ok(drawing_program_document_add_layer(state.document, "Operational", &extra_layer_id), "add_layer")) return 72;
    state.active_layer_id = extra_layer_id;
    if (!require_ok(drawing_program_layer_raster_store_init_from_document(state.rasters, state.document), "raster_init")) return 73;
    drawing_program_history_init(state.history);
    drawing_program_selection_reset(state.selection);
    drawing_program_clipboard_reset(state.clipboard);
    ok = run_bite1(&state);
    drawing_program_layer_raster_store_dispose(state.rasters);
    free(state.clipboard);
    free(state.selection);
    free(state.history);
    free(state.rasters);
    free(state.document);
    return ok ? 0 : 74;
}
