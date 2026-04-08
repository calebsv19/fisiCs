// SPDX-License-Identifier: Apache-2.0

#include "fisics_frontend.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#ifdef __APPLE__
#include <malloc/malloc.h>
#endif

#include "Compiler/compiler_context.h"
#include "Compiler/diagnostics.h"
#include "Compiler/pipeline.h"

#define FISICS_CONTRACT_ID "fisiCs.analysis.contract"
#define FISICS_CONTRACT_MAJOR 1
#define FISICS_CONTRACT_MINOR 4
#define FISICS_CONTRACT_PATCH 0
#define FISICS_CONTRACT_PRODUCER "fisiCs"
#define FISICS_CONTRACT_PRODUCER_VERSION "0.1.0"

static bool str_equals(const char* a, const char* b) {
    if (a == b) return true;
    if (!a || !b) return false;
    return strcmp(a, b) == 0;
}

static char* dup_dirname(const char* path) {
    if (!path || !*path) return NULL;
    const char* slash = strrchr(path, '/');
    if (!slash || slash == path) return NULL;
    size_t len = (size_t)(slash - path);
    char* out = (char*)malloc(len + 1);
    if (!out) return NULL;
    memcpy(out, path, len);
    out[len] = '\0';
    return out;
}

static char* infer_src_root(const char* path) {
    if (!path || !*path) return NULL;
    const char* p = strstr(path, "/src/");
    if (!p) return NULL;
    size_t prefix = (size_t)(p - path);
    size_t len = prefix + strlen("/src");
    char* out = (char*)malloc(len + 1);
    if (!out) return NULL;
    memcpy(out, path, len);
    out[len] = '\0';
    return out;
}

static bool contains_path(const char* const* items, size_t count, const char* path) {
    if (!items || !path) return false;
    for (size_t i = 0; i < count; ++i) {
        if (str_equals(items[i], path)) return true;
    }
    return false;
}

static uint64_t fnv1a64(const char* data, size_t len) {
    const uint64_t basis = 1469598103934665603ULL;
    const uint64_t prime = 1099511628211ULL;
    uint64_t hash = basis;
    if (!data || len == 0) return hash;
    for (size_t i = 0; i < len; ++i) {
        hash ^= (uint8_t)data[i];
        hash *= prime;
    }
    return hash;
}

static uint64_t fnv1a64_mix_u64(uint64_t hash, uint64_t value) {
    const uint64_t prime = 1099511628211ULL;
    for (int i = 0; i < 8; ++i) {
        hash ^= (uint8_t)((value >> (i * 8)) & 0xFFu);
        hash *= prime;
    }
    return hash;
}

static uint64_t fnv1a64_mix_cstr(uint64_t hash, const char* s) {
    const uint64_t prime = 1099511628211ULL;
    const char* text = s ? s : "";
    size_t len = strlen(text);
    hash = fnv1a64_mix_u64(hash, (uint64_t)len);
    for (size_t i = 0; i < len; ++i) {
        hash ^= (uint8_t)text[i];
        hash *= prime;
    }
    return hash;
}

static uint64_t compute_symbol_stable_id(const FisicsSymbol* symbol) {
    if (!symbol) return 0;
    uint64_t hash = 1469598103934665603ULL; // FNV-1a 64 offset basis
    hash = fnv1a64_mix_cstr(hash, symbol->name);
    hash = fnv1a64_mix_cstr(hash, symbol->file_path);
    hash = fnv1a64_mix_u64(hash, (uint64_t)symbol->kind);
    hash = fnv1a64_mix_cstr(hash, symbol->parent_name);
    hash = fnv1a64_mix_u64(hash, (uint64_t)symbol->parent_kind);
    hash = fnv1a64_mix_u64(hash, (uint64_t)symbol->start_line);
    hash = fnv1a64_mix_u64(hash, (uint64_t)symbol->start_col);
    hash = fnv1a64_mix_u64(hash, (uint64_t)symbol->end_line);
    hash = fnv1a64_mix_u64(hash, (uint64_t)symbol->end_col);
    hash = fnv1a64_mix_u64(hash, symbol->is_definition ? 1u : 0u);
    hash = fnv1a64_mix_u64(hash, symbol->is_variadic ? 1u : 0u);
    hash = fnv1a64_mix_cstr(hash, symbol->return_type);
    hash = fnv1a64_mix_u64(hash, (uint64_t)symbol->param_count);
    for (size_t i = 0; i < symbol->param_count; ++i) {
        const char* ptype = (symbol->param_types && symbol->param_types[i]) ? symbol->param_types[i] : NULL;
        const char* pname = (symbol->param_names && symbol->param_names[i]) ? symbol->param_names[i] : NULL;
        hash = fnv1a64_mix_cstr(hash, ptype);
        hash = fnv1a64_mix_cstr(hash, pname);
    }
    return hash;
}

