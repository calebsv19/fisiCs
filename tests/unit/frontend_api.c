#include <stdio.h>
#include <string.h>
#include "fisics_frontend.h"

int main(void) {
    /* baseline: missing identifier produces diagnostics and tokens */
    const char* src = "int main(){ return missing; }\n";
    FisicsAnalysisResult res = {0};
    if (!fisics_analyze_buffer("inline_input.c", src, strlen(src), NULL, &res)) {
        fprintf(stderr, "fisics_analyze_buffer failed\n");
        return 1;
    }
    if (res.token_count == 0) {
        fprintf(stderr, "expected tokens\n");
        fisics_free_analysis_result(&res);
        return 1;
    }
    if (res.diag_count == 0) {
        fprintf(stderr, "expected diagnostics\n");
        fisics_free_analysis_result(&res);
        return 1;
    }
    fisics_free_analysis_result(&res);
    return 0;
}
