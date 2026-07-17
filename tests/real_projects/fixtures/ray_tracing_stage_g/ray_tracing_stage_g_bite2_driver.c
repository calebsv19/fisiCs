#include "app/scene_project_render_request.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint64_t hash_text(uint64_t hash, const char *text) {
    const unsigned char *cursor = (const unsigned char *)(text ? text : "");
    while (*cursor) {
        hash ^= (uint64_t)*cursor++;
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

static void checkpoint(const char *name, const char *detail, uint64_t digest) {
    printf("TRACE|1|%s|detail=%s|digest=%016llx|result=1\n",
           name, detail, (unsigned long long)digest);
}

static int copy_file(const char *source, const char *destination, char *text, size_t size) {
    FILE *input = fopen(source, "rb");
    FILE *output;
    size_t used;
    if (!input) return 0;
    used = fread(text, 1u, size - 1u, input);
    if (ferror(input) || !feof(input) || fclose(input) != 0) return 0;
    text[used] = '\0';
    output = fopen(destination, "wb");
    if (!output) return 0;
    if (fwrite(text, 1u, used, output) != used) {
        fclose(output);
        return 0;
    }
    return fclose(output) == 0;
}

int main(void) {
    const char *root = getenv("RAY_TRACING_STAGEG_PROJECT_ROOT");
    const char *unsafe_runtime = getenv("RAY_TRACING_STAGEG_UNSAFE_RUNTIME");
    char runtime[PATH_MAX];
    char request_path[PATH_MAX];
    char error[512];
    char raw[16384];
    RayTracingSceneProjectRenderRequest request;
    RayTracingSceneProjectRenderRequest reloaded;
    RayTracingSceneProjectRenderRequest unsafe;
    uint64_t digest = UINT64_C(1469598103934665603);
    FILE *canonical;

    if (!root || !unsafe_runtime) return 11;
    snprintf(runtime, sizeof(runtime), "%s/scene_runtime.json", root);
    if (!ray_tracing_scene_project_render_request_resolve(runtime, NULL, &request,
                                                          error, sizeof(error)))
        return 12;
    if (!request.project_backed || !request.project_owned || !request.request_exists ||
        request.simulation_start_frame != 2 || request.simulation_frame_count != 4)
        return 13;
    checkpoint("b2_resolved", "project_owned_existing", 0u);

    if (!ray_tracing_scene_project_render_request_write(&request, 7, 9, 3,
                                                        error, sizeof(error)))
        return 14;
    checkpoint("b2_saved", "window_7_9_3", 0u);

    snprintf(request_path, sizeof(request_path), "%s/ray_tracing/render_request.json", root);
    if (!copy_file(request_path, "render_request.json", raw, sizeof(raw))) return 15;
    if (!strstr(raw, "custom_stage_g") || !strstr(raw, "unknown-field-survives")) return 16;
    digest = hash_text(digest, "window=7:9:3|unknown=1|owned=1");
    checkpoint("b2_preserved", "unknown_fields", digest);

    memset(&request, 0, sizeof(request));
    checkpoint("b2_destroyed", "state_cleared", digest);
    if (!ray_tracing_scene_project_render_request_resolve(runtime, NULL, &reloaded,
                                                          error, sizeof(error)))
        return 17;
    if (reloaded.simulation_start_frame != 7 || reloaded.simulation_frame_count != 9 ||
        reloaded.simulation_frame_stride != 3 || !reloaded.project_owned)
        return 18;
    checkpoint("b2_reloaded", "window_restored", digest);
    checkpoint("b2_compared", "semantic_state_equal", digest);

    if (ray_tracing_scene_project_render_request_resolve(unsafe_runtime, NULL, &unsafe,
                                                         error, sizeof(error)))
        return 19;
    if (ray_tracing_scene_project_render_request_write(&reloaded, -1, 0, 0,
                                                       error, sizeof(error)))
        return 20;
    checkpoint("b2_invalid", "unsafe_and_window_rejected", digest);

    canonical = fopen("roundtrip.canonical", "wb");
    if (!canonical) return 21;
    fprintf(canonical,
            "schema=ray_tracing_stage_g_roundtrip_v1\nproject_backed=1\n"
            "project_owned=1\nrequest_exists=1\nrequest_relpath=%s\n"
            "physics_cache=%s\noutput_root=%s\nwindow=%d:%d:%d\n"
            "unknown_fields_preserved=1\ndigest=%016llx\n",
            reloaded.request_relpath, reloaded.physics_cache_relpath,
            reloaded.output_root_relpath, reloaded.simulation_start_frame,
            reloaded.simulation_frame_count, reloaded.simulation_frame_stride,
            (unsigned long long)digest);
    if (fclose(canonical) != 0) return 22;
    checkpoint("b2_canonical", "artifacts_written", digest);
    memset(&reloaded, 0, sizeof(reloaded));
    checkpoint("b2_shutdown", "state_freed", 0u);
    return 0;
}
