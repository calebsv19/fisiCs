#include <stdio.h>
#include <string.h>

#include "fisics_frontend.h"

int main(void) {
    const char* src = "int main(void){ return 0; }\n";
    FisicsAnalysisResult res = (FisicsAnalysisResult){0};

    if (!fisics_analyze_buffer("contract_capabilities.c", src, strlen(src), NULL, &res)) {
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

    const uint64_t required =
        FISICS_CONTRACT_CAP_DIAGNOSTICS |
        FISICS_CONTRACT_CAP_INCLUDES |
        FISICS_CONTRACT_CAP_SYMBOLS |
        FISICS_CONTRACT_CAP_TOKENS |
        FISICS_CONTRACT_CAP_SYMBOL_PARENT_STABLE_ID |
        FISICS_CONTRACT_CAP_DIAGNOSTIC_TAXONOMY;

    if ((res.contract.capabilities & required) != required) {
        fprintf(stderr, "missing required capabilities mask=0x%llx required=0x%llx\n",
                (unsigned long long)res.contract.capabilities,
                (unsigned long long)required);
        fisics_free_analysis_result(&res);
        return 1;
    }

    fisics_free_analysis_result(&res);
    return 0;
}
