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
        "double speed [[fisics::dim(speed)]] [[fisics::unit(feet_per_second)]] = 1.0;\n";
    FisicsFrontendOptions opts = {0};
    opts.overlay_features = FISICS_OVERLAY_PHYSICS_UNITS;

    FisicsAnalysisResult res = (FisicsAnalysisResult){0};
    if (!fisics_analyze_buffer("contract_units_concrete_export.c", src, strlen(src), &opts, &res)) {
        fprintf(stderr, "fisics_analyze_buffer failed\n");
        return 1;
    }

    if ((res.contract.capabilities & FISICS_CONTRACT_CAP_EXTENSION_UNITS_ATTACHMENTS) == 0) {
        fprintf(stderr, "units attachment capability not advertised for concrete-unit export\n");
        fisics_free_analysis_result(&res);
        return 1;
    }
    if ((res.contract.capabilities & FISICS_CONTRACT_CAP_EXTENSION_UNITS_CONCRETE) == 0) {
        fprintf(stderr, "concrete-units capability not advertised for concrete-unit export\n");
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

    const FisicsSymbol* sym = find_symbol(&res, "speed");
    if (!sym || sym->stable_id == 0) {
        fprintf(stderr, "failed to find exported symbol stable id for speed\n");
        fisics_free_analysis_result(&res);
        return 1;
    }

    const FisicsUnitsAttachment* att = &res.units_attachments[0];
    if (!att->resolved || !att->unit_resolved) {
        fprintf(stderr, "expected resolved dim+unit export\n");
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
    if (!att->dim_text || strcmp(att->dim_text, "m/s") != 0) {
        fprintf(stderr, "unexpected attachment dim text: %s\n",
                att->dim_text ? att->dim_text : "<null>");
        fisics_free_analysis_result(&res);
        return 1;
    }
    if (!att->unit_name || strcmp(att->unit_name, "foot_per_second") != 0) {
        fprintf(stderr, "unexpected unit name: %s\n",
                att->unit_name ? att->unit_name : "<null>");
        fisics_free_analysis_result(&res);
        return 1;
    }
    if (!att->unit_source_text || strcmp(att->unit_source_text, "feet_per_second") != 0) {
        fprintf(stderr, "unexpected unit source text: %s\n",
                att->unit_source_text ? att->unit_source_text : "<null>");
        fisics_free_analysis_result(&res);
        return 1;
    }
    if (!att->unit_symbol || strcmp(att->unit_symbol, "ft/s") != 0) {
        fprintf(stderr, "unexpected unit symbol: %s\n",
                att->unit_symbol ? att->unit_symbol : "<null>");
        fisics_free_analysis_result(&res);
        return 1;
    }
    if (!att->unit_family || strcmp(att->unit_family, "velocity") != 0) {
        fprintf(stderr, "unexpected unit family: %s\n",
                att->unit_family ? att->unit_family : "<null>");
        fisics_free_analysis_result(&res);
        return 1;
    }

    fisics_free_analysis_result(&res);
    return 0;
}