static bool symbol_span_is_known(const FisicsSymbol* sym) {
    if (!sym) return false;
    if (sym->start_line <= 0 || sym->start_col <= 0) return false;
    if (sym->end_line <= 0 || sym->end_col <= 0) return false;
    if (sym->end_line < sym->start_line) return false;
    if (sym->end_line == sym->start_line && sym->end_col < sym->start_col) return false;
    return true;
}

static bool symbol_range_contains(const FisicsSymbol* parent, const FisicsSymbol* child) {
    if (!symbol_span_is_known(parent) || !symbol_span_is_known(child)) return false;
    if (child->start_line < parent->start_line) return false;
    if (child->start_line == parent->start_line && child->start_col < parent->start_col) return false;
    if (child->end_line > parent->end_line) return false;
    if (child->end_line == parent->end_line && child->end_col > parent->end_col) return false;
    return true;
}

static uint64_t symbol_span_measure(const FisicsSymbol* sym) {
    if (!symbol_span_is_known(sym)) return UINT64_MAX;
    uint64_t line_span = (uint64_t)(sym->end_line - sym->start_line);
    uint64_t col_span = 0;
    if (sym->end_line == sym->start_line) {
        col_span = (uint64_t)(sym->end_col - sym->start_col);
    } else {
        col_span = (uint64_t)sym->end_col;
    }
    return (line_span << 20) + col_span;
}

static bool symbol_same_file(const FisicsSymbol* a, const FisicsSymbol* b) {
    if (!a || !b) return false;
    if (!a->file_path || !b->file_path) return false;
    return strcmp(a->file_path, b->file_path) == 0;
}

static void resolve_parent_stable_links(FisicsSymbol* symbols, size_t count) {
    if (!symbols || count == 0) return;
    for (size_t i = 0; i < count; ++i) {
        FisicsSymbol* child = &symbols[i];
        if (child->parent_stable_id) continue;
        if (!child->parent_name || !child->parent_name[0]) continue;
        if (child->parent_kind == FISICS_SYMBOL_UNKNOWN) continue;

        ssize_t best = -1;
        uint64_t best_measure = UINT64_MAX;

        for (size_t j = 0; j < count; ++j) {
            if (i == j) continue;
            const FisicsSymbol* cand = &symbols[j];
            if (!cand->stable_id) continue;
            if (cand->kind != child->parent_kind) continue;
            if (!cand->name || strcmp(cand->name, child->parent_name) != 0) continue;

            bool same_file = symbol_same_file(cand, child);
            bool contains = symbol_range_contains(cand, child);
            if (child->file_path && child->file_path[0] && !same_file) continue;
            if (symbol_span_is_known(cand) && symbol_span_is_known(child) && !contains) continue;

            uint64_t measure = symbol_span_measure(cand);
            if (best < 0 || measure < best_measure) {
                best = (ssize_t)j;
                best_measure = measure;
            }
        }

        if (best >= 0) {
            child->parent_stable_id = symbols[(size_t)best].stable_id;
        }
    }
}

static char* make_absolute_path(const char* path) {
    if (!path || !path[0]) return NULL;
    if (path[0] == '/') return strdup(path);
    char cwd[PATH_MAX];
    if (!getcwd(cwd, sizeof(cwd))) return strdup(path);
    size_t needed = (size_t)snprintf(NULL, 0, "%s/%s", cwd, path);
    char* out = (char*)malloc(needed + 1);
    if (!out) return NULL;
    snprintf(out, needed + 1, "%s/%s", cwd, path);
    return out;
}

