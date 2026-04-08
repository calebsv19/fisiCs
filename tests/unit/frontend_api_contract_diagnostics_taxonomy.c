#include <stdio.h>
#include <string.h>

#include "fisics_frontend.h"

int main(void) {
    const char* src = "int main(void){ return missing_symbol; }\n";
    FisicsAnalysisResult res = (FisicsAnalysisResult){0};

    if (!fisics_analyze_buffer("contract_diag_taxonomy.c", src, strlen(src), NULL, &res)) {
        fprintf(stderr, "fisics_analyze_buffer failed\n");
        return 1;
    }
    if (res.diag_count == 0) {
        fprintf(stderr, "expected at least one diagnostic\n");
        fisics_free_analysis_result(&res);
        return 1;
    }

    const FisicsDiagnostic* d = &res.diagnostics[0];
    if (d->severity_id < FISICS_DIAG_SEVERITY_INFO || d->severity_id > FISICS_DIAG_SEVERITY_ERROR) {
        fprintf(stderr, "unexpected severity_id=%d\n", d->severity_id);
        fisics_free_analysis_result(&res);
        return 1;
    }
    if (d->code_id != d->code) {
        fprintf(stderr, "expected code_id to mirror code (%d != %d)\n", d->code_id, d->code);
        fisics_free_analysis_result(&res);
        return 1;
    }
    if (d->category_id == FISICS_DIAG_CATEGORY_UNKNOWN) {
        fprintf(stderr, "expected non-unknown category for first diagnostic\n");
        fisics_free_analysis_result(&res);
        return 1;
    }

    fisics_free_analysis_result(&res);
    return 0;
}
