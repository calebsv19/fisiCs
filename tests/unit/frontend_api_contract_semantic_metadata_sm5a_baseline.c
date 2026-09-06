#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fisics_frontend.h"

/*
 * Frozen pre-SM-5 legacy layout. Keep this mirror unchanged when the extended
 * result API is added: its purpose is to catch accidental growth or reordering
 * of caller-allocated FisicsAnalysisResult.
 */
typedef struct {
    FisicsAnalysisContract contract;
    FisicsDiagnostic* diagnostics;
    size_t diag_count;
    FisicsTokenSpan* tokens;
    size_t token_count;
    FisicsSymbol* symbols;
    size_t symbol_count;
    FisicsInclude* includes;
    size_t include_count;
    FisicsUnitsAttachment* units_attachments;
    size_t units_attachment_count;
} FisicsAnalysisResultLegacySm5Baseline;

typedef bool (*FisicsLegacyAnalyzeSignature)(const char*,
                                             const char*,
                                             size_t,
                                             const FisicsFrontendOptions*,
                                             FisicsAnalysisResult*);
typedef void (*FisicsLegacyFreeSignature)(FisicsAnalysisResult*);

#define ASSERT_FIELD_OFFSET(field) \
    _Static_assert(offsetof(FisicsAnalysisResult, field) == \
                       offsetof(FisicsAnalysisResultLegacySm5Baseline, field), \
                   "legacy analysis result field moved: " #field)

_Static_assert(sizeof(FisicsAnalysisResult) ==
                   sizeof(FisicsAnalysisResultLegacySm5Baseline),
               "legacy analysis result size changed");
ASSERT_FIELD_OFFSET(contract);
ASSERT_FIELD_OFFSET(diagnostics);
ASSERT_FIELD_OFFSET(diag_count);
ASSERT_FIELD_OFFSET(tokens);
ASSERT_FIELD_OFFSET(token_count);
ASSERT_FIELD_OFFSET(symbols);
ASSERT_FIELD_OFFSET(symbol_count);
ASSERT_FIELD_OFFSET(includes);
ASSERT_FIELD_OFFSET(include_count);
ASSERT_FIELD_OFFSET(units_attachments);
ASSERT_FIELD_OFFSET(units_attachment_count);

_Static_assert(FISICS_CONTRACT_CAP_EXTENSION_UNITS_CONCRETE == (1ULL << 7),
               "pre-SM-5 capability boundary changed");

int main(void) {
    FisicsLegacyAnalyzeSignature analyze_fn = fisics_analyze_buffer;
    FisicsLegacyFreeSignature free_fn = fisics_free_analysis_result;
    if (!analyze_fn || !free_fn) {
        fprintf(stderr, "legacy frontend signatures are unavailable\n");
        return 1;
    }

    static const char source[] = "int sm5_legacy_plain(void) { return 5; }\n";
    FisicsAnalysisResult result = {0};
    if (!analyze_fn("sm5_legacy_plain.c", source, strlen(source), NULL, &result)) {
        fprintf(stderr, "legacy frontend baseline analysis failed\n");
        return 1;
    }

    const uint64_t required =
        FISICS_CONTRACT_CAP_DIAGNOSTICS |
        FISICS_CONTRACT_CAP_INCLUDES |
        FISICS_CONTRACT_CAP_SYMBOLS |
        FISICS_CONTRACT_CAP_TOKENS |
        FISICS_CONTRACT_CAP_SYMBOL_PARENT_STABLE_ID |
        FISICS_CONTRACT_CAP_DIAGNOSTIC_TAXONOMY;
    if (result.contract.contract_major != 1 ||
        result.contract.contract_minor < 8 ||
        (result.contract.capabilities & required) != required ||
        (result.contract.capabilities & ~required) != 0) {
        fprintf(stderr,
                "legacy frontend baseline drifted: version=%u.%u.%u caps=0x%llx\n",
                (unsigned)result.contract.contract_major,
                (unsigned)result.contract.contract_minor,
                (unsigned)result.contract.contract_patch,
                (unsigned long long)result.contract.capabilities);
        free_fn(&result);
        return 1;
    }

    free_fn(&result);
    return 0;
}