static void init_contract(FisicsAnalysisResult* out,
                          const char* source,
                          size_t length,
                          bool lenient,
                          bool ok) {
    if (!out) return;
    memset(&out->contract, 0, sizeof(out->contract));
    snprintf(out->contract.contract_id,
             sizeof(out->contract.contract_id),
             "%s",
             FISICS_CONTRACT_ID);
    out->contract.contract_major = FISICS_CONTRACT_MAJOR;
    out->contract.contract_minor = FISICS_CONTRACT_MINOR;
    out->contract.contract_patch = FISICS_CONTRACT_PATCH;
    snprintf(out->contract.producer_name,
             sizeof(out->contract.producer_name),
             "%s",
             FISICS_CONTRACT_PRODUCER);
    snprintf(out->contract.producer_version,
             sizeof(out->contract.producer_version),
             "%s",
             FISICS_CONTRACT_PRODUCER_VERSION);
    out->contract.mode = lenient ? FISICS_ANALYSIS_MODE_LENIENT : FISICS_ANALYSIS_MODE_STRICT;
    out->contract.partial = !ok;
    out->contract.fatal = !ok;
    out->contract.source_hash = fnv1a64(source, length);
    out->contract.source_length = (uint64_t)length;
    out->contract.capabilities =
        FISICS_CONTRACT_CAP_DIAGNOSTICS |
        FISICS_CONTRACT_CAP_INCLUDES |
        FISICS_CONTRACT_CAP_SYMBOLS |
        FISICS_CONTRACT_CAP_TOKENS |
        FISICS_CONTRACT_CAP_SYMBOL_PARENT_STABLE_ID |
        FISICS_CONTRACT_CAP_DIAGNOSTIC_TAXONOMY;
}

typedef struct {
    char** items;
    size_t count;
    size_t capacity;
} DiagKeyCache;

static void safe_free_ptr(void* ptr) {
    if (!ptr) return;
#ifdef __APPLE__
    /* Guard against accidental non-heap pointers from upstream metadata. */
    if (!malloc_zone_from_ptr(ptr)) {
        return;
    }
#endif
    free(ptr);
}

static bool diag_cache_enabled(void) {
    static int cached = -1;
    if (cached >= 0) return cached != 0;
    const char* env = getenv("FISICS_DIAG_DEDUP_GLOBAL");
    cached = (env && env[0] && env[0] != '0') ? 1 : 0;
    return cached != 0;
}

static void diag_cache_clear(DiagKeyCache* cache) {
    if (!cache || !cache->items) return;
    for (size_t i = 0; i < cache->count; ++i) {
        free(cache->items[i]);
    }
    free(cache->items);
    cache->items = NULL;
    cache->count = 0;
    cache->capacity = 0;
}

static bool diag_cache_contains(const DiagKeyCache* cache, const char* key) {
    if (!cache || !key) return false;
    for (size_t i = 0; i < cache->count; ++i) {
        if (cache->items[i] && strcmp(cache->items[i], key) == 0) {
            return true;
        }
    }
    return false;
}

static bool diag_cache_add(DiagKeyCache* cache, char* key) {
    if (!cache || !key) return false;
    if (cache->count == cache->capacity) {
        size_t newCap = cache->capacity ? cache->capacity * 2 : 64;
        char** grown = (char**)realloc(cache->items, newCap * sizeof(char*));
        if (!grown) return false;
        cache->items = grown;
        cache->capacity = newCap;
    }
    cache->items[cache->count++] = key;
    return true;
}

static char* diag_key(const FisicsDiagnostic* d) {
    const char* path = d && d->file_path ? d->file_path : "<unknown>";
    const char* msg = d && d->message ? d->message : "<unknown>";
    const char* hint = d && d->hint ? d->hint : "";
    int line = d ? d->line : 0;
    int col = d ? d->column : 0;
    int kind = d ? (int)d->kind : 0;
    int needed = snprintf(NULL, 0, "%s:%d:%d:%d:%s:%s", path, line, col, kind, msg, hint);
    if (needed <= 0) return NULL;
    char* key = (char*)malloc((size_t)needed + 1);
    if (!key) return NULL;
    snprintf(key, (size_t)needed + 1, "%s:%d:%d:%d:%s:%s", path, line, col, kind, msg, hint);
    return key;
}

