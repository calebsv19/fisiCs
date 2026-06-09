// SPDX-License-Identifier: Apache-2.0

#include "Compiler/build_graph.h"

#include "Preprocessor/include_resolver.h"
#include "core_io.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct BuildGraphJsonBuilder {
    char* data;
    size_t len;
    size_t cap;
} BuildGraphJsonBuilder;

typedef struct BuildGraphDiagnosticSummary {
    bool available;
    size_t total;
    size_t errors;
    size_t warnings;
    size_t notes;
    bool partial;
    bool fatal;
} BuildGraphDiagnosticSummary;

static bool bg_reserve(BuildGraphJsonBuilder* b, size_t extra) {
    if (!b || extra > SIZE_MAX - b->len) return false;
    size_t need = b->len + extra;
    if (need <= b->cap) return true;
    size_t newCap = b->cap ? b->cap : 512u;
    while (newCap < need) {
        if (newCap > SIZE_MAX / 2u) {
            newCap = need;
            break;
        }
        newCap *= 2u;
    }
    char* grown = (char*)realloc(b->data, newCap);
    if (!grown) return false;
    b->data = grown;
    b->cap = newCap;
    return true;
}

static bool bg_append_raw(BuildGraphJsonBuilder* b, const char* s) {
    if (!b || !s) return false;
    size_t len = strlen(s);
    if (!bg_reserve(b, len)) return false;
    memcpy(b->data + b->len, s, len);
    b->len += len;
    return true;
}

static bool bg_append_char(BuildGraphJsonBuilder* b, char c) {
    if (!bg_reserve(b, 1u)) return false;
    b->data[b->len++] = c;
    return true;
}

static bool bg_append_bool(BuildGraphJsonBuilder* b, bool value) {
    return bg_append_raw(b, value ? "true" : "false");
}

static bool bg_append_escaped(BuildGraphJsonBuilder* b, const char* s) {
    if (!s) return bg_append_raw(b, "null");
    if (!bg_append_char(b, '"')) return false;
    for (const unsigned char* p = (const unsigned char*)s; *p; ++p) {
        if (*p == '\\' || *p == '"') {
            if (!bg_append_char(b, '\\')) return false;
            if (!bg_append_char(b, (char)*p)) return false;
        } else if (*p == '\n') {
            if (!bg_append_raw(b, "\\n")) return false;
        } else if (*p == '\r') {
            if (!bg_append_raw(b, "\\r")) return false;
        } else if (*p == '\t') {
            if (!bg_append_raw(b, "\\t")) return false;
        } else if (*p < 0x20u) {
            if (!bg_append_raw(b, "\\u00")) return false;
            static const char hex[] = "0123456789abcdef";
            if (!bg_append_char(b, hex[(*p >> 4) & 0xfu])) return false;
            if (!bg_append_char(b, hex[*p & 0xfu])) return false;
        } else {
            if (!bg_append_char(b, (char)*p)) return false;
        }
    }
    return bg_append_char(b, '"');
}

static bool bg_append_string_array(BuildGraphJsonBuilder* b,
                                   const char* const* values,
                                   size_t count) {
    if (!bg_append_char(b, '[')) return false;
    for (size_t i = 0; i < count; ++i) {
        if (i && !bg_append_char(b, ',')) return false;
        if (!bg_append_escaped(b, values[i])) return false;
    }
    return bg_append_char(b, ']');
}

static BuildGraphDiagnosticSummary bg_diagnostic_summary_from_ctx(const CompilerContext* ctx,
                                                                  bool partial,
                                                                  bool fatal) {
    BuildGraphDiagnosticSummary summary = {
        .available = ctx != NULL,
        .partial = partial,
        .fatal = fatal
    };
    size_t count = 0;
    const FisicsDiagnostic* diags = compiler_diagnostics_data(ctx, &count);
    summary.total = count;
    for (size_t i = 0; i < count; ++i) {
        switch (diags[i].kind) {
            case DIAG_ERROR:
                summary.errors += 1u;
                break;
            case DIAG_WARNING:
                summary.warnings += 1u;
                break;
            case DIAG_NOTE:
                summary.notes += 1u;
                break;
        }
    }
    return summary;
}

static BuildGraphDiagnosticSummary bg_diagnostic_summary_unavailable(bool partial,
                                                                     bool fatal) {
    BuildGraphDiagnosticSummary summary = {
        .available = false,
        .partial = partial,
        .fatal = fatal
    };
    return summary;
}

