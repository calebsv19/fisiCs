#include "physics_sim/physics_sim_app_main.h"

#include <stdio.h>
#include <string.h>

static int g_legacy_calls = 0;

static int physics_sim_legacy_stub(int argc, char **argv) {
    g_legacy_calls++;
    if (argc >= 2 && argv && argv[1] && strcmp(argv[1], "--smoke") == 0) {
        return 0;
    }
    return 3;
}

int main(void) {
    int ok = 1;
    char arg0[] = "physics_sim";
    char arg1[] = "--smoke";
    char *argv[] = {arg0, arg1, NULL};

    physics_sim_app_set_legacy_entry(physics_sim_legacy_stub);
    if (!physics_sim_app_bootstrap()) ok = 0;
    if (!physics_sim_app_config_load()) ok = 0;
    if (!physics_sim_app_state_seed()) ok = 0;
    if (!physics_sim_app_subsystems_init()) ok = 0;
    if (!physics_sim_runtime_start()) ok = 0;
    if (physics_sim_app_run_loop() != 0) ok = 0;
    physics_sim_app_shutdown();

    physics_sim_app_set_legacy_entry(physics_sim_legacy_stub);
    if (physics_sim_app_main(2, argv) != 0) ok = 0;

    printf("legacy_calls=%d ok=%d\n", g_legacy_calls, ok);
    return ok ? 0 : 1;
}
