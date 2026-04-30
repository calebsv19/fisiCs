#include <stdio.h>
#include <string.h>

#include "fisics_frontend.h"

static const FisicsSymbol* find_symbol(const FisicsAnalysisResult* res, const char* name) {
    if (!res || !name) return NULL;
    for (size_t i = 0; i < res->symbol_count; ++i) {
        if (res->symbols[i].name && strcmp(res->symbols[i].name, name) == 0) {
            return &res->symbols[i];
        }
    }
    return NULL;
}

int main(void) {
    const char* src =
        "double distance [[fisics::dim(m)]] = 1.0;\n";
    FisicsFrontendOptions opts = {0};
    opts.overlay_features = FISICS_OVERLAY_PHYSICS_UNITS;

    FisicsAnalysisResult res = (FisicsAnalysisResult){0};
    if (!fisics_analyze_buffer("contract_units_export.c", src, strlen(src), &opts, &res)) {
        fprintf(stderr, "fisics_analyze_buffer failed\n");
        return 1;
    }

    if ((res.contract.capabilities & FISICS_CONTRACT_CAP_EXTENSION_UNITS_ATTACHMENTS) == 0) {
        fprintf(stderr, "units attachment capability not advertised for resolved overlay export\n");
        fisics_free_analysis_result(&res);
        return 1;
    }

    if (!res.units_attachments || res.units_attachment_count != 1) {
        fprintf(stderr, "expected one exported units attachment, got ptr=%p count=%zu\n",
                (void*)res.units_attachments,
                res.units_attachment_count);
        fisics_free_analysis_result(&res);
        return 1;
    }

    const FisicsSymbol* sym = find_symbol(&res, "distance");
    if (!sym || sym->stable_id == 0) {
        fprintf(stderr, "failed to find exported symbol stable id for distance\n");
        fisics_free_analysis_result(&res);
        return 1;
    }

    const FisicsUnitsAttachment* att = &res.units_attachments[0];
    if (!att->resolved) {
        fprintf(stderr, "units attachment unexpectedly unresolved\n");
        fisics_free_analysis_result(&res);
        return 1;
    }
    if (att->symbol_stable_id != sym->stable_id) {
        fprintf(stderr, "units attachment stable id mismatch got=%llu expected=%llu\n",
                (unsigned long long)att->symbol_stable_id,
                (unsigned long long)sym->stable_id);
        fisics_free_analysis_result(&res);
        return 1;
    }
    if (!att->symbol_name || strcmp(att->symbol_name, "distance") != 0) {
        fprintf(stderr, "unexpected attachment symbol name: %s\n",
                att->symbol_name ? att->symbol_name : "<null>");
        fisics_free_analysis_result(&res);
        return 1;
    }
    if (!att->dim_text || strcmp(att->dim_text, "m") != 0) {
        fprintf(stderr, "unexpected attachment dim text: %s\n",
                att->dim_text ? att->dim_text : "<null>");
        fisics_free_analysis_result(&res);
        return 1;
    }
    if (att->dim[0] != 1) {
        fprintf(stderr, "expected length exponent 1, got %d\n", (int)att->dim[0]);
        fisics_free_analysis_result(&res);
        return 1;
    }
    for (size_t i = 1; i < FISICS_UNITS_DIM_SLOTS; ++i) {
        if (att->dim[i] != 0) {
            fprintf(stderr, "expected zero exponent at slot %zu, got %d\n", i, (int)att->dim[i]);
            fisics_free_analysis_result(&res);
            return 1;
        }
    }

    fisics_free_analysis_result(&res);
    return 0;
}
