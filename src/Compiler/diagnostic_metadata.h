// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "Compiler/diagnostics.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Reserved Phase 2 diagnostic code ranges. These names are helper metadata
 * targets until a later slice wires writers to emit them.
 */
enum {
    FISICS_DIAG_CODE_LEXER_GENERIC = 5000,
    FISICS_DIAG_CODE_CODEGEN_GENERIC = 6000,
    FISICS_DIAG_CODE_BUILD_GENERIC = 7000,
    FISICS_DIAG_CODE_BUILD_DRIVER_GENERIC = 7001,
    FISICS_DIAG_CODE_LINK_GENERIC = 7100,
    FISICS_DIAG_CODE_LINK_STAGE_FAILED = 7101,
    FISICS_DIAG_CODE_BUILD_MANIFEST_GENERIC = 7200,
    FISICS_DIAG_CODE_BUILD_MANIFEST_LOAD_FAILED = 7201
};

int fisics_diag_severity_id_from_kind(DiagKind kind);
const char* fisics_diag_severity_name(int severity_id);

int fisics_diag_category_id_from_code(int code_id);
const char* fisics_diag_category_name(int category_id);

const char* fisics_diag_code_name(int code_id);
const char* fisics_diag_stage_name_from_code(int code_id);

typedef struct FisicsDiagnosticExplanation {
    int code_id;
    const char* description;
    const char* common_causes;
    const char* next_action;
} FisicsDiagnosticExplanation;

const FisicsDiagnosticExplanation* fisics_diag_explanations(size_t* countOut);
const FisicsDiagnosticExplanation* fisics_diag_explanation_by_code(int code_id);
const FisicsDiagnosticExplanation* fisics_diag_explanation_by_name(const char* code_name);
const FisicsDiagnosticExplanation* fisics_diag_explanation_by_query(const char* query);

#ifdef __cplusplus
}
#endif
