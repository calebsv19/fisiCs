#include "map_forge/map_forge_app_main.h"

#include <stdio.h>

int app_run_legacy(void) {
    return 0;
}

int main(void) {
    int rc = map_forge_app_main();
    printf("stage_c_fullbuild_rc=%d\n", rc);
    return rc;
}