static bool bg_append_diagnostic_summary(BuildGraphJsonBuilder* b,
                                         const BuildGraphDiagnosticSummary* summary) {
    if (!summary) return false;
    char num[64];
    if (!bg_append_raw(b, "{\"available\":")) return false;
    if (!bg_append_bool(b, summary->available)) return false;
    snprintf(num, sizeof(num), "%zu", summary->total);
    if (!bg_append_raw(b, ",\"total\":")) return false;
    if (!bg_append_raw(b, num)) return false;
    snprintf(num, sizeof(num), "%zu", summary->errors);
    if (!bg_append_raw(b, ",\"errors\":")) return false;
    if (!bg_append_raw(b, num)) return false;
    snprintf(num, sizeof(num), "%zu", summary->warnings);
    if (!bg_append_raw(b, ",\"warnings\":")) return false;
    if (!bg_append_raw(b, num)) return false;
    snprintf(num, sizeof(num), "%zu", summary->notes);
    if (!bg_append_raw(b, ",\"notes\":")) return false;
    if (!bg_append_raw(b, num)) return false;
    if (!bg_append_raw(b, ",\"partial\":")) return false;
    if (!bg_append_bool(b, summary->partial)) return false;
    if (!bg_append_raw(b, ",\"fatal\":")) return false;
    if (!bg_append_bool(b, summary->fatal)) return false;
    return bg_append_char(b, '}');
}

static bool bg_append_manifest_string_list(BuildGraphJsonBuilder* b,
                                           const FisicsBuildManifestStringList* list) {
    return bg_append_string_array(b,
                                  list ? (const char* const*)list->items : NULL,
                                  list ? list->count : 0u);
}

static bool bg_append_arg(BuildGraphJsonBuilder* b, bool* wrote, const char* arg) {
    if (*wrote && !bg_append_char(b, ',')) return false;
    if (!bg_append_escaped(b, arg)) return false;
    *wrote = true;
    return true;
}

static bool bg_append_arg2(BuildGraphJsonBuilder* b, bool* wrote, const char* a, const char* c) {
    return bg_append_arg(b, wrote, a) && bg_append_arg(b, wrote, c);
}

static const char* bg_dialect_name(CCDialect dialect) {
    switch (dialect) {
        case CC_DIALECT_C99: return "c99";
        case CC_DIALECT_C11: return "c11";
        case CC_DIALECT_C17: return "c17";
    }
    return "unknown";
}

static bool bg_overlay_enabled(FisicsOverlayFeatures features,
                               FisicsOverlayFeatures flag) {
    return (features & flag) != 0;
}

static bool bg_append_overlays(BuildGraphJsonBuilder* b, FisicsOverlayFeatures features) {
    bool wrote = false;
    if (!bg_append_char(b, '[')) return false;
#define APPEND_OVERLAY(flag, name) \
    do { \
        if (bg_overlay_enabled(features, (flag))) { \
            if (wrote && !bg_append_char(b, ',')) return false; \
            if (!bg_append_escaped(b, (name))) return false; \
            wrote = true; \
        } \
    } while (0)
    APPEND_OVERLAY(FISICS_OVERLAY_PHYSICS_UNITS, "physics-units");
    APPEND_OVERLAY(FISICS_OVERLAY_IDE_METADATA, "ide-metadata");
    APPEND_OVERLAY(FISICS_OVERLAY_MEMORY_CHECK, "memory-check");
#undef APPEND_OVERLAY
    return bg_append_char(b, ']');
}

static bool bg_append_compat(BuildGraphJsonBuilder* b, CCCompatFeatures features) {
    bool wrote = false;
    if (!bg_append_char(b, '[')) return false;
#define APPEND_COMPAT(flag, name) \
    do { \
        if ((features & (flag)) != 0) { \
            if (wrote && !bg_append_char(b, ',')) return false; \
            if (!bg_append_escaped(b, (name))) return false; \
            wrote = true; \
        } \
    } while (0)
    APPEND_COMPAT(CC_COMPAT_PROFILE_GNU, "gnu-profile");
    APPEND_COMPAT(CC_COMPAT_BLOCK_POINTERS, "block-pointers");
    APPEND_COMPAT(CC_COMPAT_RELAXED_ATOMIC, "relaxed-atomic");
#undef APPEND_COMPAT
    return bg_append_char(b, ']');
}

