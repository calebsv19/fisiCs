// SPDX-License-Identifier: Apache-2.0

#include "syntax_errors.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static CompilerContext* g_ctx = NULL;

static SourceRange make_range_from_linecol(int line, int column) {
    SourceRange r;
    r.start.file = NULL;
    r.start.line = line;
    r.start.column = column;
    r.end = r.start;
    return r;
}

void initErrorList(CompilerContext* ctx) {
    g_ctx = ctx;
}

void addError(int line, int column, const char* message, const char* hint) {
    if (!g_ctx) return;
    SourceRange loc = make_range_from_linecol(line, column);
    compiler_report_diag(g_ctx, loc, DIAG_ERROR, CDIAG_SEMANTIC_GENERIC, hint, "%s", message ? message : "error");
}

void addWarning(int line, int column, const char* message, const char* hint) {
    if (!g_ctx) return;
    SourceRange loc = make_range_from_linecol(line, column);
    compiler_report_diag(g_ctx, loc, DIAG_WARNING, CDIAG_SEMANTIC_GENERIC, hint, "%s", message ? message : "warning");
}

void addErrorFromToken(const Token* tok, const char* message, const char* hint) {
    if (!g_ctx) return;
    SourceRange loc = tok ? tok->location : make_range_from_linecol(0, 0);
    if (tok && tok->macroCallSite.start.file) {
        loc = tok->macroCallSite;
    }
    compiler_report_diag(g_ctx, loc, DIAG_ERROR, CDIAG_SEMANTIC_GENERIC, hint, "%s", message ? message : "error");
}

void addErrorWithRanges(SourceRange spelling,
                        SourceRange macroCallSite,
                        SourceRange macroDefinition,
                        const char* message,
                        const char* hint) {
    if (!g_ctx) return;
    (void)macroDefinition;
    SourceRange loc = spelling;
    if (macroCallSite.start.file) {
        loc = macroCallSite;
    }
    compiler_report_diag(g_ctx, loc, DIAG_ERROR, CDIAG_SEMANTIC_GENERIC, hint, "%s", message ? message : "error");
}

static void print_one(const FisicsDiagnostic* d) {
    if (!d) return;
    const char* kind = (d->kind == DIAG_WARNING) ? "Warning" : "Error";
    printf("%s at (%d:%d): %s\n", kind, d->line, d->column, d->message ? d->message : "");
    if (d->hint) {
        printf("   Hint: %s\n", d->hint);
    }
    if (d->file_path) {
        printf("   Spelling: %s:%d:%d\n", d->file_path, d->line, d->column);
    }
}

void reportErrors(void) {
    if (!g_ctx) return;
    size_t count = 0;
    const FisicsDiagnostic* items = compiler_diagnostics_data(g_ctx, &count);
    for (size_t i = 0; i < count; ++i) {
        if (items[i].code >= CDIAG_PARSER_GENERIC && items[i].code < CDIAG_SEMANTIC_GENERIC) {
            continue; // keep parser diagnostics for IDE consumers but skip CLI printing to preserve expectations
        }
        print_one(&items[i]);
    }
}

size_t getErrorCount(void) {
    if (!g_ctx) return 0;
    size_t count = 0;
    const FisicsDiagnostic* items = compiler_diagnostics_data(g_ctx, &count);
    size_t errors = 0;
    for (size_t i = 0; i < count; ++i) {
        if (items[i].kind == DIAG_ERROR &&
            !(items[i].code >= CDIAG_PARSER_GENERIC && items[i].code < CDIAG_SEMANTIC_GENERIC)) {
            errors++;
        }
    }
    return errors;
}

void freeErrorList(void) {
    // Diagnostics live on the CompilerContext; nothing to do here.
}
#include "Lexer/tokens.h"
#include "Compiler/diagnostics.h"
