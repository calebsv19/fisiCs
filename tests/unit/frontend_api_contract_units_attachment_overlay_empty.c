#include <stdio.h>
#include <string.h>

#include "fisics_frontend.h"

int main(void) {
    const char* src = "int main(void){ return 0; }\n";
    FisicsFrontendOptions opts = {0};
    opts.overlay_features = FISICS_OVERLAY_PHYSICS_UNITS;

    FisicsAnalysisResult res = (FisicsAnalysisResult){0};
    if (!fisics_analyze_buffer("contract_units_overlay_empty.c", src, strlen(src), &opts, &res)) {
        fprintf(stderr, "fisics_analyze_buffer failed\n");
        return 1;
    }

    if ((res.contract.capabilities & FISICS_CONTRACT_CAP_EXTENSION_UNITS_ATTACHMENTS) != 0) {
        fprintf(stderr, "units attachment capability unexpectedly advertised without any units annotations\n");
        fisics_free_analysis_result(&res);
        return 1;
    }
    if ((res.contract.capabilities & FISICS_CONTRACT_CAP_EXTENSION_UNITS_CONCRETE) != 0) {
        fprintf(stderr, "concrete-units capability unexpectedly advertised without any units annotations\n");
        fisics_free_analysis_result(&res);
        return 1;
    }
    if (res.units_attachments != NULL || res.units_attachment_count != 0) {
        fprintf(stderr, "expected empty units lane with overlay enabled but no annotations, got ptr=%p count=%zu\n",
                (void*)res.units_attachments,
                res.units_attachment_count);
        fisics_free_analysis_result(&res);
        return 1;
    }

    fisics_free_analysis_result(&res);
    return 0;
}
