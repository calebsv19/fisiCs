#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fisics_frontend.h"

int main(void) {
    const char* src = "int main(){ return missing; }\n";
    FisicsAnalysisResult res = {0};
    FisicsAnalysisResult res2 = {0};

    if (setenv("FISICS_DIAG_DEDUP_GLOBAL", "1", 1) != 0) {
        fprintf(stderr, "failed to set FISICS_DIAG_DEDUP_GLOBAL\n");
        return 1;
    }

    if (!fisics_analyze_buffer("inline_input.c", src, strlen(src), NULL, &res)) {
        fprintf(stderr, "fisics_analyze_buffer failed (first)\n");
        return 1;
    }
    if (res.diag_count == 0) {
        fprintf(stderr, "expected diagnostics in first pass\n");
        fisics_free_analysis_result(&res);
        return 1;
    }
    fisics_free_analysis_result(&res);

    if (!fisics_analyze_buffer("inline_input.c", src, strlen(src), NULL, &res2)) {
        fprintf(stderr, "fisics_analyze_buffer failed (second)\n");
        return 1;
    }
    if (res2.diag_count != 0) {
        fprintf(stderr, "expected diagnostics to be deduplicated in second pass\n");
        fisics_free_analysis_result(&res2);
        return 1;
    }
    fisics_free_analysis_result(&res2);
    return 0;
}
