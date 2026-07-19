#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Compiler/diagnostic_metadata.h"

static uint64_t hash_bytes(uint64_t hash, const char* text) {
    const unsigned char* p = (const unsigned char*)(text ? text : "");
    while (*p) {
        hash ^= (uint64_t)*p++;
        hash *= 1099511628211ULL;
    }
    return hash;
}

static uint64_t hash_int(uint64_t hash, int value) {
    char text[32];
    snprintf(text, sizeof(text), "%d", value);
    return hash_bytes(hash, text);
}

static int require_environment(void) {
    const char* scenario = getenv("FISICS_STAGE_G_SCENARIO");
    const char* seed = getenv("FISICS_STAGE_G_SEED");
    return scenario && seed &&
           strcmp(scenario, "fisics-self") == 0 &&
           strcmp(seed, "424242") == 0;
}

int main(void) {
    if (!require_environment()) return 2;
    printf("TRACE|1|boot|scenario=fisics-self|seed=424242|status=ready\n");

    size_t count = 0;
    const FisicsDiagnosticExplanation* entries = fisics_diag_explanations(&count);
    if (!entries || count < 9u) return 3;
    uint64_t digest = 1469598103934665603ULL;
    for (size_t i = 0; i < count; ++i) {
        int code = entries[i].code_id;
        int category = fisics_diag_category_id_from_code(code);
        digest = hash_int(digest, code);
        digest = hash_bytes(digest, fisics_diag_code_name(code));
        digest = hash_int(digest, category);
        digest = hash_bytes(digest, fisics_diag_category_name(category));
        digest = hash_bytes(digest, fisics_diag_stage_name_from_code(code));
        digest = hash_bytes(digest, entries[i].description);
        digest = hash_bytes(digest, entries[i].common_causes);
        digest = hash_bytes(digest, entries[i].next_action);
    }
    printf("TRACE|1|taxonomy|count=%llu|digest=%llu|unknown=%s\n",
           (unsigned long long)count,
           (unsigned long long)digest,
           fisics_diag_code_name(999999));

    const FisicsDiagnosticExplanation* by_code =
        fisics_diag_explanation_by_code(FISICS_DIAG_CODE_PARSER_EXPECT_SEMICOLON);
    const FisicsDiagnosticExplanation* by_name =
        fisics_diag_explanation_by_name("extension.units.assign_dim_mismatch");
    const FisicsDiagnosticExplanation* by_query =
        fisics_diag_explanation_by_query("7101");
    const FisicsDiagnosticExplanation* invalid =
        fisics_diag_explanation_by_query("7101x");
    if (!by_code || !by_name || !by_query || invalid) return 4;
    printf("TRACE|1|lookup|parser=%d|extension=%d|link=%d\n",
           by_code->code_id, by_name->code_id, by_query->code_id);

    printf("TRACE|1|shutdown|lookups=3|invalid=1|status=clean\n");
    return 0;
}
