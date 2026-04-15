#include "app/data_paths.h"

#include <stdio.h>
#include <string.h>

int main(void) {
    char built[256];
    char preset_buf[256];
    char shape_buf[256];
    char import_buf[256];
    int ok = 1;

    if (strcmp(physics_sim_default_config_path(), "config/app.json") != 0) ok = 0;
    if (strcmp(physics_sim_runtime_config_path(), "data/runtime/app_state.json") != 0) ok = 0;
    if (strcmp(physics_sim_default_input_root(), "config") != 0) ok = 0;
    if (strcmp(physics_sim_default_snapshot_dir(), "data/snapshots") != 0) ok = 0;

    if (!physics_sim_compose_root_path("config", "app.json", built, sizeof(built))) {
        ok = 0;
    } else if (strcmp(built, "config/app.json") != 0) {
        ok = 0;
    }

    if (physics_sim_resolve_input_root("custom_root") != (const char *)"custom_root") ok = 0;
    if (physics_sim_resolve_snapshot_output_dir("snapshots/custom") != (const char *)"snapshots/custom") ok = 0;

    {
        const char *preset = physics_sim_resolve_preset_load_path_for_root(
            "does_not_exist_root", preset_buf, sizeof(preset_buf));
        const char *shape = physics_sim_resolve_shape_asset_dir_for_root(
            "does_not_exist_root", shape_buf, sizeof(shape_buf));
        const char *import_dir = physics_sim_resolve_import_dir_for_root(
            "does_not_exist_root", import_buf, sizeof(import_buf));
        if (!preset || !preset[0] || !shape || !shape[0] || !import_dir || !import_dir[0]) ok = 0;
    }

    if (!physics_sim_ensure_runtime_dirs()) ok = 0;

    printf("built=%s ok=%d\n", built, ok);
    return ok ? 0 : 1;
}
