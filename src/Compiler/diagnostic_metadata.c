// SPDX-License-Identifier: Apache-2.0

#include "Compiler/diagnostic_metadata.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

static const FisicsDiagnosticExplanation kFisicsDiagnosticExplanations[] = {
    {
        FISICS_DIAG_CODE_GENERIC,
        "A general compiler diagnostic was reported without a more specific code.",
        "Older diagnostic paths, fallback error handling, or an unexpected compiler state.",
        "Use the primary message and location first; if the issue is unclear, rerun with --emit-diags-json for stable metadata."
    },
    {
        FISICS_DIAG_CODE_PARSER_GENERIC,
        "The parser rejected source syntax before semantic analysis completed.",
        "Malformed declarations or statements, unsupported syntax, or earlier punctuation that changed parser state.",
        "Inspect the reported token and nearby punctuation; fixing the first parser error often removes follow-on errors."
    },
    {
        FISICS_DIAG_CODE_PARSER_EXPECT_SEMICOLON,
        "The parser expected a semicolon to finish the current declaration or statement.",
        "A missing semicolon, an unterminated declaration, or a previous expression that consumed more tokens than intended.",
        "Add the missing semicolon or simplify the previous declaration until the parser can recover at the reported location."
    },
    {
        FISICS_DIAG_CODE_SEMANTIC_GENERIC,
        "Semantic analysis rejected a source construct after parsing succeeded.",
        "Type mismatches, invalid lvalues, undeclared names, duplicate declarations, or cross-translation-unit conflicts.",
        "Check the referenced declaration and expression types; use structured diagnostics when IDE/build consumers need exact ranges."
    },
    {
        FISICS_DIAG_CODE_PREPROCESSOR_GENERIC,
        "The preprocessor failed while expanding directives, includes, or macros.",
        "Missing include files, unbalanced conditionals, invalid macro calls, or unsupported preprocessor constructs.",
        "Check include search paths and macro arguments; JSON diagnostics may include include_stack or macro_trace context."
    },
    {
        FISICS_DIAG_CODE_LINK_STAGE_FAILED,
        "The driver completed compilation but the link command failed.",
        "Missing object files, unresolved symbols, incompatible libraries, or a linker invocation rejected by the host toolchain.",
        "Inspect the linker stderr and verify -L, -l, framework, and object inputs in the emitted driver/build graph data."
    },
    {
        FISICS_DIAG_CODE_EXTENSION_UNITS_ADD_DIM_MISMATCH,
        "A units extension addition used operands with incompatible dimensions.",
        "Adding values such as length plus time, or mixing a dimensioned value with a non-compatible unit expression.",
        "Convert operands to compatible units or restructure the expression so addition combines the same physical dimension."
    },
    {
        FISICS_DIAG_CODE_EXTENSION_UNITS_ASSIGN_DIM_MISMATCH,
        "A units extension assignment tried to store a value with an incompatible dimension.",
        "Assigning a value of one physical dimension to a variable declared with another dimension.",
        "Use an explicit supported conversion or change the destination declaration to match the assigned value's dimension."
    },
    {
        FISICS_DIAG_CODE_EXTENSION_UNITS_CONVERSION_INCOMPATIBLE,
        "A units extension conversion requested incompatible source and target dimensions.",
        "Converting between dimensions that do not represent the same physical quantity.",
        "Convert only between compatible units, and check that both source and target unit declarations use the intended dimensions."
    }
};

int fisics_diag_severity_id_from_kind(DiagKind kind) {
    switch (kind) {
        case DIAG_WARNING: return FISICS_DIAG_SEVERITY_WARNING;
        case DIAG_NOTE: return FISICS_DIAG_SEVERITY_INFO;
        case DIAG_ERROR:
        default: return FISICS_DIAG_SEVERITY_ERROR;
    }
}

const char* fisics_diag_severity_name(int severity_id) {
    switch (severity_id) {
        case FISICS_DIAG_SEVERITY_INFO: return "info";
        case FISICS_DIAG_SEVERITY_WARNING: return "warning";
        case FISICS_DIAG_SEVERITY_ERROR: return "error";
        default: return "unknown";
    }
}

