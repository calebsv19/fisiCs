#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "fisics_frontend.h"

static uint64_t mix_u64(uint64_t hash, uint64_t value) {
    for (int i = 0; i < 8; ++i) {
        hash ^= (value >> (i * 8)) & 0xffULL;
        hash *= 1099511628211ULL;
    }
    return hash;
}

static uint64_t digest_result(const FisicsAnalysisResult* result) {
    uint64_t hash = 1469598103934665603ULL;
    hash = mix_u64(hash, result->contract.source_hash);
    hash = mix_u64(hash, result->contract.source_length);
    hash = mix_u64(hash, result->contract.capabilities);
    hash = mix_u64(hash, result->contract.partial ? 1u : 0u);
    hash = mix_u64(hash, result->contract.fatal ? 1u : 0u);
    hash = mix_u64(hash, result->diag_count);
    hash = mix_u64(hash, result->token_count);
    hash = mix_u64(hash, result->symbol_count);
    hash = mix_u64(hash, result->include_count);
    hash = mix_u64(hash, result->units_attachment_count);
    for (size_t i = 0; i < result->diag_count; ++i) {
        hash = mix_u64(hash, (uint64_t)result->diagnostics[i].code_id);
        hash = mix_u64(hash, (uint64_t)result->diagnostics[i].category_id);
        hash = mix_u64(hash, (uint64_t)result->diagnostics[i].line);
        hash = mix_u64(hash, (uint64_t)result->diagnostics[i].column);
    }
    for (size_t i = 0; i < result->symbol_count; ++i) {
        hash = mix_u64(hash, result->symbols[i].stable_id);
        hash = mix_u64(hash, result->symbols[i].parent_stable_id);
        hash = mix_u64(hash, (uint64_t)result->symbols[i].kind);
    }
    return hash;
}

static int analyze(const char* path, const char* source,
                   const FisicsFrontendOptions* options,
                   FisicsAnalysisResult* result) {
    memset(result, 0, sizeof(*result));
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

static int write_artifact(uint64_t valid, uint64_t invalid,
                          uint64_t units, uint64_t reload) {
    if (mkdir("artifacts", 0700) != 0 && errno != EEXIST) return 0;
    FILE* fp = fopen("artifacts/workflow_summary.txt", "wb");
    if (!fp) return 0;
    int ok = fprintf(fp,
                     "scenario=fisics-self\nseed=424242\n"
                     "valid=%llu\ninvalid=%llu\nunits=%llu\nreload=%llu\n",
                     (unsigned long long)valid,
                     (unsigned long long)invalid,
                     (unsigned long long)units,
                     (unsigned long long)reload) > 0;
    return fclose(fp) == 0 && ok;
}

int main(void) {
    const char* scenario = getenv("FISICS_STAGE_G_SCENARIO");
    const char* seed = getenv("FISICS_STAGE_G_SEED");
    if (!scenario || !seed || strcmp(scenario, "fisics-self") != 0 ||
        strcmp(seed, "424242") != 0) return 2;
    printf("TRACE|1|bootstrap|scenario=fisics-self|seed=424242|status=ready\n");

    const char* valid_source =
        "typedef struct StagePoint { int x; int y; } StagePoint;\n"
        "static int add(int a, int b) { return a + b; }\n"
        "int workflow(StagePoint p) { return add(p.x, p.y); }\n";
    const char* invalid_source =
        "int broken(void) { return stage_g_missing_symbol + 1; }\n";
    const char* units_source =
        "double distance [[fisics::dim(length)]] = 1.0;\n"
        "double time_value [[fisics::dim(time)]] = 2.0;\n";
    printf("TRACE|1|project|inputs=3|overlays=1|artifact_count=1\n");

    FisicsAnalysisResult valid = {0};
    if (!analyze("/fisics-stage-g/workflow_valid.c", valid_source, NULL, &valid)) return 3;
    uint64_t valid_digest = digest_result(&valid);
    printf("TRACE|1|analyzed_valid|digest=%llu|symbols=%llu|diagnostics=%llu\n",
           (unsigned long long)valid_digest,
           (unsigned long long)valid.symbol_count,
           (unsigned long long)valid.diag_count);
    fisics_free_analysis_result(&valid);

    FisicsAnalysisResult invalid = {0};
    if (!analyze("/fisics-stage-g/workflow_invalid.c", invalid_source, NULL, &invalid)) return 4;
    if (invalid.diag_count == 0) {
        fisics_free_analysis_result(&invalid);
        return 5;
    }
    uint64_t invalid_digest = digest_result(&invalid);
    printf("TRACE|1|analyzed_invalid|digest=%llu|diagnostics=%llu|partial=%d\n",
           (unsigned long long)invalid_digest,
           (unsigned long long)invalid.diag_count,
           invalid.contract.partial ? 1 : 0);
    fisics_free_analysis_result(&invalid);

    FisicsFrontendOptions units_options = {0};
    units_options.overlay_features = FISICS_OVERLAY_PHYSICS_UNITS;
    FisicsAnalysisResult units = {0};
    if (!analyze("/fisics-stage-g/workflow_units.c", units_source, &units_options, &units)) return 6;
    if (units.units_attachment_count != 2) {
        fisics_free_analysis_result(&units);
        return 7;
    }
    uint64_t units_digest = digest_result(&units);
    printf("TRACE|1|analyzed_units|digest=%llu|attachments=%llu|capabilities=%llu\n",
           (unsigned long long)units_digest,
           (unsigned long long)units.units_attachment_count,
           (unsigned long long)units.contract.capabilities);
    fisics_free_analysis_result(&units);

    FisicsAnalysisResult reload = {0};
    if (!analyze("/fisics-stage-g/workflow_valid.c", valid_source, NULL, &reload)) return 8;
    uint64_t reload_digest = digest_result(&reload);
    fisics_free_analysis_result(&reload);
    if (reload_digest != valid_digest) return 9;
    printf("TRACE|1|reloaded|digest=%llu|matches_initial=1|status=clean\n",
           (unsigned long long)reload_digest);

    if (!write_artifact(valid_digest, invalid_digest, units_digest, reload_digest)) return 10;
    printf("TRACE|1|exported|artifact=workflow_summary.txt|digests=4|status=written\n");
    printf("TRACE|1|shutdown|analyses=4|destroyed=4|status=clean\n");
    return 0;
}
