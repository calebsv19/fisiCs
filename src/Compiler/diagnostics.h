#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

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

// Placeholder diagnostic codes; expand/partition per subsystem as we migrate callers.
typedef enum {
    CDIAG_GENERIC              = 1,
    CDIAG_PARSER_GENERIC       = 1000,
    CDIAG_PARSER_EXPECT_SEMICOLON = 1001,
    CDIAG_SEMANTIC_GENERIC     = 2000,
    CDIAG_PREPROCESSOR_GENERIC = 3000
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

#ifdef __cplusplus
}
#endif
