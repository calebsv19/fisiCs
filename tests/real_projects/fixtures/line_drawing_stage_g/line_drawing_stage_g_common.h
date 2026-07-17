#pragma once

#include "Layout/scene/layout_scene_authoring.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint64_t ld_stageg_hash_text(uint64_t hash, const char* text) {
    const unsigned char* bytes = (const unsigned char*)(text ? text : "");
    while (*bytes) {
        hash ^= (uint64_t)*bytes++;
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

static uint64_t ld_stageg_authoring_digest(const LineDrawingSceneAuthoringState* state) {
    uint64_t hash = UINT64_C(1469598103934665603);
    char field[160];
    size_t i;
    size_t p;
    if (!state) return 0u;
    snprintf(field, sizeof(field), "counts:%zu:%zu:%zu:%zu|sel:%d:%zu",
             state->light_count, state->camera_count, state->path_count,
             state->material_count, (int)state->selected_kind, state->selected_index);
    hash = ld_stageg_hash_text(hash, field);
    for (i = 0u; i < state->path_count; ++i) {
        const LineDrawingScenePath* path = &state->paths[i];
        snprintf(field, sizeof(field), "path:%s:%d:%zu:%d:%d:%.6f:%.6f:%d",
                 path->path_id, (int)path->role, path->control_point_count,
                 path->closed ? 1 : 0, (int)path->playback_mode,
                 (double)path->duration_seconds, (double)path->normalized_distance,
                 path->playing ? 1 : 0);
        hash = ld_stageg_hash_text(hash, field);
        for (p = 0u; p < path->control_point_count; ++p) {
            snprintf(field, sizeof(field), "pt:%zu:%.6f:%.6f:%.6f", p,
                     (double)path->control_points[p].x,
                     (double)path->control_points[p].y,
                     (double)path->control_points[p].z);
            hash = ld_stageg_hash_text(hash, field);
        }
    }
    for (i = 0u; i < state->camera_count; ++i) {
        const LineDrawingSceneCamera* camera = &state->cameras[i];
        snprintf(field, sizeof(field), "camera:%s:%s:%d:%.6f:%.6f:%.6f",
                 camera->camera_id, camera->path_id, (int)camera->orientation_mode,
                 (double)camera->vertical_fov_degrees,
                 (double)camera->near_clip, (double)camera->far_clip);
        hash = ld_stageg_hash_text(hash, field);
    }
    for (i = 0u; i < state->light_count; ++i) {
        const LineDrawingSceneLight* light = &state->lights[i];
        snprintf(field, sizeof(field), "light:%s:%s:%d:%d:%.6f:%.6f",
                 light->light_id, light->path_id, (int)light->kind,
                 light->enabled ? 1 : 0, (double)light->intensity,
                 (double)light->radius);
        hash = ld_stageg_hash_text(hash, field);
    }
    for (i = 0u; i < state->material_count; ++i) {
        const LineDrawingSceneMaterial* material = &state->materials[i];
        snprintf(field, sizeof(field), "material:%s:%.6f:%.6f:%.6f:%.6f",
                 material->material_id, (double)material->rgba[0],
                 (double)material->rgba[1], (double)material->rgba[2],
                 (double)material->rgba[3]);
        hash = ld_stageg_hash_text(hash, field);
    }
    return hash;
}

static void ld_stageg_trace(const char* name, const char* detail, uint64_t digest) {
    printf("TRACE|1|%s|detail=%s|digest=%016llx|result=1\n",
           name, detail ? detail : "ok", (unsigned long long)digest);
}

static int ld_stageg_write_state(const LineDrawingSceneAuthoringState* state,
                                 const char* path,
                                 const char* label) {
    FILE* file = fopen(path, "wb");
    size_t i;
    size_t p;
    if (!file || !state) return 0;
    fprintf(file, "schema=line_drawing_stage_g_state_v1\nlabel=%s\n",
            label ? label : "state");
    fprintf(file, "lights=%zu\ncameras=%zu\npaths=%zu\nmaterials=%zu\n",
            state->light_count, state->camera_count, state->path_count,
            state->material_count);
    for (i = 0u; i < state->path_count; ++i) {
        const LineDrawingScenePath* path_state = &state->paths[i];
        fprintf(file, "path[%zu]=%s|role=%d|points=%zu|closed=%d|playback=%d|duration=%.6f|distance=%.6f|playing=%d\n",
                i, path_state->path_id, (int)path_state->role,
                path_state->control_point_count, path_state->closed ? 1 : 0,
                (int)path_state->playback_mode,
                (double)path_state->duration_seconds,
                (double)path_state->normalized_distance,
                path_state->playing ? 1 : 0);
        for (p = 0u; p < path_state->control_point_count; ++p) {
            fprintf(file, "point[%zu,%zu]=%.6f,%.6f,%.6f\n", i, p,
                    (double)path_state->control_points[p].x,
                    (double)path_state->control_points[p].y,
                    (double)path_state->control_points[p].z);
        }
    }
    fprintf(file, "digest=%016llx\n",
            (unsigned long long)ld_stageg_authoring_digest(state));
    return fclose(file) == 0;
}
