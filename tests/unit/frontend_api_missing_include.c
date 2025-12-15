#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fisics_frontend.h"

int main(void) {
    const char* path = "missing_include.c";
    const char* src =
        "#include \"missing_header_xyz.h\"\n"
        "int main(void) { return 0; }\n";

    FisicsAnalysisResult res = {0};
    FisicsFrontendOptions opts = {0};
    /* No include paths; lenient include handling is enabled by fisics_analyze_buffer. */

    if (!fisics_analyze_buffer(path, src, strlen(src), &opts, &res)) {
        fprintf(stderr, "analysis unexpectedly failed\n");
        return 1;
    }

    if (res.diag_count == 0) {
        fprintf(stderr, "expected missing-include diagnostic\n");
        fisics_free_analysis_result(&res);
        return 1;
    }

    int found = 0;
    for (size_t i = 0; i < res.diag_count; ++i) {
        const FisicsDiagnostic* d = &res.diagnostics[i];
        if (d->file_path && strstr(d->file_path, "missing_include.c")) {
            found = 1;
            break;
        }
    }

    fisics_free_analysis_result(&res);

    if (!found) {
        fprintf(stderr, "missing-include diagnostic did not point to the including file\n");
        return 1;
    }

    return 0;
}
