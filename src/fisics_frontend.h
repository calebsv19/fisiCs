#pragma once

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "Compiler/diagnostics.h"
#include "Compiler/compiler_context.h"

typedef struct {
    FisicsDiagnostic* diagnostics;
    size_t diag_count;

    FisicsTokenSpan* tokens;
    size_t token_count;

    FisicsSymbol* symbols;
    size_t symbol_count;
} FisicsAnalysisResult;

bool fisics_analyze_buffer(const char* file_path,
                           const char* source,
                           size_t length,
                           FisicsAnalysisResult* out);

void fisics_free_analysis_result(FisicsAnalysisResult* result);

#ifdef __cplusplus
}
#endif
