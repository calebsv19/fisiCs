#include "map_forge/map_forge_app_main.h"
#include "app/app_headless.h"

#include <stdio.h>

int app_run_legacy(void) {
    return 0;
}

bool map_forge_headless_args_parse(int argc,
                                   char **argv,
                                   MapForgeHeadlessCliOptions *out_options,
                                   char *out_error,
                                   size_t out_error_size) {
    (void)argc;
    (void)argv;
    (void)out_error;
    (void)out_error_size;
    if (out_options) {
        *out_options = (MapForgeHeadlessCliOptions){0};
    }
    return true;
}

void map_forge_headless_args_usage(const char *program_name,
                                   char *out_text,
                                   size_t out_text_size) {
    (void)program_name;
    if (out_text && out_text_size > 0) {
        out_text[0] = '\0';
    }
}

int map_forge_headless_run(const MapForgeHeadlessCliOptions *options,
                           int argc,
                           char **argv) {
    (void)options;
    (void)argc;
    (void)argv;
    return 0;
}

int main(int argc, char **argv) {
    int rc = map_forge_app_main(argc, argv);
    printf("stage_c_fullbuild_rc=%d\n", rc);
    return rc;
}
