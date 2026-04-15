#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "mem_console_state.h"

char *SDL_GetBasePath(void) {
    return 0;
}

void SDL_free(void *mem) {
    (void)mem;
}

int main(void) {
    char parent[256];
    char output_root[1024];
    char input_root[1024];
    char active_db_path[1024];
    char resolved_default_db[1024];
    char *argv_ok[] = { "mem_console", "--db", "x.sqlite", "--kernel-bridge" };
    char *argv_bad[] = { "mem_console", "--unknown" };

    assert(mem_console_path_parent("/tmp/mem_console_stageb/a.sqlite", parent, sizeof(parent)) == 1);
    assert(strcmp(parent, "/tmp/mem_console_stageb") == 0);

    assert(has_flag(4, argv_ok, "--kernel-bridge") == 1);
    assert(has_unknown_flag(4, argv_ok) == 0);
    assert(has_unknown_flag(2, argv_bad) == 1);
    assert(find_flag_value(4, argv_ok, "--db") != 0);
    assert(strcmp(find_flag_value(4, argv_ok, "--db"), "x.sqlite") == 0);

    assert(mem_console_path_contract_normalize(0,
                                               "/tmp/mem_console_stageb_out",
                                               "/tmp/mem_console_stageb_out/data/default.sqlite",
                                               input_root,
                                               sizeof(input_root),
                                               output_root,
                                               sizeof(output_root),
                                               active_db_path,
                                               sizeof(active_db_path)) == 1);
    assert(output_root[0] != '\0');
    assert(input_root[0] != '\0');
    assert(active_db_path[0] != '\0');
    assert(mem_console_path_exists(output_root) == 1);

    assert(resolve_default_db_path(resolved_default_db, sizeof(resolved_default_db)) == 1);
    assert(resolved_default_db[0] != '\0');

    return 0;
}
