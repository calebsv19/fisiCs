#include <stdio.h>
#include <string.h>

#include "fisics_frontend.h"

int main(void) {
    const char* src = "double distance [[fisics::dim(qqq)]] = 1.0;\n";
    FisicsFrontendOptions opts = (FisicsFrontendOptions){0};
    opts.overlay_features = FISICS_OVERLAY_PHYSICS_UNITS;

    FisicsAnalysisResult res = (FisicsAnalysisResult){0};
    if (!fisics_analyze_buffer("contract_units_invalid_gated.c", src, strlen(src), &opts, &res)) {
        fprintf(stderr, "fisics_analyze_buffer failed\n");
        return 1;
    }

    if (res.diag_count == 0) {
        fprintf(stderr, "expected invalid-dimension diagnostic\n");
        fisics_free_analysis_result(&res);
        return 1;
    }

    const FisicsDiagnostic* d = &res.diagnostics[0];
    if (d->category_id != FISICS_DIAG_CATEGORY_EXTENSION) {
        fprintf(stderr, "expected extension diagnostic category, got %d\n", d->category_id);
        fisics_free_analysis_result(&res);
        return 1;
    }
    if (d->code != FISICS_DIAG_CODE_EXTENSION_UNITS_INVALID_DIM ||
        d->code_id != FISICS_DIAG_CODE_EXTENSION_UNITS_INVALID_DIM) {
        fprintf(stderr, "expected invalid-dimension code 4002, got code=%d code_id=%d\n",
                d->code,
                d->code_id);
        fisics_free_analysis_result(&res);
        return 1;
    }
    if ((res.contract.capabilities & FISICS_CONTRACT_CAP_EXTENSION_UNITS_ATTACHMENTS) != 0) {
        fprintf(stderr, "units attachment capability unexpectedly advertised for invalid units annotation\n");
        fisics_free_analysis_result(&res);
        return 1;
    }
    if (res.units_attachments != NULL || res.units_attachment_count != 0) {
        fprintf(stderr, "expected invalid units annotation to export no units payload, got ptr=%p count=%zu\n",
                (void*)res.units_attachments,
                res.units_attachment_count);
        fisics_free_analysis_result(&res);
        return 1;
    }

    fisics_free_analysis_result(&res);
    return 0;
}
