#include <stdio.h>
#include <string.h>

#include "fisics_frontend.h"

int main(void) {
    const char* src = "int main(void){ return 0; }\n";
    FisicsAnalysisResult res = (FisicsAnalysisResult){0};

    if (!fisics_analyze_buffer("contract_units_lane.c", src, strlen(src), NULL, &res)) {
        fprintf(stderr, "fisics_analyze_buffer failed\n");
        return 1;
    }

    if (res.contract.contract_major != 1 || res.contract.contract_minor < 7) {
        fprintf(stderr, "unexpected contract version %u.%u.%u\n",
                (unsigned)res.contract.contract_major,
                (unsigned)res.contract.contract_minor,
                (unsigned)res.contract.contract_patch);
        fisics_free_analysis_result(&res);
        return 1;
    }

    if ((res.contract.capabilities & FISICS_CONTRACT_CAP_EXTENSION_UNITS_ATTACHMENTS) != 0) {
        fprintf(stderr, "units attachment capability unexpectedly advertised before export implementation\n");
        fisics_free_analysis_result(&res);
        return 1;
    }
    if ((res.contract.capabilities & FISICS_CONTRACT_CAP_EXTENSION_UNITS_CONCRETE) != 0) {
        fprintf(stderr, "concrete-units capability unexpectedly advertised before export implementation\n");
        fisics_free_analysis_result(&res);
        return 1;
    }

    if (res.units_attachments != NULL || res.units_attachment_count != 0) {
        fprintf(stderr, "units attachment lane expected null/empty default, got ptr=%p count=%zu\n",
                (void*)res.units_attachments,
                res.units_attachment_count);
        fisics_free_analysis_result(&res);
        return 1;
    }

    fisics_free_analysis_result(&res);
    return 0;
}