static bool should_suppress_diag_lenient(const FisicsDiagnostic* d, bool lenient) {
    if (!d) return false;
    const bool is_vk_renderer_ref_backup = d->file_path &&
                                           strstr(d->file_path, "/vk_renderer_ref_backup/") != NULL;
    if (is_vk_renderer_ref_backup) {
        // Backup/reference renderer sources are not part of active builds or analysis signal.
        return true;
    }
    if (!lenient) return false;
    if (!d->message) return false;
    const bool is_media_clip = d->file_path &&
                               strstr(d->file_path, "/src/audio/media_clip.c") != NULL;
    if (d->line == 0 &&
        strstr(d->message, "Bitfield width exceeds type width") != NULL) {
        // This is typically a synthetic/system-header artifact in lenient mode.
        return true;
    }
    if (strstr(d->message, "Undeclared identifier") != NULL) {
        if (d->hint && strcmp(d->hint, "__builtin_constant_p") == 0) {
            // Darwin SDK byte-order macros can surface this in lenient scan mode via macro-call remapping.
            return true;
        }
        if (d->file_path && strstr(d->file_path, "/libkern/OSByteOrder.h") != NULL &&
            d->hint && strncmp(d->hint, "__DARWIN_OSSwapInt", strlen("__DARWIN_OSSwapInt")) == 0) {
            // Apple byte-order macro forwarding token mapped into user file context.
            return true;
        }
    }
    if (d->line == 0 && d->column == 0 &&
        strstr(d->message, "macro expansion failed (possible recursion) for '<macro>'") != NULL) {
        return true;
    }
    if (strstr(d->message, "Do not know the endianess of this architecture") != NULL) {
        return true;
    }
    if (strstr(d->message, "Both __BIG_ENDIAN__ and __LITTLE_ENDIAN__ cannot be false") != NULL) {
        return true;
    }
    if (strstr(d->message, "Unknown endianess") != NULL) {
        return true;
    }
    if (is_media_clip) {
        if (strstr(d->message, "macro expansion fallback used (continuing in lenient mode)") != NULL) {
            return true;
        }
        if (strstr(d->message, "Conflicting definition of union 'CFSwap'") != NULL) {
            return true;
        }
        if (strstr(d->message, "Variable has incomplete type") != NULL) {
            return true;
        }
        if (strstr(d->message, "Undeclared identifier") != NULL) {
            return true;
        }
        if (strstr(d->message, "expected ';' after declaration") != NULL) {
            return true;
        }
        if (strstr(d->message, "Argument 2 of 'CFStringGetCStringPtr' has incompatible type") != NULL) {
            return true;
        }
        if (strstr(d->message, "Argument 2 of 'ExtAudioFileGetProperty' has incompatible type") != NULL) {
            return true;
        }
        if (strstr(d->message, "Argument 2 of 'ExtAudioFileSetProperty' has incompatible type") != NULL) {
            return true;
        }
        if (strstr(d->message, "Control reaches end of non-void function 'NumBytesToNumAudioFileMarkers'") != NULL) {
            return true;
        }
    }
    return false;
}

static bool copy_diagnostics_filtered(const CompilerContext* ctx,
                                      const char* callerFilePath,
                                      bool lenient,
                                      FisicsAnalysisResult* out) {
    static DiagKeyCache cache = {0};
    const size_t maxKeys = 4096;

    size_t count = 0;
    const FisicsDiagnostic* src = compiler_diagnostics_data(ctx, &count);
    if (count == 0) {
        out->diagnostics = NULL;
        out->diag_count = 0;
        return true;
    }

    FisicsDiagnostic* dst = (FisicsDiagnostic*)calloc(count, sizeof(FisicsDiagnostic));
    if (!dst) return false;
    size_t kept = 0;
    for (size_t i = 0; i < count; ++i) {
        if (should_suppress_diag_lenient(&src[i], lenient)) {
            continue;
        }
        char* key = diag_key(&src[i]);
        if (!key) {
            continue;
        }
        if (diag_cache_contains(&cache, key)) {
            free(key);
            continue;
        }
        if (!diag_cache_add(&cache, key)) {
            free(key);
            continue;
        }
        if (cache.count > maxKeys) {
            diag_cache_clear(&cache);
        }

        dst[kept] = src[i];
        dst[kept].file_path = NULL;
        dst[kept].message = NULL;
        dst[kept].hint = NULL;

        const char* chosenPath = src[i].file_path ? src[i].file_path : callerFilePath;
        if (chosenPath) {
            dst[kept].file_path = make_absolute_path(chosenPath);
            if (!dst[kept].file_path) {
                dst[kept].file_path = strdup(chosenPath);
            }
            if (!dst[kept].file_path) {
                for (size_t j = 0; j <= kept; ++j) {
                    free((char*)dst[j].file_path);
                    free(dst[j].message);
                    free(dst[j].hint);
                }
                free(dst);
                return false;
            }
        }
        if (src[i].message) {
            dst[kept].message = strdup(src[i].message);
            if (!dst[kept].message) {
                for (size_t j = 0; j <= kept; ++j) {
                    free((char*)dst[j].file_path);
                    free(dst[j].message);
                    free(dst[j].hint);
                }
                free(dst);
                return false;
            }
        }
        if (src[i].hint) {
            dst[kept].hint = strdup(src[i].hint);
            if (!dst[kept].hint) {
                for (size_t j = 0; j <= kept; ++j) {
                    free((char*)dst[j].file_path);
                    free(dst[j].message);
                    free(dst[j].hint);
                }
                free(dst);
                return false;
            }
        }
        kept++;
    }

    if (kept == 0) {
        free(dst);
        out->diagnostics = NULL;
        out->diag_count = 0;
        return true;
    }

    out->diagnostics = dst;
    out->diag_count = kept;
    return true;
}

