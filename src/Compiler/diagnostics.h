// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

#include "core_data.h"
#include "Lexer/tokens.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DIAG_ERROR = 0,
    DIAG_WARNING = 1,
    DIAG_NOTE = 2
} DiagKind;

typedef struct {
    const char* file_path;  // may be NULL
    int line;               // 1-based
    int column;             // 1-based
    int length;             // characters to underline; >= 1
    DiagKind kind;
    int code;               // internal diagnostic code
    char* message;          // owned null-terminated message
    char* hint;             // optional hint string; owned if present
} FisicsDiagnostic;

// Stable v1 diagnostic code lane for external/compiler-IDE contract consumers.
// Codes may be extended in minor versions but existing values are stable within major v1.
typedef enum {
    FISICS_DIAG_CODE_UNKNOWN = 0,
    FISICS_DIAG_CODE_GENERIC = 1,
    FISICS_DIAG_CODE_PARSER_GENERIC = 1000,
    FISICS_DIAG_CODE_PARSER_EXPECT_SEMICOLON = 1001,
    FISICS_DIAG_CODE_SEMANTIC_GENERIC = 2000,
    FISICS_DIAG_CODE_PREPROCESSOR_GENERIC = 3000
} FisicsDiagCode;

// Backward-compatible aliases retained for existing internal callsites.
typedef enum {
    CDIAG_GENERIC = FISICS_DIAG_CODE_GENERIC,
    CDIAG_PARSER_GENERIC = FISICS_DIAG_CODE_PARSER_GENERIC,
    CDIAG_PARSER_EXPECT_SEMICOLON = FISICS_DIAG_CODE_PARSER_EXPECT_SEMICOLON,
    CDIAG_SEMANTIC_GENERIC = FISICS_DIAG_CODE_SEMANTIC_GENERIC,
    CDIAG_PREPROCESSOR_GENERIC = FISICS_DIAG_CODE_PREPROCESSOR_GENERIC
} CompilerDiagCode;

struct CompilerContext;

bool compiler_report_diag(struct CompilerContext* ctx,
                          SourceRange loc,
                          DiagKind kind,
                          int code,
                          const char* hint,
                          const char* fmt,
                          ...);

// Accessors for consumers/IDE bridge.
const FisicsDiagnostic* compiler_diagnostics_data(const struct CompilerContext* ctx, size_t* countOut);
bool compiler_diagnostics_copy(const struct CompilerContext* ctx, FisicsDiagnostic** outItems, size_t* outCount);
void compiler_diagnostics_clear(struct CompilerContext* ctx);
size_t compiler_diagnostics_error_count(const struct CompilerContext* ctx);
size_t compiler_diagnostics_parser_error_count(const struct CompilerContext* ctx);
CoreResult compiler_diagnostics_build_core_dataset(const struct CompilerContext* ctx,
                                                   CoreDataset* out_dataset);
CoreResult compiler_diagnostics_write_core_dataset_json(const struct CompilerContext* ctx,
                                                        const char* out_path);
CoreResult compiler_diagnostics_write_core_dataset_pack(const struct CompilerContext* ctx,
                                                        const char* out_path);

#ifdef __cplusplus
}
#endif
