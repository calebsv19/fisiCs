#include "app/data_paths.h"

#include <stdio.h>
#include <string.h>

int main(void) {
    char built[256];
    char shape_buf[256];
    char import_buf[256];
    const char** roots = NULL;
    int ok = 1;

    if (strcmp(ray_tracing_default_input_root(), "config") != 0) ok = 0;
    if (strcmp(ray_tracing_default_output_root(), "data/runtime") != 0) ok = 0;
    if (strcmp(ray_tracing_default_shape_asset_dir(), "config/objects") != 0) ok = 0;
    if (strcmp(ray_tracing_default_import_dir(), "import") != 0) ok = 0;

    if (!ray_tracing_compose_path("config", "scene_bundle.json", built, sizeof(built))) {
        ok = 0;
        built[0] = '\0';
    }
    if (strcmp(built, "config/scene_bundle.json") != 0) ok = 0;

    if (ray_tracing_resolve_shape_asset_dir("custom/assets", shape_buf, sizeof(shape_buf)) !=
        (const char*)"custom/assets") {
        ok = 0;
    }

    {
        const char* import_dir = ray_tracing_resolve_import_dir(import_buf, sizeof(import_buf));
        if (!import_dir || !import_dir[0]) ok = 0;
    }

    {
        size_t count = ray_tracing_manifest_default_roots(&roots);
        if (count == 0 || !roots || !roots[0] || !roots[0][0]) ok = 0;
        printf("built=%s roots=%zu ok=%d\n", built, count, ok);
    }

    return ok ? 0 : 1;
}
