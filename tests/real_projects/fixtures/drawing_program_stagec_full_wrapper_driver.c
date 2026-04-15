#include <stdio.h>

#include "drawing_program/drawing_program_app_main.h"

static int expect_ok(CoreResult result, const char *label) {
    if (result.code == CORE_OK) {
        return 1;
    }
    fprintf(stderr,
            "drawing_program_stagec_full_wrapper_driver: %s failed code=%d message=%s\n",
            label,
            (int)result.code,
            result.message ? result.message : "n/a");
    return 0;
}

int main(void) {
    DrawingProgramAppContext ctx;
    char arg0[] = "drawing_program_stagec";
    char arg1[] = "--headless";
    char arg2[] = "--smoke-frames";
    char arg3[] = "1";
    char arg4[] = "--no-persist";
    char *argv[] = { arg0, arg1, arg2, arg3, arg4, 0 };

    if (!expect_ok(drawing_program_app_bootstrap(&ctx, 5, argv), "bootstrap")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_config_load(&ctx), "config_load")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_state_seed(&ctx), "state_seed")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_subsystems_init(&ctx), "subsystems_init")) {
        return 1;
    }
    if (!expect_ok(drawing_program_runtime_start(&ctx), "runtime_start")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_run_loop(&ctx), "run_loop")) {
        return 1;
    }
    if (!expect_ok(drawing_program_app_shutdown(&ctx), "shutdown")) {
        return 1;
    }

    puts("drawing_program_stagec_full_wrapper_driver: success");
    return 0;
}