static bool bg_append_compile_args(BuildGraphJsonBuilder* b,
                                   const FisicsBuildGraphSourceOptions* options) {
    bool wrote = false;
    if (!bg_append_char(b, '[')) return false;
    if (!bg_append_arg(b, &wrote, "fisics")) return false;

    switch (options->dialect) {
        case CC_DIALECT_C11:
            if (!bg_append_arg(b, &wrote, "-std=c11")) return false;
            break;
        case CC_DIALECT_C17:
            if (!bg_append_arg(b, &wrote, "-std=c17")) return false;
            break;
        case CC_DIALECT_C99:
        default:
            if (!bg_append_arg(b, &wrote, "-std=c99")) return false;
            break;
    }

    if (options->targetTriple && options->targetTriple[0]) {
        if (!bg_append_arg2(b, &wrote, "--target", options->targetTriple)) return false;
    }
    if (options->dataLayout && options->dataLayout[0]) {
        if (!bg_append_arg2(b, &wrote, "--data-layout", options->dataLayout)) return false;
    }
    for (size_t i = 0; i < options->includePathCount; ++i) {
        if (options->includePaths[i] && options->includePaths[i][0]) {
            if (!bg_append_arg2(b, &wrote, "-I", options->includePaths[i])) return false;
        }
    }
    for (size_t i = 0; i < options->macroDefineCount; ++i) {
        if (options->macroDefines[i] && options->macroDefines[i][0]) {
            if (!bg_append_arg2(b, &wrote, "-D", options->macroDefines[i])) return false;
        }
    }
    for (size_t i = 0; i < options->forcedIncludeCount; ++i) {
        if (options->forcedIncludes[i] && options->forcedIncludes[i][0]) {
            if (!bg_append_arg2(b, &wrote, "-include", options->forcedIncludes[i])) return false;
        }
    }
    if (options->overlayFeatures) {
        if (!bg_append_arg(b, &wrote, "--overlay=<profile>")) return false;
    }
    if (options->compileOnly) {
        if (!bg_append_arg(b, &wrote, "-c")) return false;
    }
    if (options->inputPath && !bg_append_arg(b, &wrote, options->inputPath)) return false;
    if (options->outputObject) {
        if (!bg_append_arg2(b, &wrote, "-o", options->outputObject)) return false;
    }
    return bg_append_char(b, ']');
}

static const char* bg_manifest_standard(const FisicsBuildManifest* manifest) {
    if (manifest && manifest->defaults.standard && manifest->defaults.standard[0]) {
        return manifest->defaults.standard;
    }
    return "c99";
}

static bool bg_append_manifest_overlay_arg(BuildGraphJsonBuilder* b,
                                           bool* wrote,
                                           const FisicsBuildManifestStringList* overlays) {
    if (!overlays || overlays->count == 0) return true;
    BuildGraphJsonBuilder joined = {0};
    for (size_t i = 0; i < overlays->count; ++i) {
        if (i && !bg_append_char(&joined, ',')) goto fail;
        if (!bg_append_raw(&joined, overlays->items[i])) goto fail;
    }
    if (!bg_append_char(&joined, '\0')) goto fail;
    bool ok = bg_append_arg2(b, wrote, "--overlay", joined.data);
    free(joined.data);
    return ok;
fail:
    free(joined.data);
    return false;
}

static bool bg_append_manifest_compile_args(BuildGraphJsonBuilder* b,
                                            const FisicsBuildManifest* manifest,
                                            const FisicsBuildManifestTranslationUnit* tu) {
    bool wrote = false;
    if (!bg_append_char(b, '[')) return false;
    if (!bg_append_arg(b, &wrote, "fisics")) return false;

    char stdArg[64];
    snprintf(stdArg, sizeof(stdArg), "-std=%s", bg_manifest_standard(manifest));
    if (!bg_append_arg(b, &wrote, stdArg)) return false;

    for (size_t i = 0; i < manifest->defaults.includeDirs.count; ++i) {
        if (!bg_append_arg2(b, &wrote, "-I", manifest->defaults.includeDirs.items[i])) return false;
    }
    for (size_t i = 0; i < manifest->defaults.defines.count; ++i) {
        if (!bg_append_arg2(b, &wrote, "-D", manifest->defaults.defines.items[i])) return false;
    }
    if (!bg_append_manifest_overlay_arg(b, &wrote, &manifest->defaults.overlays)) return false;
    if (!bg_append_arg(b, &wrote, "-c")) return false;
    if (!bg_append_arg(b, &wrote, tu->resolvedSource ? tu->resolvedSource : tu->source)) return false;
    if (tu->resolvedObject) {
        if (!bg_append_arg2(b, &wrote, "-o", tu->resolvedObject)) return false;
    }
    return bg_append_char(b, ']');
}

