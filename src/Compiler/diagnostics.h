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

// Stable v1 diagnostic taxonomy lane for compiler-to-IDE contract consumers.
typedef enum {
    FISICS_DIAG_SEVERITY_INFO = 0,
    FISICS_DIAG_SEVERITY_WARNING = 1,
    FISICS_DIAG_SEVERITY_ERROR = 2
} FisicsDiagSeverityId;

typedef enum {
    FISICS_DIAG_CATEGORY_UNKNOWN = 0,
    FISICS_DIAG_CATEGORY_ANALYSIS = 1,
    FISICS_DIAG_CATEGORY_PARSER = 2,
    FISICS_DIAG_CATEGORY_SEMANTIC = 3,
    FISICS_DIAG_CATEGORY_PREPROCESSOR = 4,
    FISICS_DIAG_CATEGORY_LEXER = 5,
    FISICS_DIAG_CATEGORY_CODEGEN = 6,
    FISICS_DIAG_CATEGORY_BUILD = 7,
    FISICS_DIAG_CATEGORY_EXTENSION = 8
} FisicsDiagCategoryId;

typedef struct {
    const char* file_path;  // may be NULL
    int line;               // 1-based
    int column;             // 1-based
    int length;             // characters to underline; >= 1
    DiagKind kind;
    int code;               // internal diagnostic code
    int severity_id;        // stable taxonomy severity id (info/warning/error)
    int category_id;        // stable taxonomy category id
    int code_id;            // stable external-facing diagnostic code id
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
    FISICS_DIAG_CODE_PREPROCESSOR_GENERIC = 3000,
    /*
     * Extension namespace policy:
     * - 4000-4099: generic extension framework/bootstrap diagnostics
     * - 4100-4199: physics-units semantic diagnostics
     * Existing scaffold codes 4001-4003 are grandfathered bootstrap ids and
     * remain stable for v1 compatibility.
     */
    FISICS_DIAG_CODE_EXTENSION_GENERIC = 4000,
    FISICS_DIAG_CODE_EXTENSION_UNITS_DISABLED = 4001,
    FISICS_DIAG_CODE_EXTENSION_UNITS_INVALID_DIM = 4002,
    FISICS_DIAG_CODE_EXTENSION_UNITS_DUPLICATE = 4003,
    FISICS_DIAG_CODE_EXTENSION_UNITS_INVALID_UNIT = 4004,
    FISICS_DIAG_CODE_EXTENSION_UNITS_DUPLICATE_UNIT = 4005,
    FISICS_DIAG_CODE_EXTENSION_UNITS_UNIT_REQUIRES_DIM = 4006,
    FISICS_DIAG_CODE_EXTENSION_UNITS_UNIT_DIM_MISMATCH = 4007,

    FISICS_DIAG_CODE_EXTENSION_UNITS_SEMANTIC_BASE = 4100,
    FISICS_DIAG_CODE_EXTENSION_UNITS_ADD_DIM_MISMATCH = 4101,
    FISICS_DIAG_CODE_EXTENSION_UNITS_SUB_DIM_MISMATCH = 4102,
    FISICS_DIAG_CODE_EXTENSION_UNITS_ASSIGN_DIM_MISMATCH = 4103,
    FISICS_DIAG_CODE_EXTENSION_UNITS_COMPARE_DIM_MISMATCH = 4104,
    FISICS_DIAG_CODE_EXTENSION_UNITS_EXPONENT_OVERFLOW = 4105,
    FISICS_DIAG_CODE_EXTENSION_UNITS_UNSUPPORTED_OPERATION = 4106,
    FISICS_DIAG_CODE_EXTENSION_UNITS_IMPLICIT_CONCRETE_CONVERSION = 4107,
    FISICS_DIAG_CODE_EXTENSION_UNITS_CONVERSION_INVALID_TARGET = 4108,
    FISICS_DIAG_CODE_EXTENSION_UNITS_CONVERSION_INCOMPATIBLE = 4109,
    FISICS_DIAG_CODE_EXTENSION_UNITS_CONVERSION_REQUIRES_SOURCE_UNIT = 4110,
    FISICS_DIAG_CODE_EXTENSION_UNITS_CONVERSION_REQUIRES_FLOATING = 4111
} FisicsDiagCode;

