#include <stdio.h>
#include <string.h>

#include "fisics_frontend.h"

int main(void) {
    const char* src =
        "enum ContractStatus { CONTRACT_READY = 1, CONTRACT_PENDING = 2 };\n"
        "int read_status(enum ContractStatus s){ return (int)s; }\n";

    FisicsAnalysisResult res = (FisicsAnalysisResult){0};
    if (!fisics_analyze_buffer("contract_parent_stable.c", src, strlen(src), NULL, &res)) {
        fprintf(stderr, "fisics_analyze_buffer failed\n");
        return 1;
    }

    size_t owned_count = 0;
    size_t missing_parent_stable = 0;
    for (size_t i = 0; i < res.symbol_count; ++i) {
        const FisicsSymbol* sym = &res.symbols[i];
        if (!sym->parent_name || !sym->parent_name[0]) continue;
        if (sym->parent_kind == FISICS_SYMBOL_UNKNOWN) continue;
        owned_count++;
        if (sym->parent_stable_id == 0) {
            missing_parent_stable++;
        }
    }

    if (owned_count != 0 && missing_parent_stable != 0) {
        fprintf(stderr, "parent_stable_id missing for %zu/%zu owned symbols\n",
                missing_parent_stable, owned_count);
        fisics_free_analysis_result(&res);
        return 1;
    }

    fisics_free_analysis_result(&res);
    return 0;
}
