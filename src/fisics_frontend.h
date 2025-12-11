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

    FisicsInclude* includes;
    size_t include_count;
} FisicsAnalysisResult;

typedef struct {
    // Array of include search paths (equivalent to -I). Optional; pass NULL/0 to use defaults.
    const char* const* include_paths;
    size_t include_path_count;
} FisicsFrontendOptions;

bool fisics_analyze_buffer(const char* file_path,
                           const char* source,
                           size_t length,
                           const FisicsFrontendOptions* opts,
                           FisicsAnalysisResult* out);

void fisics_free_analysis_result(FisicsAnalysisResult* result);

#ifdef __cplusplus
}
#endif