int fisics_diag_category_id_from_code(int code_id) {
    if (code_id >= 7200 && code_id <= 7299) {
        return FISICS_DIAG_CATEGORY_BUILD;
    }
    if (code_id >= 7100 && code_id <= 7199) {
        return FISICS_DIAG_CATEGORY_BUILD;
    }
    if (code_id >= 7000 && code_id <= 7099) {
        return FISICS_DIAG_CATEGORY_BUILD;
    }
    if (code_id >= 6000 && code_id <= 6099) {
        return FISICS_DIAG_CATEGORY_CODEGEN;
    }
    if (code_id >= 5000 && code_id <= 5099) {
        return FISICS_DIAG_CATEGORY_LEXER;
    }
    if (code_id >= FISICS_DIAG_CODE_EXTENSION_GENERIC &&
        code_id <= FISICS_DIAG_CODE_EXTENSION_UNITS_CONVERSION_REQUIRES_FLOATING) {
        return FISICS_DIAG_CATEGORY_EXTENSION;
    }
    if (code_id >= FISICS_DIAG_CODE_PREPROCESSOR_GENERIC &&
        code_id < FISICS_DIAG_CODE_EXTENSION_GENERIC) {
        return FISICS_DIAG_CATEGORY_PREPROCESSOR;
    }
    if (code_id >= FISICS_DIAG_CODE_SEMANTIC_GENERIC &&
        code_id < FISICS_DIAG_CODE_PREPROCESSOR_GENERIC) {
        return FISICS_DIAG_CATEGORY_SEMANTIC;
    }
    if (code_id >= FISICS_DIAG_CODE_PARSER_GENERIC &&
        code_id < FISICS_DIAG_CODE_SEMANTIC_GENERIC) {
        return FISICS_DIAG_CATEGORY_PARSER;
    }
    if (code_id >= FISICS_DIAG_CODE_GENERIC &&
        code_id < FISICS_DIAG_CODE_PARSER_GENERIC) {
        return FISICS_DIAG_CATEGORY_ANALYSIS;
    }
    return FISICS_DIAG_CATEGORY_UNKNOWN;
}

const char* fisics_diag_category_name(int category_id) {
    switch (category_id) {
        case FISICS_DIAG_CATEGORY_ANALYSIS: return "analysis";
        case FISICS_DIAG_CATEGORY_PARSER: return "parser";
        case FISICS_DIAG_CATEGORY_SEMANTIC: return "semantic";
        case FISICS_DIAG_CATEGORY_PREPROCESSOR: return "preprocessor";
        case FISICS_DIAG_CATEGORY_LEXER: return "lexer";
        case FISICS_DIAG_CATEGORY_CODEGEN: return "codegen";
        case FISICS_DIAG_CATEGORY_BUILD: return "build";
        case FISICS_DIAG_CATEGORY_EXTENSION: return "extension";
        case FISICS_DIAG_CATEGORY_UNKNOWN:
        default: return "unknown";
    }
}

const char* fisics_diag_code_name(int code_id) {
    switch (code_id) {
        case FISICS_DIAG_CODE_UNKNOWN: return "unknown";
        case FISICS_DIAG_CODE_GENERIC: return "generic";
        case FISICS_DIAG_CODE_PARSER_GENERIC: return "parser.generic";
        case FISICS_DIAG_CODE_PARSER_EXPECT_SEMICOLON: return "parser.expect_semicolon";
        case FISICS_DIAG_CODE_SEMANTIC_GENERIC: return "semantic.generic";
        case FISICS_DIAG_CODE_PREPROCESSOR_GENERIC: return "preprocessor.generic";
        case FISICS_DIAG_CODE_EXTENSION_GENERIC: return "extension.generic";
        case FISICS_DIAG_CODE_EXTENSION_UNITS_DISABLED: return "extension.units.disabled";
        case FISICS_DIAG_CODE_EXTENSION_UNITS_INVALID_DIM: return "extension.units.invalid_dim";
        case FISICS_DIAG_CODE_EXTENSION_UNITS_DUPLICATE: return "extension.units.duplicate_dim";
        case FISICS_DIAG_CODE_EXTENSION_UNITS_INVALID_UNIT: return "extension.units.invalid_unit";
        case FISICS_DIAG_CODE_EXTENSION_UNITS_DUPLICATE_UNIT: return "extension.units.duplicate_unit";
        case FISICS_DIAG_CODE_EXTENSION_UNITS_UNIT_REQUIRES_DIM: return "extension.units.unit_requires_dim";
        case FISICS_DIAG_CODE_EXTENSION_UNITS_UNIT_DIM_MISMATCH: return "extension.units.unit_dim_mismatch";
        case FISICS_DIAG_CODE_EXTENSION_UNITS_ADD_DIM_MISMATCH: return "extension.units.add_dim_mismatch";
        case FISICS_DIAG_CODE_EXTENSION_UNITS_SUB_DIM_MISMATCH: return "extension.units.sub_dim_mismatch";
        case FISICS_DIAG_CODE_EXTENSION_UNITS_ASSIGN_DIM_MISMATCH: return "extension.units.assign_dim_mismatch";
        case FISICS_DIAG_CODE_EXTENSION_UNITS_COMPARE_DIM_MISMATCH: return "extension.units.compare_dim_mismatch";
        case FISICS_DIAG_CODE_EXTENSION_UNITS_EXPONENT_OVERFLOW: return "extension.units.exponent_overflow";
        case FISICS_DIAG_CODE_EXTENSION_UNITS_UNSUPPORTED_OPERATION: return "extension.units.unsupported_operation";
        case FISICS_DIAG_CODE_EXTENSION_UNITS_IMPLICIT_CONCRETE_CONVERSION: return "extension.units.implicit_concrete_conversion";
        case FISICS_DIAG_CODE_EXTENSION_UNITS_CONVERSION_INVALID_TARGET: return "extension.units.conversion_invalid_target";
        case FISICS_DIAG_CODE_EXTENSION_UNITS_CONVERSION_INCOMPATIBLE: return "extension.units.conversion_incompatible";
        case FISICS_DIAG_CODE_EXTENSION_UNITS_CONVERSION_REQUIRES_SOURCE_UNIT: return "extension.units.conversion_requires_source_unit";
        case FISICS_DIAG_CODE_EXTENSION_UNITS_CONVERSION_REQUIRES_FLOATING: return "extension.units.conversion_requires_floating";
        case FISICS_DIAG_CODE_LEXER_GENERIC: return "lexer.generic";
        case FISICS_DIAG_CODE_CODEGEN_GENERIC: return "codegen.generic";
        case FISICS_DIAG_CODE_BUILD_GENERIC: return "build.generic";
        case FISICS_DIAG_CODE_BUILD_DRIVER_GENERIC: return "build.driver_generic";
        case FISICS_DIAG_CODE_LINK_GENERIC: return "link.generic";
        case FISICS_DIAG_CODE_LINK_STAGE_FAILED: return "link.stage_failed";
        case FISICS_DIAG_CODE_BUILD_MANIFEST_GENERIC: return "build.manifest_generic";
        case FISICS_DIAG_CODE_BUILD_MANIFEST_LOAD_FAILED: return "build.manifest_load_failed";
        default: return "unknown";
    }
}