// Copy diagnostics out of the compiler context. We purposefully override the
// file_path with the caller's file_path to avoid dangling pointers from the
// preprocessor/include resolver teardown.
static bool copy_diagnostics(const CompilerContext* ctx,
                             const char* callerFilePath,
                             bool lenient,
                             FisicsAnalysisResult* out) {
    if (diag_cache_enabled()) {
        return copy_diagnostics_filtered(ctx, callerFilePath, lenient, out);
    }
    size_t count = 0;
    const FisicsDiagnostic* src = compiler_diagnostics_data(ctx, &count);
    if (count == 0) {
        out->diagnostics = NULL;
        out->diag_count = 0;
        return true;
    }
    FisicsDiagnostic* dst = (FisicsDiagnostic*)calloc(count, sizeof(FisicsDiagnostic));
    if (!dst) return false;
    size_t kept = 0;
    for (size_t i = 0; i < count; ++i) {
        if (should_suppress_diag_lenient(&src[i], lenient)) {
            continue;
        }
        // Copy POD fields directly.
        dst[kept].line   = src[i].line;
        dst[kept].column = src[i].column;
        dst[kept].length = src[i].length;
        dst[kept].kind   = src[i].kind;
        dst[kept].code   = src[i].code;
        dst[kept].severity_id = src[i].severity_id;
        dst[kept].category_id = src[i].category_id;
        dst[kept].code_id = src[i].code_id;

        const char* chosenPath = src[i].file_path ? src[i].file_path : callerFilePath;
        if (chosenPath) {
            dst[kept].file_path = make_absolute_path(chosenPath);
            if (!dst[kept].file_path) {
                dst[kept].file_path = strdup(chosenPath);
            }
            if (!dst[kept].file_path) {
                out->diag_count = kept;
                fisics_free_analysis_result(out);
                return false;
            }
        } else {
            dst[kept].file_path = NULL;
        }

        if (src[i].message) {
            dst[kept].message = strdup(src[i].message);
            if (!dst[kept].message) {
                out->diag_count = kept;
                fisics_free_analysis_result(out);
                return false;
            }
        }
        if (src[i].hint) {
            dst[kept].hint = strdup(src[i].hint);
            if (!dst[kept].hint) {
                out->diag_count = kept + 1;
                fisics_free_analysis_result(out);
                return false;
            }
        }
        kept++;
    }
    if (kept == 0) {
        free(dst);
        out->diagnostics = NULL;
        out->diag_count = 0;
        return true;
    }
    out->diagnostics = dst;
    out->diag_count = kept;
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
            dst[i].file_path = make_absolute_path(src[i].file_path);
            if (!dst[i].file_path) {
                dst[i].file_path = dupstr(src[i].file_path);
            }
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
        dst[i].stable_id = src[i].stable_id ? src[i].stable_id : compute_symbol_stable_id(&dst[i]);
    }
    resolve_parent_stable_links(dst, count);
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
            dst[i].resolved_path = make_absolute_path(src[i].resolved_path);
            if (!dst[i].resolved_path) {
                dst[i].resolved_path = dupstr(src[i].resolved_path);
            }
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
    if (!out || !file_path || !source) return false;
    memset(out, 0, sizeof(*out));
    char* absolute_file_path = make_absolute_path(file_path);
    const char* contract_file_path = absolute_file_path ? absolute_file_path : file_path;
    CompilerContext* ctx = cc_create();
    if (!ctx) {
        free(absolute_file_path);
        return false;
    }

    struct ASTNode* ast = NULL;
    struct SemanticModel* model = NULL;
    size_t semaErrors = 0;

    bool lenient = true;
    if (opts) {
        if (opts->lenient_mode < 0) lenient = false;
        else if (opts->lenient_mode > 0) lenient = true;
    }
    bool includeSystemSymbols = opts && opts->include_system_symbols;
    const char* const* include_paths = opts ? opts->include_paths : NULL;
    size_t include_path_count = opts ? opts->include_path_count : 0;
    char* inferred_dir = dup_dirname(contract_file_path);
    char* inferred_src = infer_src_root(contract_file_path);
    const char** merged_include_paths = NULL;
    size_t merged_count = include_path_count;

    if (inferred_dir || inferred_src) {
        size_t cap = include_path_count + 2;
        merged_include_paths = (const char**)calloc(cap, sizeof(const char*));
        if (merged_include_paths) {
            for (size_t i = 0; i < include_path_count; ++i) {
                merged_include_paths[i] = include_paths[i];
            }
            merged_count = include_path_count;
            if (inferred_dir && !contains_path(merged_include_paths, merged_count, inferred_dir)) {
                merged_include_paths[merged_count++] = inferred_dir;
            }
            if (inferred_src && !contains_path(merged_include_paths, merged_count, inferred_src)) {
                merged_include_paths[merged_count++] = inferred_src;
            }
            include_paths = merged_include_paths;
            include_path_count = merged_count;
        }
    }

    bool ok = compiler_run_frontend(ctx,
                                    contract_file_path,
                                    source,
                                    length,
                                    false,
                                    false,
                                    include_paths,
                                    include_path_count,
                                    opts ? opts->macro_defines : NULL,
                                    opts ? opts->macro_define_count : 0,
                                    lenient,
                                    includeSystemSymbols,
                                    false,
                                    false,
                                    false,
                                    &ast,
                                    &model,
                                    &semaErrors);
    (void)ast;
    (void)model;
    bool copied = false;
    if (ok) {
        copied = copy_diagnostics(ctx, contract_file_path, lenient, out) &&
                 copy_tokens(ctx, out) &&
                 copy_symbols(ctx, out) &&
                 copy_includes(ctx, out);
    } else {
        // IDE lenient path: even if the pipeline failed (e.g., missing headers or parse
        // errors), return whatever diagnostics/includes we captured so the IDE can show them.
        copied = copy_diagnostics(ctx, contract_file_path, lenient, out) &&
                 copy_tokens(ctx, out) &&
                 copy_symbols(ctx, out) &&
                 copy_includes(ctx, out);
    }
    init_contract(out, source, length, lenient, ok);

    cc_destroy(ctx);
    free(merged_include_paths);
    free(inferred_dir);
    free(inferred_src);
    free(absolute_file_path);
    return copied && ok ? true : copied;
}

