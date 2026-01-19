#include "fisics_frontend.h"

#include <stdlib.h>
#include <string.h>

#include "Compiler/compiler_context.h"
#include "Compiler/diagnostics.h"
#include "Compiler/pipeline.h"

// Copy diagnostics out of the compiler context. We purposefully override the
// file_path with the caller's file_path to avoid dangling pointers from the
// preprocessor/include resolver teardown.
static bool copy_diagnostics(const CompilerContext* ctx,
                             const char* callerFilePath,
                             FisicsAnalysisResult* out) {
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
        // Copy POD fields directly.
        dst[i].line   = src[i].line;
        dst[i].column = src[i].column;
        dst[i].length = src[i].length;
        dst[i].kind   = src[i].kind;
        dst[i].code   = src[i].code;

        const char* chosenPath = src[i].file_path ? src[i].file_path : callerFilePath;
        if (chosenPath) {
            dst[i].file_path = strdup(chosenPath);
            if (!dst[i].file_path) {
                out->diag_count = i;
                fisics_free_analysis_result(out);
                return false;
            }
        } else {
            dst[i].file_path = NULL;
        }

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
        if (src[i].parent_name) {
            dst[i].parent_name = dupstr(src[i].parent_name);
            if (!dst[i].parent_name) { out->symbol_count = i + 1; fisics_free_analysis_result(out); return false; }
        }
        if (src[i].return_type) {
            dst[i].return_type = dupstr(src[i].return_type);
            if (!dst[i].return_type) { out->symbol_count = i + 1; fisics_free_analysis_result(out); return false; }
        }
        dst[i].param_types = NULL;
        dst[i].param_count = src[i].param_count;
        if (src[i].param_types && src[i].param_count > 0) {
            dst[i].param_types = (const char**)calloc(src[i].param_count, sizeof(char*));
            if (!dst[i].param_types) { out->symbol_count = i + 1; fisics_free_analysis_result(out); return false; }
            for (size_t p = 0; p < src[i].param_count; ++p) {
                if (src[i].param_types[p]) {
                    ((char**)dst[i].param_types)[p] = dupstr(src[i].param_types[p]);
                    if (!dst[i].param_types[p]) { out->symbol_count = i + 1; fisics_free_analysis_result(out); return false; }
                }
            }
        }
        dst[i].param_names = NULL;
        if (src[i].param_names && src[i].param_count > 0) {
            dst[i].param_names = (const char**)calloc(src[i].param_count, sizeof(char*));
            if (!dst[i].param_names) { out->symbol_count = i + 1; fisics_free_analysis_result(out); return false; }
            for (size_t p = 0; p < src[i].param_count; ++p) {
                if (src[i].param_names[p]) {
                    ((char**)dst[i].param_names)[p] = dupstr(src[i].param_names[p]);
                    if (!dst[i].param_names[p]) { out->symbol_count = i + 1; fisics_free_analysis_result(out); return false; }
                }
            }
        }
    }
    out->symbols = dst;
    out->symbol_count = count;
    return true;
}

static bool copy_includes(const CompilerContext* ctx, FisicsAnalysisResult* out) {
    size_t count = 0;
    const FisicsInclude* src = cc_get_includes(ctx, &count);
    if (count == 0 || !src) {
        out->includes = NULL;
        out->include_count = 0;
        return true;
    }
    FisicsInclude* dst = (FisicsInclude*)calloc(count, sizeof(FisicsInclude));
    if (!dst) return false;
    for (size_t i = 0; i < count; ++i) {
        dst[i] = src[i];
        if (src[i].name) {
            dst[i].name = dupstr(src[i].name);
            if (!dst[i].name) { out->include_count = i; fisics_free_analysis_result(out); return false; }
        }
        if (src[i].resolved_path) {
            dst[i].resolved_path = dupstr(src[i].resolved_path);
            if (!dst[i].resolved_path) { out->include_count = i + 1; fisics_free_analysis_result(out); return false; }
        }
    }
    out->includes = dst;
    out->include_count = count;
    return true;
}

bool fisics_analyze_buffer(const char* file_path,
                           const char* source,
                           size_t length,
                           const FisicsFrontendOptions* opts,
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

    bool lenient = true;
    if (opts) {
        if (opts->lenient_mode < 0) lenient = false;
        else if (opts->lenient_mode > 0) lenient = true;
    }
    bool includeSystemSymbols = opts && opts->include_system_symbols;

    bool ok = compiler_run_frontend(ctx,
                                    file_path,
                                    source,
                                    length,
                                    false,
                                    false,
                                    opts ? opts->include_paths : NULL,
                                    opts ? opts->include_path_count : 0,
                                    opts ? opts->macro_defines : NULL,
                                    opts ? opts->macro_define_count : 0,
                                    lenient,
                                    includeSystemSymbols,
                                    false,
                                    false,
                                    &ast,
                                    &model,
                                    &semaErrors);
    (void)ast;
    (void)model;
    bool copied = false;
    if (ok) {
        copied = copy_diagnostics(ctx, file_path, out) &&
                 copy_tokens(ctx, out) &&
                 copy_symbols(ctx, out) &&
                 copy_includes(ctx, out);
    } else {
        // IDE lenient path: even if the pipeline failed (e.g., missing headers or parse
        // errors), return whatever diagnostics/includes we captured so the IDE can show them.
        copied = copy_diagnostics(ctx, file_path, out) &&
                 copy_tokens(ctx, out) &&
                 copy_symbols(ctx, out) &&
                 copy_includes(ctx, out);
    }

    cc_destroy(ctx);
    return copied && ok ? true : copied;
}

void fisics_free_analysis_result(FisicsAnalysisResult* result) {
    if (!result) return;
    if (result->diagnostics) {
        for (size_t i = 0; i < result->diag_count; ++i) {
            free((char*)result->diagnostics[i].file_path);
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
            free((char*)result->symbols[i].parent_name);
            free((char*)result->symbols[i].return_type);
            if (result->symbols[i].param_types) {
                for (size_t p = 0; p < result->symbols[i].param_count; ++p) {
                    free((char*)result->symbols[i].param_types[p]);
                }
                free(result->symbols[i].param_types);
            }
            if (result->symbols[i].param_names) {
                for (size_t p = 0; p < result->symbols[i].param_count; ++p) {
                    free((char*)result->symbols[i].param_names[p]);
                }
                free(result->symbols[i].param_names);
            }
        }
        free(result->symbols);
    }
    if (result->includes) {
        for (size_t i = 0; i < result->include_count; ++i) {
            free((char*)result->includes[i].name);
            free((char*)result->includes[i].resolved_path);
        }
        free(result->includes);
    }
    result->diagnostics = NULL;
    result->diag_count = 0;
    result->tokens = NULL;
    result->token_count = 0;
    result->symbols = NULL;
    result->symbol_count = 0;
    result->includes = NULL;
    result->include_count = 0;
}