const char* fisics_diag_stage_name_from_code(int code_id) {
    if (code_id >= 7200 && code_id <= 7299) {
        return "build";
    }
    if (code_id >= 7100 && code_id <= 7199) {
        return "link";
    }
    if (code_id >= 7000 && code_id <= 7099) {
        return "build";
    }
    if (code_id >= 6000 && code_id <= 6099) {
        return "codegen";
    }
    if (code_id >= 5000 && code_id <= 5099) {
        return "lex";
    }
    if (code_id >= FISICS_DIAG_CODE_EXTENSION_GENERIC &&
        code_id <= FISICS_DIAG_CODE_EXTENSION_UNITS_CONVERSION_REQUIRES_FLOATING) {
        return "extension";
    }
    if (code_id >= FISICS_DIAG_CODE_PREPROCESSOR_GENERIC &&
        code_id < FISICS_DIAG_CODE_EXTENSION_GENERIC) {
        return "preprocess";
    }
    if (code_id >= FISICS_DIAG_CODE_SEMANTIC_GENERIC &&
        code_id < FISICS_DIAG_CODE_PREPROCESSOR_GENERIC) {
        return "semantic";
    }
    if (code_id >= FISICS_DIAG_CODE_PARSER_GENERIC &&
        code_id < FISICS_DIAG_CODE_SEMANTIC_GENERIC) {
        return "parse";
    }
    return "unknown";
}

const FisicsDiagnosticExplanation* fisics_diag_explanations(size_t* countOut) {
    if (countOut) {
        *countOut = sizeof(kFisicsDiagnosticExplanations) /
                    sizeof(kFisicsDiagnosticExplanations[0]);
    }
    return kFisicsDiagnosticExplanations;
}

const FisicsDiagnosticExplanation* fisics_diag_explanation_by_code(int code_id) {
    size_t count = 0;
    const FisicsDiagnosticExplanation* explanations = fisics_diag_explanations(&count);
    for (size_t i = 0; i < count; ++i) {
        if (explanations[i].code_id == code_id) {
            return &explanations[i];
        }
    }
    return NULL;
}

const FisicsDiagnosticExplanation* fisics_diag_explanation_by_name(const char* code_name) {
    if (!code_name || !code_name[0]) {
        return NULL;
    }
    size_t count = 0;
    const FisicsDiagnosticExplanation* explanations = fisics_diag_explanations(&count);
    for (size_t i = 0; i < count; ++i) {
        const char* known = fisics_diag_code_name(explanations[i].code_id);
        if (known && strcmp(known, code_name) == 0) {
            return &explanations[i];
        }
    }
    return NULL;
}

const FisicsDiagnosticExplanation* fisics_diag_explanation_by_query(const char* query) {
    const FisicsDiagnosticExplanation* byName = fisics_diag_explanation_by_name(query);
    if (byName) {
        return byName;
    }
    if (!query || !query[0]) {
        return NULL;
    }
    errno = 0;
    char* end = NULL;
    long value = strtol(query, &end, 10);
    if (errno != 0 || end == query || !end || *end != '\0' ||
        value < 0 || value > 2147483647L) {
        return NULL;
    }
    return fisics_diag_explanation_by_code((int)value);
}