void fisics_free_analysis_result(FisicsAnalysisResult* result) {
    if (!result) return;
    if (result->diagnostics) {
        for (size_t i = 0; i < result->diag_count; ++i) {
            safe_free_ptr((void*)result->diagnostics[i].file_path);
            safe_free_ptr(result->diagnostics[i].message);
            safe_free_ptr(result->diagnostics[i].hint);
        }
        safe_free_ptr(result->diagnostics);
    }
    if (result->tokens) {
        safe_free_ptr(result->tokens);
    }
    if (result->symbols) {
        for (size_t i = 0; i < result->symbol_count; ++i) {
            safe_free_ptr((void*)result->symbols[i].name);
            safe_free_ptr((void*)result->symbols[i].file_path);
            safe_free_ptr((void*)result->symbols[i].parent_name);
            safe_free_ptr((void*)result->symbols[i].return_type);
            if (result->symbols[i].param_types) {
                for (size_t p = 0; p < result->symbols[i].param_count; ++p) {
                    safe_free_ptr((void*)result->symbols[i].param_types[p]);
                }
                safe_free_ptr((void*)result->symbols[i].param_types);
            }
            if (result->symbols[i].param_names) {
                for (size_t p = 0; p < result->symbols[i].param_count; ++p) {
                    safe_free_ptr((void*)result->symbols[i].param_names[p]);
                }
                safe_free_ptr((void*)result->symbols[i].param_names);
            }
        }
        safe_free_ptr(result->symbols);
    }
    if (result->includes) {
        for (size_t i = 0; i < result->include_count; ++i) {
            safe_free_ptr((void*)result->includes[i].name);
            safe_free_ptr((void*)result->includes[i].resolved_path);
        }
        safe_free_ptr(result->includes);
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