static bool bg_append_manifest_compile_command(BuildGraphJsonBuilder* b,
                                               const FisicsBuildManifest* manifest,
                                               const FisicsBuildManifestTranslationUnit* tu) {
    BuildGraphJsonBuilder cmd = {0};
    bool wrote = false;
#define APPEND_CMD_ARG(arg_text) \
    do { \
        if (wrote && !bg_append_char(&cmd, ' ')) goto fail; \
        if (!bg_append_raw(&cmd, (arg_text))) goto fail; \
        wrote = true; \
    } while (0)
#define APPEND_CMD_ARG2(a_text, b_text) \
    do { \
        APPEND_CMD_ARG((a_text)); \
        APPEND_CMD_ARG((b_text)); \
    } while (0)

    APPEND_CMD_ARG("fisics");
    char stdArg[64];
    snprintf(stdArg, sizeof(stdArg), "-std=%s", bg_manifest_standard(manifest));
    APPEND_CMD_ARG(stdArg);
    for (size_t i = 0; i < manifest->defaults.includeDirs.count; ++i) {
        APPEND_CMD_ARG2("-I", manifest->defaults.includeDirs.items[i]);
    }
    for (size_t i = 0; i < manifest->defaults.defines.count; ++i) {
        APPEND_CMD_ARG2("-D", manifest->defaults.defines.items[i]);
    }
    if (manifest->defaults.overlays.count > 0) {
        BuildGraphJsonBuilder joined = {0};
        for (size_t i = 0; i < manifest->defaults.overlays.count; ++i) {
            if (i && !bg_append_char(&joined, ',')) {
                free(joined.data);
                goto fail;
            }
            if (!bg_append_raw(&joined, manifest->defaults.overlays.items[i])) {
                free(joined.data);
                goto fail;
            }
        }
        if (!bg_append_char(&joined, '\0')) {
            free(joined.data);
            goto fail;
        }
        APPEND_CMD_ARG2("--overlay", joined.data);
        free(joined.data);
    }
    APPEND_CMD_ARG("-c");
    APPEND_CMD_ARG(tu->resolvedSource ? tu->resolvedSource : tu->source);
    if (tu->resolvedObject) {
        APPEND_CMD_ARG2("-o", tu->resolvedObject);
    }
    if (!bg_append_char(&cmd, '\0')) goto fail;
    bool ok = bg_append_escaped(b, cmd.data);
    free(cmd.data);
    return ok;

fail:
    free(cmd.data);
    return false;
#undef APPEND_CMD_ARG2
#undef APPEND_CMD_ARG
}

