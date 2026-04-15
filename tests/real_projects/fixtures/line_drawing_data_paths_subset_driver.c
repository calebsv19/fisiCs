#include "Core/data_paths.h"

#include <stdio.h>
#include <string.h>

int main(void) {
    LineDrawingDataPaths paths;
    char built[LINE_DRAWING_PATH_CAP];
    int ok = 1;

    LineDrawingDataPaths_SetDefaults(&paths);

    if (strcmp(paths.input_root, "config") != 0) ok = 0;
    if (strcmp(paths.output_root, "export") != 0) ok = 0;
    if (strcmp(paths.layout_root, "config") != 0) ok = 0;

    if (!LineDrawingDataPaths_BuildPath(built, sizeof(built), paths.input_root, "demo.layout")) {
        ok = 0;
        built[0] = '\0';
    }

    if (strcmp(LineDrawingDataPaths_DefaultInputRoot(), "config") != 0) ok = 0;
    if (strcmp(LineDrawingDataPaths_DefaultOutputRoot(), "export") != 0) ok = 0;
    if (strcmp(LineDrawingDataPaths_DefaultLayoutRoot(), "config") != 0) ok = 0;

    printf("in=%s out=%s layout=%s path=%s ok=%d\n",
           paths.input_root,
           paths.output_root,
           paths.layout_root,
           built,
           ok);
    return ok ? 0 : 1;
}