// Backward-compatible aliases retained for existing internal callsites.
typedef enum {
    CDIAG_GENERIC = FISICS_DIAG_CODE_GENERIC,
    CDIAG_PARSER_GENERIC = FISICS_DIAG_CODE_PARSER_GENERIC,
    CDIAG_PARSER_EXPECT_SEMICOLON = FISICS_DIAG_CODE_PARSER_EXPECT_SEMICOLON,
    CDIAG_SEMANTIC_GENERIC = FISICS_DIAG_CODE_SEMANTIC_GENERIC,
    CDIAG_PREPROCESSOR_GENERIC = FISICS_DIAG_CODE_PREPROCESSOR_GENERIC,
    CDIAG_EXTENSION_GENERIC = FISICS_DIAG_CODE_EXTENSION_GENERIC,
    CDIAG_EXTENSION_UNITS_DISABLED = FISICS_DIAG_CODE_EXTENSION_UNITS_DISABLED,
    CDIAG_EXTENSION_UNITS_INVALID_DIM = FISICS_DIAG_CODE_EXTENSION_UNITS_INVALID_DIM,
    CDIAG_EXTENSION_UNITS_DUPLICATE = FISICS_DIAG_CODE_EXTENSION_UNITS_DUPLICATE,
    CDIAG_EXTENSION_UNITS_INVALID_UNIT = FISICS_DIAG_CODE_EXTENSION_UNITS_INVALID_UNIT,
    CDIAG_EXTENSION_UNITS_DUPLICATE_UNIT = FISICS_DIAG_CODE_EXTENSION_UNITS_DUPLICATE_UNIT,
    CDIAG_EXTENSION_UNITS_UNIT_REQUIRES_DIM = FISICS_DIAG_CODE_EXTENSION_UNITS_UNIT_REQUIRES_DIM,
    CDIAG_EXTENSION_UNITS_UNIT_DIM_MISMATCH = FISICS_DIAG_CODE_EXTENSION_UNITS_UNIT_DIM_MISMATCH,
    CDIAG_EXTENSION_UNITS_SEMANTIC_BASE = FISICS_DIAG_CODE_EXTENSION_UNITS_SEMANTIC_BASE,
    CDIAG_EXTENSION_UNITS_ADD_DIM_MISMATCH = FISICS_DIAG_CODE_EXTENSION_UNITS_ADD_DIM_MISMATCH,
    CDIAG_EXTENSION_UNITS_SUB_DIM_MISMATCH = FISICS_DIAG_CODE_EXTENSION_UNITS_SUB_DIM_MISMATCH,
    CDIAG_EXTENSION_UNITS_ASSIGN_DIM_MISMATCH = FISICS_DIAG_CODE_EXTENSION_UNITS_ASSIGN_DIM_MISMATCH,
    CDIAG_EXTENSION_UNITS_COMPARE_DIM_MISMATCH = FISICS_DIAG_CODE_EXTENSION_UNITS_COMPARE_DIM_MISMATCH,
    CDIAG_EXTENSION_UNITS_EXPONENT_OVERFLOW = FISICS_DIAG_CODE_EXTENSION_UNITS_EXPONENT_OVERFLOW,
    CDIAG_EXTENSION_UNITS_UNSUPPORTED_OPERATION = FISICS_DIAG_CODE_EXTENSION_UNITS_UNSUPPORTED_OPERATION,
    CDIAG_EXTENSION_UNITS_IMPLICIT_CONCRETE_CONVERSION = FISICS_DIAG_CODE_EXTENSION_UNITS_IMPLICIT_CONCRETE_CONVERSION,
    CDIAG_EXTENSION_UNITS_CONVERSION_INVALID_TARGET = FISICS_DIAG_CODE_EXTENSION_UNITS_CONVERSION_INVALID_TARGET,
    CDIAG_EXTENSION_UNITS_CONVERSION_INCOMPATIBLE = FISICS_DIAG_CODE_EXTENSION_UNITS_CONVERSION_INCOMPATIBLE,
    CDIAG_EXTENSION_UNITS_CONVERSION_REQUIRES_SOURCE_UNIT = FISICS_DIAG_CODE_EXTENSION_UNITS_CONVERSION_REQUIRES_SOURCE_UNIT,
    CDIAG_EXTENSION_UNITS_CONVERSION_REQUIRES_FLOATING = FISICS_DIAG_CODE_EXTENSION_UNITS_CONVERSION_REQUIRES_FLOATING
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