static bool bg_file_exists(const char* path) {
    struct stat st;
    return path && stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

static bool bg_append_manifest_translation_units(BuildGraphJsonBuilder* b,
                                                 const FisicsBuildManifest* manifest) {
    BuildGraphDiagnosticSummary summary = bg_diagnostic_summary_unavailable(false, false);
    if (!bg_append_char(b, '[')) return false;
    for (size_t i = 0; i < manifest->translationUnitCount; ++i) {
        const FisicsBuildManifestTranslationUnit* tu = &manifest->translationUnits[i];
        if (i && !bg_append_char(b, ',')) return false;
        char id[64];
        snprintf(id, sizeof(id), "tu%zu", i);
        if (!bg_append_raw(b, "{\"id\":")) return false;
        if (!bg_append_escaped(b, id)) return false;
        if (!bg_append_raw(b, ",\"source\":")) return false;
        if (!bg_append_escaped(b, tu->resolvedSource ? tu->resolvedSource : tu->source)) return false;
        if (!bg_append_raw(b, ",\"object\":")) return false;
        if (!bg_append_escaped(b, tu->resolvedObject)) return false;
        if (!bg_append_raw(b, ",\"standard\":")) return false;
        if (!bg_append_escaped(b, bg_manifest_standard(manifest))) return false;
        if (!bg_append_raw(b, ",\"include_dirs\":")) return false;
        if (!bg_append_manifest_string_list(b, &manifest->defaults.includeDirs)) return false;
        if (!bg_append_raw(b, ",\"system_include_dirs\":[]")) return false;
        if (!bg_append_raw(b, ",\"defines\":")) return false;
        if (!bg_append_manifest_string_list(b, &manifest->defaults.defines)) return false;
        if (!bg_append_raw(b, ",\"forced_includes\":[]")) return false;
        if (!bg_append_raw(b, ",\"overlays\":")) return false;
        if (!bg_append_manifest_string_list(b, &manifest->defaults.overlays)) return false;
        if (!bg_append_raw(b, ",\"compile_args\":")) return false;
        if (!bg_append_manifest_compile_args(b, manifest, tu)) return false;
        if (!bg_append_raw(b, ",\"deps_json_path\":null,\"diagnostic_summary\":")) return false;
        if (!bg_append_diagnostic_summary(b, &summary)) return false;
        if (!bg_append_raw(b, ",\"status\":\"ok\"}")) return false;
    }
    return bg_append_char(b, ']');
}

static bool bg_append_manifest_compile_action(BuildGraphJsonBuilder* b,
                                              const FisicsBuildManifest* manifest,
                                              const FisicsBuildManifestTranslationUnit* tu,
                                              size_t index) {
    BuildGraphDiagnosticSummary summary = bg_diagnostic_summary_unavailable(false, false);
    char id[64];
    snprintf(id, sizeof(id), "compile%zu", index);
    if (!bg_append_raw(b, "{\"id\":")) return false;
    if (!bg_append_escaped(b, id)) return false;
    if (!bg_append_raw(b, ",\"kind\":\"compile\",\"status\":\"planned\",\"will_execute\":false")) return false;
    if (!bg_append_raw(b, ",\"source\":")) return false;
    if (!bg_append_escaped(b, tu->resolvedSource ? tu->resolvedSource : tu->source)) return false;
    if (!bg_append_raw(b, ",\"object\":")) return false;
    if (!bg_append_escaped(b, tu->resolvedObject)) return false;
    if (!bg_append_raw(b, ",\"input_exists\":")) return false;
    if (!bg_append_bool(b, bg_file_exists(tu->resolvedSource))) return false;
    if (!bg_append_raw(b, ",\"args\":")) return false;
    if (!bg_append_manifest_compile_args(b, manifest, tu)) return false;
    if (!bg_append_raw(b, ",\"diagnostic_summary\":")) return false;
    if (!bg_append_diagnostic_summary(b, &summary)) return false;
    return bg_append_char(b, '}');
}

static bool bg_append_manifest_link_action(BuildGraphJsonBuilder* b,
                                           const FisicsBuildManifest* manifest) {
    if (!manifest->link.output || !manifest->link.output[0]) return true;
    if (!bg_append_raw(b, ",{\"id\":\"link0\",\"kind\":\"link\",\"status\":\"planned\",\"will_execute\":false")) return false;
    if (!bg_append_raw(b, ",\"output\":")) return false;
    if (!bg_append_escaped(b, manifest->link.resolvedOutput ? manifest->link.resolvedOutput : manifest->link.output)) return false;
    if (!bg_append_raw(b, ",\"objects\":[")) return false;
    bool wrote = false;
    for (size_t i = 0; i < manifest->translationUnitCount; ++i) {
        const char* object = manifest->translationUnits[i].resolvedObject;
        if (!object) continue;
        if (wrote && !bg_append_char(b, ',')) return false;
        if (!bg_append_escaped(b, object)) return false;
        wrote = true;
    }
    if (!bg_append_raw(b, "],\"libraries\":")) return false;
    if (!bg_append_manifest_string_list(b, &manifest->link.libraries)) return false;
    if (!bg_append_raw(b, ",\"library_dirs\":")) return false;
    if (!bg_append_manifest_string_list(b, &manifest->link.libraryDirs)) return false;
    if (!bg_append_raw(b, ",\"args\":")) return false;
    if (!bg_append_manifest_string_list(b, &manifest->link.args)) return false;
    return bg_append_char(b, '}');
}

static bool bg_append_manifest_plan(BuildGraphJsonBuilder* b,
                                    const FisicsBuildManifest* manifest) {
    if (!bg_append_raw(b, "{\"schema\":\"fisiCs.build_plan\",\"version\":0,\"dry_run\":true,\"actions\":[")) return false;
    for (size_t i = 0; i < manifest->translationUnitCount; ++i) {
        if (i && !bg_append_char(b, ',')) return false;
        if (!bg_append_manifest_compile_action(b, manifest, &manifest->translationUnits[i], i)) return false;
    }
    if (!bg_append_manifest_link_action(b, manifest)) return false;
    if (!bg_append_raw(b, "]")) return false;
    return bg_append_char(b, '}');
}

static bool bg_append_include_edges(BuildGraphJsonBuilder* b, const IncludeGraph* graph) {
    if (!bg_append_char(b, '[')) return false;
    if (graph) {
        for (size_t i = 0; i < graph->count; ++i) {
            if (i && !bg_append_char(b, ',')) return false;
            if (!bg_append_raw(b, "{\"from\":")) return false;
            if (!bg_append_escaped(b, graph->edges[i].from)) return false;
            if (!bg_append_raw(b, ",\"include_text\":null,\"resolved_path\":")) return false;
            if (!bg_append_escaped(b, graph->edges[i].to)) return false;
            if (!bg_append_raw(b, ",\"kind\":\"unknown\",\"status\":\"resolved\",\"depth\":null}")) return false;
        }
    }
    return bg_append_char(b, ']');
}

static const char* bg_include_kind_name(FisicsIncludeKind kind) {
    switch (kind) {
        case FISICS_INCLUDE_LOCAL: return "quote";
        case FISICS_INCLUDE_SYSTEM: return "system";
    }
    return "unknown";
}

static bool bg_append_include_records(BuildGraphJsonBuilder* b, const CompilerContext* ctx) {
    size_t count = 0;
    const FisicsInclude* includes = cc_get_includes(ctx, &count);
    if (!bg_append_char(b, '[')) return false;
    for (size_t i = 0; i < count; ++i) {
        if (i && !bg_append_char(b, ',')) return false;
        if (!bg_append_raw(b, "{\"include_text\":")) return false;
        if (!bg_append_escaped(b, includes[i].name)) return false;
        if (!bg_append_raw(b, ",\"resolved_path\":")) return false;
        if (!bg_append_escaped(b, includes[i].resolved_path)) return false;
        if (!bg_append_raw(b, ",\"kind\":")) return false;
        if (!bg_append_escaped(b, bg_include_kind_name(includes[i].kind))) return false;
        if (!bg_append_raw(b, ",\"status\":\"")) return false;
        if (!bg_append_raw(b, includes[i].resolved ? "resolved" : "unresolved")) return false;
        if (!bg_append_raw(b, "\",\"line\":")) return false;
        char num[32];
        snprintf(num, sizeof(num), "%d", includes[i].line);
        if (!bg_append_raw(b, num)) return false;
        if (!bg_append_raw(b, ",\"column\":")) return false;
        snprintf(num, sizeof(num), "%d", includes[i].column);
        if (!bg_append_raw(b, num)) return false;
        if (!bg_append_char(b, '}')) return false;
    }
    return bg_append_char(b, ']');
}

char* fisics_build_graph_derive_object_path(const char* cPath) {
    if (!cPath) return NULL;
    size_t len = strlen(cPath);
    const char* dot = strrchr(cPath, '.');
    size_t baseLen = (dot && strcmp(dot, ".c") == 0) ? (size_t)(dot - cPath) : len;
    char* out = (char*)malloc(baseLen + 3u);
    if (!out) return NULL;
    memcpy(out, cPath, baseLen);
    out[baseLen] = '\0';
    strcat(out, ".o");
    return out;
}

bool fisics_build_graph_write_source_json(const FisicsBuildGraphSourceOptions* options,
                                          const CompilerContext* ctx) {
    if (!options || !options->outputPath || options->outputPath[0] == '\0') return false;
    BuildGraphJsonBuilder b = {0};
    char cwd[4096];
    const char* cwdText = getcwd(cwd, sizeof(cwd)) ? cwd : NULL;
    const IncludeGraph* graph = ctx ? cc_get_include_graph(ctx) : NULL;
    BuildGraphDiagnosticSummary summary =
        bg_diagnostic_summary_from_ctx(ctx, options->partial, options->fatal);
    bool graphPartial = options->partial || summary.errors > 0u;
    summary.partial = graphPartial;

    if (!bg_append_raw(&b, "{")) goto fail;
    if (!bg_append_raw(&b, "\"schema\":\"fisiCs.build_graph\"")) goto fail;
    if (!bg_append_raw(&b, ",\"version\":0")) goto fail;
    if (!bg_append_raw(&b, ",\"producer\":{\"name\":\"fisiCs\"}")) goto fail;
    if (!bg_append_raw(&b, ",\"cwd\":")) goto fail;
    if (!bg_append_escaped(&b, cwdText)) goto fail;
    if (!bg_append_raw(&b, ",\"project_root\":")) goto fail;
    if (!bg_append_escaped(&b, cwdText)) goto fail;
    if (!bg_append_raw(&b, ",\"mode\":\"source\"")) goto fail;
    if (!bg_append_raw(&b, ",\"partial\":")) goto fail;
    if (!bg_append_bool(&b, graphPartial)) goto fail;
    if (!bg_append_raw(&b, ",\"fatal\":")) goto fail;
    if (!bg_append_bool(&b, options->fatal)) goto fail;
    if (!bg_append_raw(&b, ",\"target\":{\"triple\":")) goto fail;
    if (!bg_append_escaped(&b, options->targetTriple)) goto fail;
    if (!bg_append_raw(&b, ",\"data_layout\":")) goto fail;
    if (!bg_append_escaped(&b, options->dataLayout)) goto fail;
    if (!bg_append_raw(&b, "}")) goto fail;
    if (!bg_append_raw(&b, ",\"diagnostic_summary\":")) goto fail;
    if (!bg_append_diagnostic_summary(&b, &summary)) goto fail;
    if (!bg_append_raw(&b, ",\"diagnostics\":[]")) goto fail;

    if (!bg_append_raw(&b, ",\"translation_units\":[{\"id\":\"tu0\",\"source\":")) goto fail;
    if (!bg_append_escaped(&b, options->inputPath)) goto fail;
    if (!bg_append_raw(&b, ",\"object\":")) goto fail;
    if (!bg_append_escaped(&b, options->outputObject)) goto fail;
    if (!bg_append_raw(&b, ",\"standard\":")) goto fail;
    if (!bg_append_escaped(&b, bg_dialect_name(options->dialect))) goto fail;
    if (!bg_append_raw(&b, ",\"include_dirs\":")) goto fail;
    if (!bg_append_string_array(&b, options->includePaths, options->includePathCount)) goto fail;
    if (!bg_append_raw(&b, ",\"system_include_dirs\":[]")) goto fail;
    if (!bg_append_raw(&b, ",\"defines\":")) goto fail;
    if (!bg_append_string_array(&b, options->macroDefines, options->macroDefineCount)) goto fail;
    if (!bg_append_raw(&b, ",\"forced_includes\":")) goto fail;
    if (!bg_append_string_array(&b, options->forcedIncludes, options->forcedIncludeCount)) goto fail;
    if (!bg_append_raw(&b, ",\"overlays\":")) goto fail;
    if (!bg_append_overlays(&b, options->overlayFeatures)) goto fail;
    if (!bg_append_raw(&b, ",\"compat\":")) goto fail;
    if (!bg_append_compat(&b, options->compatFeatures)) goto fail;
    if (!bg_append_raw(&b, ",\"compile_args\":")) goto fail;
    if (!bg_append_compile_args(&b, options)) goto fail;
    if (!bg_append_raw(&b, ",\"deps_json_path\":null")) goto fail;
    if (!bg_append_raw(&b, ",\"diagnostic_summary\":")) goto fail;
    if (!bg_append_diagnostic_summary(&b, &summary)) goto fail;
    if (!bg_append_raw(&b, ",\"status\":\"")) goto fail;
    if (!bg_append_raw(&b, options->fatal ? "fatal" : (graphPartial ? "partial" : "ok"))) goto fail;
    if (!bg_append_raw(&b, "\"}]")) goto fail;

    if (!bg_append_raw(&b, ",\"include_edges\":")) goto fail;
    if (!bg_append_include_edges(&b, graph)) goto fail;
    if (!bg_append_raw(&b, ",\"include_records\":")) goto fail;
    if (!bg_append_include_records(&b, ctx)) goto fail;
    if (!bg_append_raw(&b, ",\"link\":null")) goto fail;
    if (!bg_append_raw(&b, ",\"artifacts\":{\"build_graph\":")) goto fail;
    if (!bg_append_escaped(&b, options->outputPath)) goto fail;
    if (!bg_append_raw(&b, "}}\n")) goto fail;

    CoreResult wr = core_io_write_all(options->outputPath, b.data, b.len);
    free(b.data);
    return wr.code == CORE_OK;

fail:
    free(b.data);
    return false;
}

bool fisics_build_graph_write_manifest_dry_run_json(
    const FisicsBuildGraphManifestOptions* options) {
    if (!options || !options->manifest || !options->outputPath ||
        options->outputPath[0] == '\0') {
        return false;
    }
    const FisicsBuildManifest* manifest = options->manifest;
    BuildGraphJsonBuilder b = {0};
    char cwd[4096];
    const char* cwdText = getcwd(cwd, sizeof(cwd)) ? cwd : NULL;
    BuildGraphDiagnosticSummary summary =
        bg_diagnostic_summary_unavailable(options->partial, options->fatal);

    if (!bg_append_raw(&b, "{")) goto fail;
    if (!bg_append_raw(&b, "\"schema\":\"fisiCs.build_graph\"")) goto fail;
    if (!bg_append_raw(&b, ",\"version\":0")) goto fail;
    if (!bg_append_raw(&b, ",\"producer\":{\"name\":\"fisiCs\"}")) goto fail;
    if (!bg_append_raw(&b, ",\"cwd\":")) goto fail;
    if (!bg_append_escaped(&b, cwdText)) goto fail;
    if (!bg_append_raw(&b, ",\"project_root\":")) goto fail;
    if (!bg_append_escaped(&b, manifest->resolvedRoot)) goto fail;
    if (!bg_append_raw(&b, ",\"mode\":\"dry_run\"")) goto fail;
    if (!bg_append_raw(&b, ",\"partial\":")) goto fail;
    if (!bg_append_bool(&b, options->partial)) goto fail;
    if (!bg_append_raw(&b, ",\"fatal\":")) goto fail;
    if (!bg_append_bool(&b, options->fatal)) goto fail;
    if (!bg_append_raw(&b, ",\"target\":{\"triple\":null,\"data_layout\":null}")) goto fail;
    if (!bg_append_raw(&b, ",\"diagnostic_summary\":")) goto fail;
    if (!bg_append_diagnostic_summary(&b, &summary)) goto fail;
    if (!bg_append_raw(&b, ",\"diagnostics\":[]")) goto fail;
    if (!bg_append_raw(&b, ",\"manifest\":{\"schema\":")) goto fail;
    if (!bg_append_escaped(&b, manifest->schema)) goto fail;
    if (!bg_append_raw(&b, ",\"version\":0,\"name\":")) goto fail;
    if (!bg_append_escaped(&b, manifest->name)) goto fail;
    if (!bg_append_raw(&b, ",\"root\":")) goto fail;
    if (!bg_append_escaped(&b, manifest->resolvedRoot)) goto fail;
    if (!bg_append_raw(&b, ",\"build_dir\":")) goto fail;
    if (!bg_append_escaped(&b, manifest->buildDir)) goto fail;
    if (!bg_append_raw(&b, "}")) goto fail;
    if (!bg_append_raw(&b, ",\"translation_units\":")) goto fail;
    if (!bg_append_manifest_translation_units(&b, manifest)) goto fail;
    if (!bg_append_raw(&b, ",\"include_edges\":[],\"include_records\":[]")) goto fail;
    if (!bg_append_raw(&b, ",\"link\":")) goto fail;
    if (manifest->link.output && manifest->link.output[0]) {
        if (!bg_append_raw(&b, "{\"output\":")) goto fail;
        if (!bg_append_escaped(&b, manifest->link.resolvedOutput ? manifest->link.resolvedOutput : manifest->link.output)) goto fail;
        if (!bg_append_raw(&b, ",\"objects\":[")) goto fail;
        bool wroteObject = false;
        for (size_t i = 0; i < manifest->translationUnitCount; ++i) {
            const char* object = manifest->translationUnits[i].resolvedObject;
            if (!object) continue;
            if (wroteObject && !bg_append_char(&b, ',')) goto fail;
            if (!bg_append_escaped(&b, object)) goto fail;
            wroteObject = true;
        }
        if (!bg_append_raw(&b, "],\"libraries\":")) goto fail;
        if (!bg_append_manifest_string_list(&b, &manifest->link.libraries)) goto fail;
        if (!bg_append_raw(&b, ",\"library_dirs\":")) goto fail;
        if (!bg_append_manifest_string_list(&b, &manifest->link.libraryDirs)) goto fail;
        if (!bg_append_raw(&b, ",\"link_args\":")) goto fail;
        if (!bg_append_manifest_string_list(&b, &manifest->link.args)) goto fail;
        if (!bg_append_raw(&b, ",\"runtime_archives\":[],\"status\":\"planned\"}")) goto fail;
    } else {
        if (!bg_append_raw(&b, "null")) goto fail;
    }
    if (!bg_append_raw(&b, ",\"plan\":")) goto fail;
    if (!bg_append_manifest_plan(&b, manifest)) goto fail;
    if (!bg_append_raw(&b, ",\"artifacts\":{\"build_graph\":")) goto fail;
    if (!bg_append_escaped(&b, options->outputPath)) goto fail;
    if (!bg_append_raw(&b, "}}\n")) goto fail;

    bool ok = false;
    if (strcmp(options->outputPath, "-") == 0) {
        ok = fwrite(b.data, 1u, b.len, stdout) == b.len;
    } else {
        CoreResult wr = core_io_write_all(options->outputPath, b.data, b.len);
        ok = wr.code == CORE_OK;
    }
    free(b.data);
    return ok;

fail:
    free(b.data);
    return false;
}

bool fisics_build_graph_write_compile_commands_json(
    const char* outputPath,
    const FisicsBuildManifest* manifest) {
    if (!outputPath || !outputPath[0] || !manifest) return false;
    BuildGraphJsonBuilder b = {0};
    if (!bg_append_char(&b, '[')) goto fail;
    for (size_t i = 0; i < manifest->translationUnitCount; ++i) {
        const FisicsBuildManifestTranslationUnit* tu = &manifest->translationUnits[i];
        if (i && !bg_append_char(&b, ',')) goto fail;
        if (!bg_append_raw(&b, "{\"directory\":")) goto fail;
        if (!bg_append_escaped(&b, manifest->resolvedRoot)) goto fail;
        if (!bg_append_raw(&b, ",\"file\":")) goto fail;
        if (!bg_append_escaped(&b, tu->resolvedSource ? tu->resolvedSource : tu->source)) goto fail;
        if (!bg_append_raw(&b, ",\"output\":")) goto fail;
        if (!bg_append_escaped(&b, tu->resolvedObject)) goto fail;
        if (!bg_append_raw(&b, ",\"arguments\":")) goto fail;
        if (!bg_append_manifest_compile_args(&b, manifest, tu)) goto fail;
        if (!bg_append_raw(&b, ",\"command\":")) goto fail;
        if (!bg_append_manifest_compile_command(&b, manifest, tu)) goto fail;
        if (!bg_append_char(&b, '}')) goto fail;
    }
    if (!bg_append_raw(&b, "]\n")) goto fail;

    bool ok = false;
    if (strcmp(outputPath, "-") == 0) {
        ok = fwrite(b.data, 1u, b.len, stdout) == b.len;
    } else {
        CoreResult wr = core_io_write_all(outputPath, b.data, b.len);
        ok = wr.code == CORE_OK;
    }
    free(b.data);
    return ok;

fail:
    free(b.data);
    return false;
}
