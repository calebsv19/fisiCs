#include <stdio.h>
#include <string.h>

#include "Compiler/diagnostic_metadata.h"

static int expect_str(const char* label, const char* got, const char* want) {
    if (!got || strcmp(got, want) != 0) {
        fprintf(stderr, "%s: expected '%s', got '%s'\n", label, want, got ? got : "<null>");
        return 0;
    }
    return 1;
}

static int expect_int(const char* label, int got, int want) {
    if (got != want) {
        fprintf(stderr, "%s: expected %d, got %d\n", label, want, got);
        return 0;
    }
    return 1;
}

static int expect_code(int code,
                       int category,
                       const char* category_name,
                       const char* code_name,
                       const char* stage_name) {
    return expect_int("category id", fisics_diag_category_id_from_code(code), category) &&
           expect_str("category name",
                      fisics_diag_category_name(fisics_diag_category_id_from_code(code)),
                      category_name) &&
           expect_str("code name", fisics_diag_code_name(code), code_name) &&
           expect_str("stage name", fisics_diag_stage_name_from_code(code), stage_name);
}

int main(void) {
    if (!expect_int("error severity id",
                    fisics_diag_severity_id_from_kind(DIAG_ERROR),
                    FISICS_DIAG_SEVERITY_ERROR) ||
        !expect_str("error severity name",
                    fisics_diag_severity_name(FISICS_DIAG_SEVERITY_ERROR),
                    "error") ||
        !expect_str("warning severity name",
                    fisics_diag_severity_name(FISICS_DIAG_SEVERITY_WARNING),
                    "warning") ||
        !expect_str("note severity name",
                    fisics_diag_severity_name(FISICS_DIAG_SEVERITY_INFO),
                    "info")) {
        return 1;
    }

    if (!expect_code(FISICS_DIAG_CODE_PARSER_EXPECT_SEMICOLON,
                     FISICS_DIAG_CATEGORY_PARSER,
                     "parser",
                     "parser.expect_semicolon",
                     "parse") ||
        !expect_code(FISICS_DIAG_CODE_SEMANTIC_GENERIC,
                     FISICS_DIAG_CATEGORY_SEMANTIC,
                     "semantic",
                     "semantic.generic",
                     "semantic") ||
        !expect_code(FISICS_DIAG_CODE_PREPROCESSOR_GENERIC,
                     FISICS_DIAG_CATEGORY_PREPROCESSOR,
                     "preprocessor",
                     "preprocessor.generic",
                     "preprocess") ||
        !expect_code(FISICS_DIAG_CODE_LINK_STAGE_FAILED,
                     FISICS_DIAG_CATEGORY_BUILD,
                     "build",
                     "link.stage_failed",
                     "link") ||
        !expect_code(FISICS_DIAG_CODE_LEXER_GENERIC,
                     FISICS_DIAG_CATEGORY_LEXER,
                     "lexer",
                     "lexer.generic",
                     "lex") ||
        !expect_code(FISICS_DIAG_CODE_EXTENSION_UNITS_ASSIGN_DIM_MISMATCH,
                     FISICS_DIAG_CATEGORY_EXTENSION,
                     "extension",
                     "extension.units.assign_dim_mismatch",
                     "extension")) {
        return 1;
    }

    if (!expect_str("unknown code name", fisics_diag_code_name(999999), "unknown") ||
        !expect_str("unknown category name",
                    fisics_diag_category_name(FISICS_DIAG_CATEGORY_UNKNOWN),
                    "unknown") ||
        !expect_str("unknown stage", fisics_diag_stage_name_from_code(999999), "unknown")) {
        return 1;
    }

    return 0;
}
