#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fisics_frontend.h"
#include "Extensions/extension_profile.h"

static int environment_ok(void) {
    const char* scenario = getenv("FISICS_STAGE_G_SCENARIO");
    const char* seed = getenv("FISICS_STAGE_G_SEED");
    return scenario && seed && strcmp(scenario, "fisics-self") == 0 &&
           strcmp(seed, "424242") == 0;
}

static int has_diag_code(const FisicsAnalysisResult* result, int code) {
    for (size_t i = 0; i < result->diag_count; ++i) {
        if (result->diagnostics[i].code_id == code) return 1;
    }
    return 0;
}

static int analyze_quiet(const char* path, const char* source,
                         const FisicsFrontendOptions* options,
                         FisicsAnalysisResult* result) {
    fflush(stdout);
    int saved_stdout = dup(STDOUT_FILENO);
    FILE* sink = fopen("/dev/null", "wb");
    if (saved_stdout < 0 || !sink || dup2(fileno(sink), STDOUT_FILENO) < 0) {
        if (sink) fclose(sink);
        if (saved_stdout >= 0) close(saved_stdout);
        return 0;
    }
    int ok = fisics_analyze_buffer(path, source, strlen(source), options, result) ? 1 : 0;
    fflush(stdout);
    int restored = dup2(saved_stdout, STDOUT_FILENO) >= 0;
    close(saved_stdout);
    fclose(sink);
    clearerr(stdout);
    return ok && restored;
}

int main(void) {
    if (!environment_ok()) return 2;
    printf("TRACE|1|boot|scenario=fisics-self|seed=424242|status=ready\n");

    const char* modes[] = {"off", "ide", "physics-units", "ide, physics_units", "all"};
    FisicsOverlayFeatures parsed = FISICS_OVERLAY_NONE;
    unsigned long long mode_digest = 0;
    for (size_t i = 0; i < sizeof(modes) / sizeof(modes[0]); ++i) {
        if (!fisics_parse_overlay_mode(modes[i], &parsed)) return 3;
        mode_digest = mode_digest * 131ULL + (unsigned long long)parsed;
    }
    if (fisics_parse_overlay_mode("ide,unknown", &parsed)) return 4;
    if (fisics_parse_overlay_mode(NULL, &parsed)) return 5;
    printf("TRACE|1|profiles|valid=5|invalid=2|digest=%llu\n", mode_digest);

    const char* macro_source =
        "#ifndef STAGE_G_VALUE\n#error missing stage value\n#endif\n"
        "int stage_g_value(void) { return STAGE_G_VALUE; }\n";
    const char* defines[] = {"STAGE_G_VALUE=17"};
    FisicsFrontendOptions options = {0};
    options.macro_defines = defines;
    options.macro_define_count = 1;
    options.lenient_mode = -1;
    FisicsAnalysisResult macro_result = {0};
    if (!analyze_quiet("stage_g_macro.c", macro_source,
                       &options, &macro_result)) return 6;
    if (macro_result.contract.fatal || macro_result.diag_count != 0) {
        fisics_free_analysis_result(&macro_result);
        return 7;
    }
    printf("TRACE|1|macro|hash=%llu|symbols=%llu|diagnostics=%llu\n",
           (unsigned long long)macro_result.contract.source_hash,
           (unsigned long long)macro_result.symbol_count,
           (unsigned long long)macro_result.diag_count);
    fisics_free_analysis_result(&macro_result);

    const char* units_source = "double distance [[fisics::dim(length)]] = 1.0;\n";
    FisicsAnalysisResult disabled = {0};
    FisicsFrontendOptions disabled_options = {0};
    if (!analyze_quiet("stage_g_units_disabled.c", units_source,
                       &disabled_options, &disabled)) return 8;
    int disabled_diag = has_diag_code(&disabled, FISICS_DIAG_CODE_EXTENSION_UNITS_DISABLED);
    size_t disabled_attachments = disabled.units_attachment_count;
    fisics_free_analysis_result(&disabled);
    if (!disabled_diag || disabled_attachments != 0) return 9;

    FisicsAnalysisResult enabled = {0};
    FisicsFrontendOptions enabled_options = {0};
    enabled_options.overlay_features = FISICS_OVERLAY_PHYSICS_UNITS;
    if (!analyze_quiet("stage_g_units_enabled.c", units_source,
                       &enabled_options, &enabled)) return 10;
    if (enabled.units_attachment_count != 1 ||
        !(enabled.contract.capabilities & FISICS_CONTRACT_CAP_EXTENSION_UNITS_ATTACHMENTS) ||
        has_diag_code(&enabled, FISICS_DIAG_CODE_EXTENSION_UNITS_DISABLED)) {
        fisics_free_analysis_result(&enabled);
        return 11;
    }
    printf("TRACE|1|overlay|disabled_diag=1|enabled_attachments=%llu|capabilities=%llu\n",
           (unsigned long long)enabled.units_attachment_count,
           (unsigned long long)enabled.contract.capabilities);
    fisics_free_analysis_result(&enabled);

    printf("TRACE|1|shutdown|analyses=3|profiles=7|status=clean\n");
    return 0;
}
