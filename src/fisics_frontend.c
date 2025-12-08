#include "fisics_frontend.h"

#include <stdlib.h>
#include <string.h>

#include "Compiler/compiler_context.h"
#include "Compiler/diagnostics.h"
#include "Compiler/pipeline.h"

static bool copy_diagnostics(const CompilerContext* ctx, FisicsAnalysisResult* out) {
    size_t count = 0;
    const FisicsDiagnostic* src = compiler_diagnostics_data(ctx, &count);
    if (count == 0) {
        out->diagnostics = NULL;
        out->diag_count = 0;
        return true;
    }
    FisicsDiagnostic* dst = (FisicsDiagnostic*)calloc(count, sizeof(FisicsDiagnostic));
    if (!dst) return false;
    for (size_t i = 0; i < count; ++i) {
        dst[i] = src[i];
        if (src[i].message) {
            dst[i].message = strdup(src[i].message);
            if (!dst[i].message) {
                out->diag_count = i;
                fisics_free_analysis_result(out);
                return false;
            }
        }
        if (src[i].hint) {
            dst[i].hint = strdup(src[i].hint);
            if (!dst[i].hint) {
                out->diag_count = i + 1;
                fisics_free_analysis_result(out);
                return false;
            }
        }
    }
    out->diagnostics = dst;
    out->diag_count = count;
    return true;
}

static bool copy_tokens(const CompilerContext* ctx, FisicsAnalysisResult* out) {
    size_t count = 0;
    const FisicsTokenSpan* src = cc_get_token_spans(ctx, &count);
    if (count == 0 || !src) {
        out->tokens = NULL;
        out->token_count = 0;
        return true;
    }
    FisicsTokenSpan* dst = (FisicsTokenSpan*)malloc(count * sizeof(FisicsTokenSpan));
    if (!dst) return false;
    memcpy(dst, src, count * sizeof(FisicsTokenSpan));
    out->tokens = dst;
    out->token_count = count;
    return true;
}

static char* dupstr(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* copy = (char*)malloc(len);
    if (copy) memcpy(copy, s, len);
    return copy;
}

static bool copy_symbols(const CompilerContext* ctx, FisicsAnalysisResult* out) {
    size_t count = 0;
    const FisicsSymbol* src = cc_get_symbols(ctx, &count);
    if (count == 0 || !src) {
        out->symbols = NULL;
        out->symbol_count = 0;
        return true;
    }
    FisicsSymbol* dst = (FisicsSymbol*)calloc(count, sizeof(FisicsSymbol));
    if (!dst) return false;
    for (size_t i = 0; i < count; ++i) {
        dst[i] = src[i];
        if (src[i].name) {
            dst[i].name = dupstr(src[i].name);
            if (!dst[i].name) { out->symbol_count = i; fisics_free_analysis_result(out); return false; }
        }
        if (src[i].file_path) {
            dst[i].file_path = dupstr(src[i].file_path);
            if (!dst[i].file_path) { out->symbol_count = i + 1; fisics_free_analysis_result(out); return false; }
        }
    }
    out->symbols = dst;
    out->symbol_count = count;
    return true;
}

bool fisics_analyze_buffer(const char* file_path,
                           const char* source,
                           size_t length,
                           FisicsAnalysisResult* out) {
    if (out) {
        memset(out, 0, sizeof(*out));
    }
    if (!file_path || !source) return false;
    CompilerContext* ctx = cc_create();
    if (!ctx) return false;

    struct ASTNode* ast = NULL;
    struct SemanticModel* model = NULL;
    size_t semaErrors = 0;

    bool ok = compiler_run_frontend(ctx,
                                    file_path,
                                    source,
                                    length,
                                    false,
                                    NULL,
                                    0,
                                    false,
                                    false,
                                    &ast,
                                    &model,
                                    &semaErrors);
    (void)ast;
    (void)model;
    if (!ok) {
        cc_destroy(ctx);
        return false;
    }

    bool copied = copy_diagnostics(ctx, out) &&
                  copy_tokens(ctx, out) &&
                  copy_symbols(ctx, out);

    cc_destroy(ctx);
    return copied;
}

void fisics_free_analysis_result(FisicsAnalysisResult* result) {
    if (!result) return;
    if (result->diagnostics) {
        for (size_t i = 0; i < result->diag_count; ++i) {
            free(result->diagnostics[i].message);
            free(result->diagnostics[i].hint);
        }
        free(result->diagnostics);
    }
    if (result->tokens) {
        free(result->tokens);
    }
    if (result->symbols) {
        for (size_t i = 0; i < result->symbol_count; ++i) {
            free((char*)result->symbols[i].name);
            free((char*)result->symbols[i].file_path);
        }
        free(result->symbols);
    }
    result->diagnostics = NULL;
    result->diag_count = 0;
    result->tokens = NULL;
    result->token_count = 0;
    result->symbols = NULL;
    result->symbol_count = 0;
}
