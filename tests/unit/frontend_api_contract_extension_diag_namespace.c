#include <stdio.h>
#include <string.h>

#include "fisics_frontend.h"

int main(void) {
    const char* src = "double distance [[fisics::dim(m)]] = 1.0;\n";
    FisicsAnalysisResult res = (FisicsAnalysisResult){0};

    if (!fisics_analyze_buffer("contract_extension_diag_namespace.c", src, strlen(src), NULL, &res)) {
        fprintf(stderr, "fisics_analyze_buffer failed\n");
        return 1;
    }
    if (res.diag_count == 0) {
        fprintf(stderr, "expected extension diagnostic for disabled overlay\n");
        fisics_free_analysis_result(&res);
        return 1;
    }

    const FisicsDiagnostic* d = &res.diagnostics[0];
    if (d->category_id != FISICS_DIAG_CATEGORY_EXTENSION) {
        fprintf(stderr, "expected extension category, got %d\n", d->category_id);
        fisics_free_analysis_result(&res);
        return 1;
    }
    if (d->code != FISICS_DIAG_CODE_EXTENSION_UNITS_DISABLED ||
        d->code_id != FISICS_DIAG_CODE_EXTENSION_UNITS_DISABLED) {
        fprintf(stderr, "expected disabled-overlay code 4001, got code=%d code_id=%d\n",
                d->code,
                d->code_id);
        fisics_free_analysis_result(&res);
        return 1;
    }

    fisics_free_analysis_result(&res);
    return 0;
}
