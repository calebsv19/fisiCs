#include <stdio.h>
#include <string.h>

#include "fisics_frontend.h"

static const FisicsUnitsAttachment* find_attachment(const FisicsAnalysisResult* res,
                                                    const char* name) {
    if (!res || !name) return NULL;
    for (size_t i = 0; i < res->units_attachment_count; ++i) {
        if (res->units_attachments[i].symbol_name &&
            strcmp(res->units_attachments[i].symbol_name, name) == 0) {
            return &res->units_attachments[i];
        }
    }
    return NULL;
}

int main(void) {
    const char* src =
        "double sample(double dt [[fisics::dim(time)]] [[fisics::unit(second)]]) {\n"
        "    double speed [[fisics::dim(speed)]] [[fisics::unit(meter_per_second)]] = 3.0;\n"
        "    return speed * dt;\n"
        "}\n";
    FisicsFrontendOptions opts = {0};
    opts.overlay_features = FISICS_OVERLAY_PHYSICS_UNITS;

    FisicsAnalysisResult res = (FisicsAnalysisResult){0};
    if (!fisics_analyze_buffer("contract_units_local_export.c", src, strlen(src), &opts, &res)) {
        fprintf(stderr, "fisics_analyze_buffer failed\n");
        return 1;
    }

    if ((res.contract.capabilities & FISICS_CONTRACT_CAP_EXTENSION_UNITS_ATTACHMENTS) == 0) {
        fprintf(stderr, "units attachment capability not advertised for local export\n");
        fisics_free_analysis_result(&res);
        return 1;
    }
    if (!res.units_attachments || res.units_attachment_count != 2) {
        fprintf(stderr, "expected two local units attachments, got ptr=%p count=%zu\n",
                (void*)res.units_attachments,
                res.units_attachment_count);
        fisics_free_analysis_result(&res);
        return 1;
    }

    const FisicsUnitsAttachment* dt = find_attachment(&res, "dt");
    const FisicsUnitsAttachment* speed = find_attachment(&res, "speed");
    if (!dt || !speed) {
        fprintf(stderr, "missing local attachments: dt=%p speed=%p\n", (void*)dt, (void*)speed);
        fisics_free_analysis_result(&res);
        return 1;
    }
    if (dt->has_symbol_stable_id || dt->symbol_stable_id != 0 ||
        speed->has_symbol_stable_id || speed->symbol_stable_id != 0) {
        fprintf(stderr, "local attachments unexpectedly exported stable ids\n");
        fisics_free_analysis_result(&res);
        return 1;
    }
    if (dt->start_line != 1 || dt->start_col <= 0 ||
        speed->start_line != 2 || speed->start_col <= 0) {
        fprintf(stderr, "unexpected local ranges dt=%d:%d speed=%d:%d\n",
                dt->start_line, dt->start_col, speed->start_line, speed->start_col);
        fisics_free_analysis_result(&res);
        return 1;
    }
    if (!dt->source_file_path || strstr(dt->source_file_path, "contract_units_local_export.c") == NULL ||
        !speed->source_file_path || strstr(speed->source_file_path, "contract_units_local_export.c") == NULL) {
        fprintf(stderr, "missing local source paths\n");
        fisics_free_analysis_result(&res);
        return 1;
    }
    if (!dt->unit_name || strcmp(dt->unit_name, "second") != 0 ||
        !speed->unit_name || strcmp(speed->unit_name, "meter_per_second") != 0) {
        fprintf(stderr, "unexpected local unit names dt=%s speed=%s\n",
                dt->unit_name ? dt->unit_name : "<null>",
                speed->unit_name ? speed->unit_name : "<null>");
        fisics_free_analysis_result(&res);
        return 1;
    }

    fisics_free_analysis_result(&res);
    return 0;
}
