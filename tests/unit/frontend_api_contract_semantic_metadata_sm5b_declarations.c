#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "fisics_frontend.h"

_Static_assert(FISICS_ANALYSIS_RESULT_EX_ABI_VERSION == 1,
               "unexpected extended-result ABI version");
_Static_assert(FISICS_SEMANTIC_METADATA_SCHEMA_VERSION == 1,
               "unexpected semantic-metadata schema version");
_Static_assert(FISICS_CONTRACT_CAP_EXTENSION_SEMANTIC_METADATA == (1ULL << 8),
               "unexpected semantic-metadata capability bit");
_Static_assert(FISICS_SEMANTIC_METADATA_SYMBOL_FUNCTION == 1 &&
                   FISICS_SEMANTIC_METADATA_SYMBOL_FILE_VARIABLE == 2,
               "unexpected public metadata symbol enum values");
_Static_assert(FISICS_SEMANTIC_METADATA_EDGE_PRODUCES == 1 &&
                   FISICS_SEMANTIC_METADATA_EDGE_CONSUMES == 2,
               "unexpected public metadata edge enum values");
_Static_assert(FISICS_SEMANTIC_METADATA_REFERENCE_EXTERNAL == 1 &&
                   FISICS_SEMANTIC_METADATA_REFERENCE_RESOLVED == 2,
               "unexpected public metadata reference enum values");
_Static_assert(offsetof(FisicsAnalysisResultEx, analysis) >=
                   sizeof(uint32_t) + sizeof(size_t),
               "extended result header does not precede the base result");
_Static_assert(offsetof(FisicsAnalysisResultEx, semantic_metadata_records) >=
                   offsetof(FisicsAnalysisResultEx, analysis) +
                       sizeof(FisicsAnalysisResult),
               "metadata records overlap the legacy base result");

int main(void) {
    if (strcmp(FISICS_SEMANTIC_METADATA_SCHEMA_ID,
               "fisiCs.semantic_metadata") != 0) {
        return 1;
    }

    FisicsAnalysisResultEx result = {0};
    result.abi_version = FISICS_ANALYSIS_RESULT_EX_ABI_VERSION;
    result.struct_size = sizeof(result);
    if (result.semantic_metadata_records != NULL ||
        result.semantic_metadata_record_count != 0 ||
        result.semantic_metadata_edges != NULL ||
        result.semantic_metadata_edge_count != 0) {
        return 1;
    }

    /* Unevaluated references prove the frozen declarations and signatures
     * without requiring SM-5D implementations during declarations-only SM-5B. */
    (void)sizeof(&fisics_analyze_buffer_ex);
    (void)sizeof(&fisics_free_analysis_result_ex);
    return 0;
}
