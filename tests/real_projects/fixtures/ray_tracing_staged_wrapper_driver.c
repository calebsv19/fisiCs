#include "ray_tracing/ray_tracing_app_main.h"

#include <stdio.h>
#include <string.h>

static int ray_tracing_stage_smoke_legacy_entry(int argc, char **argv) {
    if (argc >= 2 && argv && argv[1] && strcmp(argv[1], "--smoke") == 0) {
        return 0;
    }
    return 2;
}

int main(void) {
    char arg0[] = "ray_tracing";
    char arg1[] = "--smoke";
    char *argv[] = {arg0, arg1, NULL};
    int rc = 0;

    ray_tracing_app_set_legacy_entry(ray_tracing_stage_smoke_legacy_entry);
    rc = ray_tracing_app_main(2, argv);
    printf("stage_d_smoke_rc=%d\n", rc);
    return rc;
}
