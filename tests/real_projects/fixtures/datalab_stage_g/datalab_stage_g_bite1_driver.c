#include "data/dataset_builders.h"
#include "data/input_file_loader.h"
#include "data/pack_inspector.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint64_t hash_bytes(uint64_t hash, const void *data, size_t size) {
    const unsigned char *bytes = (const unsigned char *)data;
    size_t i = 0u;
    for (i = 0u; i < size; ++i) {
        hash ^= (uint64_t)bytes[i];
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

static uint64_t frame_digest(const DatalabFrame *frame) {
    uint64_t hash = UINT64_C(1469598103934665603);
    size_t cell_count = 0u;
    size_t i = 0u;
    if (!frame) return 0u;
    hash = hash_bytes(hash, &frame->profile, sizeof(frame->profile));
    hash = hash_bytes(hash, &frame->width, sizeof(frame->width));
    hash = hash_bytes(hash, &frame->height, sizeof(frame->height));
    hash = hash_bytes(hash, &frame->frame_index, sizeof(frame->frame_index));
    hash = hash_bytes(hash, &frame->obstacle_mask_crc32, sizeof(frame->obstacle_mask_crc32));
    hash = hash_bytes(hash, &frame->chunk_count, sizeof(frame->chunk_count));
    cell_count = (size_t)frame->width * (size_t)frame->height;
    for (i = 0u; i < cell_count; ++i) {
        hash = hash_bytes(hash, &frame->density[i], sizeof(frame->density[i]));
        hash = hash_bytes(hash, &frame->velx[i], sizeof(frame->velx[i]));
        hash = hash_bytes(hash, &frame->vely[i], sizeof(frame->vely[i]));
    }
    return hash;
}

static void checkpoint(const char *name, const char *detail, uint64_t digest) {
    printf("TRACE|1|%s|detail=%s|digest=%016llx|result=1\n",
           name,
           detail ? detail : "ok",
           (unsigned long long)digest);
}

int main(void) {
    const char *pack_path = getenv("DATALAB_STAGEG_PACK_INPUT");
    const char *invalid_path = getenv("DATALAB_STAGEG_INVALID_INPUT");
    DatalabFrame frame;
    DatalabFrame invalid_frame;
    DatalabPackInspection inspection;
    CoreDataset dataset;
    CoreResult result;
    uint64_t digest = 0u;
    FILE *canonical = NULL;
    size_t row_bytes = 0u;
    size_t image_bytes = 0u;

    if (!pack_path || !invalid_path) return 11;
    datalab_frame_init(&frame);
    datalab_frame_init(&invalid_frame);
    memset(&inspection, 0, sizeof(inspection));
    core_dataset_init(&dataset);

    if (!datalab_input_file_is_pack(pack_path) ||
        datalab_input_file_is_bmp(pack_path) ||
        datalab_input_file_is_png(pack_path) ||
        !datalab_input_file_is_supported(pack_path)) return 12;
    checkpoint("b1_classified", "pack_supported", 0u);

    result = datalab_load_input_file(pack_path, &frame);
    if (result.code != CORE_OK || frame.profile != DATALAB_PROFILE_PHYSICS ||
        frame.width != 96u || frame.height != 96u || !frame.density || !frame.velx || !frame.vely) return 13;
    digest = frame_digest(&frame);
    checkpoint("b1_loaded", "physics_96x96", digest);

    result = datalab_inspect_pack(pack_path, &inspection);
    if (result.code != CORE_OK || inspection.chunk_count != frame.chunk_count ||
        inspection.listed_chunk_count == 0u || inspection.family[0] == '\0') return 14;
    checkpoint("b1_inspected", "physics_field_2d", digest);

    result = datalab_build_dataset_from_frame(&frame, &dataset);
    if (result.code != CORE_OK || dataset.item_count != 2u || dataset.metadata_count != 3u ||
        !core_dataset_find(&dataset, "density") || !core_dataset_find(&dataset, "velocity")) return 15;
    checkpoint("b1_dataset", "density_velocity", digest);

    result = datalab_input_image_bounds(16384u, 1u, &row_bytes, &image_bytes);
    if (result.code != CORE_OK || row_bytes != 65536u || image_bytes != 65536u) return 16;
    result = datalab_input_image_bounds(16385u, 1u, &row_bytes, &image_bytes);
    if (result.code == CORE_OK) return 17;
    checkpoint("b1_bounds", "max_accept_overflow_reject", digest);

    result = datalab_load_input_file(invalid_path, &invalid_frame);
    if (result.code == CORE_OK || datalab_input_file_is_supported(invalid_path)) return 18;
    checkpoint("b1_invalid", "unsupported_rejected", digest);

    canonical = fopen("load.canonical", "wb");
    if (!canonical) return 19;
    fprintf(canonical,
            "profile=%d\nwidth=%u\nheight=%u\nframe=%llu\nchunks=%zu\ninspection=%s\n"
            "items=%zu\nmetadata=%zu\ncrc=%u\ndigest=%016llx\n",
            (int)frame.profile,
            frame.width,
            frame.height,
            (unsigned long long)frame.frame_index,
            frame.chunk_count,
            inspection.family,
            dataset.item_count,
            dataset.metadata_count,
            frame.obstacle_mask_crc32,
            (unsigned long long)digest);
    if (fclose(canonical) != 0) return 20;
    checkpoint("b1_canonical", "artifact_written", digest);

    core_dataset_free(&dataset);
    datalab_frame_free(&frame);
    datalab_frame_free(&invalid_frame);
    checkpoint("b1_shutdown", "state_freed", 0u);
    return 0;
}
