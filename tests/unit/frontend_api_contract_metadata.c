#include <stdio.h>
#include <string.h>

#include "fisics_frontend.h"

int main(void) {
    const char* src = "int main(void){ return 0; }\n";
    FisicsAnalysisResult res = (FisicsAnalysisResult){0};

    if (!fisics_analyze_buffer("contract_metadata.c", src, strlen(src), NULL, &res)) {
        fprintf(stderr, "fisics_analyze_buffer failed\n");
        return 1;
    }

    if (strcmp(res.contract.contract_id, "fisiCs.analysis.contract") != 0) {
        fprintf(stderr, "unexpected contract_id: %s\n", res.contract.contract_id);
        fisics_free_analysis_result(&res);
        return 1;
    }

    if (res.contract.contract_major != 1) {
        fprintf(stderr, "unsupported contract_major: %u\n", (unsigned)res.contract.contract_major);
        fisics_free_analysis_result(&res);
        return 1;
    }
    if (res.contract.contract_minor < 7) {
        fprintf(stderr, "contract_minor below expected floor: %u\n", (unsigned)res.contract.contract_minor);
        fisics_free_analysis_result(&res);
        return 1;
    }

    if (res.contract.producer_name[0] == '\0' || res.contract.producer_version[0] == '\0') {
        fprintf(stderr, "producer metadata missing\n");
        fisics_free_analysis_result(&res);
        return 1;
    }

    if (res.contract.source_length != (uint64_t)strlen(src)) {
        fprintf(stderr, "source_length mismatch: got=%llu expected=%llu\n",
                (unsigned long long)res.contract.source_length,
                (unsigned long long)strlen(src));
        fisics_free_analysis_result(&res);
        return 1;
    }

    if (res.contract.source_hash == 0ULL) {
        fprintf(stderr, "source_hash unexpectedly zero\n");
        fisics_free_analysis_result(&res);
        return 1;
    }

    fisics_free_analysis_result(&res);
    return 0;
}
