#include <stdio.h>
#include <string.h>

#include "drawing_program/drawing_program_document.h"
#include "drawing_program/drawing_program_layer_raster.h"

static int expect_ok(CoreResult result, const char *label) {
    if (result.code == CORE_OK) {
        return 1;
    }
    fprintf(stderr,
            "drawing_program_stageb_document_layer_raster_subset: %s failed code=%d message=%s\n",
            label,
            (int)result.code,
            result.message ? result.message : "n/a");
    return 0;
}

int main(void) {
    DrawingProgramDocument doc;
    DrawingProgramLayerRasterStore store;
    CoreResult result;
    uint32_t layer_id = 0u;
    uint8_t prev = 0u;
    uint8_t value = 0u;
    int rc = 1;

    memset(&store, 0, sizeof(store));

    result = drawing_program_document_init_with_shape(&doc, 32u, 32u, 1u);
    if (!expect_ok(result, "document_init_with_shape")) {
        goto cleanup;
    }

    result = drawing_program_document_add_layer(&doc, "Ink", &layer_id);
    if (!expect_ok(result, "document_add_layer")) {
        goto cleanup;
    }
    if (layer_id == 0u) {
        fprintf(stderr, "drawing_program_stageb_document_layer_raster_subset: invalid layer id\n");
        goto cleanup;
    }

    result = drawing_program_layer_raster_store_init_from_document(&store, &doc);
    if (!expect_ok(result, "layer_raster_store_init_from_document")) {
        goto cleanup;
    }

    result = drawing_program_layer_raster_store_sample_write(&store, layer_id, 3u, 4u, 77u, &prev);
    if (!expect_ok(result, "layer_raster_store_sample_write")) {
        goto cleanup;
    }
    result = drawing_program_layer_raster_store_sample_read(&store, layer_id, 3u, 4u, &value);
    if (!expect_ok(result, "layer_raster_store_sample_read")) {
        goto cleanup;
    }
    if (value != 77u) {
        fprintf(stderr,
                "drawing_program_stageb_document_layer_raster_subset: expected layer value=77 got=%u\n",
                (unsigned)value);
        goto cleanup;
    }

    result = drawing_program_document_sample_write(&doc, 3u, 4u, 88u, &prev);
    if (!expect_ok(result, "document_sample_write")) {
        goto cleanup;
    }
    result = drawing_program_document_sample_read(&doc, 3u, 4u, &value);
    if (!expect_ok(result, "document_sample_read")) {
        goto cleanup;
    }
    if (value != 88u) {
        fprintf(stderr,
                "drawing_program_stageb_document_layer_raster_subset: expected document value=88 got=%u\n",
                (unsigned)value);
        goto cleanup;
    }

    puts("drawing_program_stageb_document_layer_raster_subset: success");
    rc = 0;

cleanup:
    drawing_program_layer_raster_store_dispose(&store);
    return rc;
}
