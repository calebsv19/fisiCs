#include <assert.h>
#include <string.h>

#include "mem_console_prefs.h"

int mem_console_ensure_parent_directory(const char *path) {
    (void)path;
    return 1;
}

int mem_console_resolve_app_data_dir(char *out_path, size_t out_cap) {
    if (!out_path || out_cap == 0u) {
        return 0;
    }
    if (out_cap < 2u) {
        return 0;
    }
    out_path[0] = '.';
    out_path[1] = '\0';
    return 1;
}

char *SDL_GetBasePath(void) {
    return 0;
}

void SDL_free(void *mem) {
    (void)mem;
}

int main(void) {
    char loaded_db[1024];
    char loaded_input[1024];
    char loaded_output[1024];
    char loaded_active[1024];
    const char *prefs_path = "/tmp/mem_console_stageb_out/mem_console.app.pack";
    const char *output_root = "/tmp/mem_console_stageb_out";
    const char *db_path = "/tmp/mem_console_stageb_out/data/default.sqlite";
    const char *input_root = "/tmp/mem_console_stageb_in";
    const char *active_db = "/tmp/mem_console_stageb_out/data/default.sqlite";
    CoreResult result;

    result = mem_console_app_prefs_save(prefs_path, db_path, input_root, output_root, active_db);
    assert(result.code == CORE_OK);

    result = mem_console_app_prefs_load(prefs_path,
                                        loaded_db,
                                        sizeof(loaded_db),
                                        loaded_input,
                                        sizeof(loaded_input),
                                        loaded_output,
                                        sizeof(loaded_output),
                                        loaded_active,
                                        sizeof(loaded_active));
    assert(result.code == CORE_OK);
    assert(strcmp(loaded_db, db_path) == 0);
    assert(strcmp(loaded_input, input_root) == 0);
    assert(strcmp(loaded_output, output_root) == 0);
    assert(strcmp(loaded_active, active_db) == 0);

    return 0;
}
