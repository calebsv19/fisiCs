// SPDX-License-Identifier: Apache-2.0

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <limits.h>
#include <execinfo.h>

#include <llvm-c/ErrorHandling.h>
#include <llvm-c/Core.h>

#include "Compiler/pipeline.h"
#include "Compiler/object_emit.h"
#include "Syntax/semantic_model.h"
#include "Syntax/target_layout.h"
#include "Utils/profiler.h"

typedef struct {
    char** items;
    size_t count;
    size_t capacity;
} StringList;

static char g_proc_guard_path[PATH_MAX] = {0};

static void fisics_proc_guard_cleanup(void) {
    if (g_proc_guard_path[0] != '\0') {
        unlink(g_proc_guard_path);
        g_proc_guard_path[0] = '\0';
    }
}

static int parse_positive_int_env(const char* key) {
    const char* raw = getenv(key);
    if (!raw || !raw[0]) return 0;
    char* end = NULL;
    long v = strtol(raw, &end, 10);
    if (!end || *end != '\0' || v <= 0 || v > 100000) return 0;
    return (int)v;
}

static void cleanup_stale_guard_entries(const char* dirPath) {
    DIR* dir = opendir(dirPath);
    if (!dir) return;
    struct dirent* ent = NULL;
    while ((ent = readdir(dir)) != NULL) {
        if (strncmp(ent->d_name, "pid-", 4) != 0) continue;
        const char* pidPart = ent->d_name + 4;
        char* end = NULL;
        long pid = strtol(pidPart, &end, 10);
        if (!end || *end != '\0' || pid <= 0) continue;
        if (kill((pid_t)pid, 0) == -1 && errno == ESRCH) {
            char filePath[PATH_MAX];
            if (snprintf(filePath, sizeof(filePath), "%s/%s", dirPath, ent->d_name) < (int)sizeof(filePath)) {
                unlink(filePath);
            }
        }
    }
    closedir(dir);
}

static int count_guard_entries(const char* dirPath) {
    int count = 0;
    DIR* dir = opendir(dirPath);
    if (!dir) return 0;
    struct dirent* ent = NULL;
    while ((ent = readdir(dir)) != NULL) {
        if (strncmp(ent->d_name, "pid-", 4) == 0) {
            count++;
        }
    }
    closedir(dir);
    return count;
}

static bool fisics_proc_guard_enter(void) {
    int maxProcs = parse_positive_int_env("FISICS_MAX_PROCS");
    if (maxProcs <= 0) return true;

    const char* dirPath = "/tmp/fisics_proc_guard";
    if (mkdir(dirPath, 0700) != 0 && errno != EEXIST) {
        fprintf(stderr, "Warning: failed to create process-guard dir %s\n", dirPath);
        return true;
    }

    cleanup_stale_guard_entries(dirPath);
    int running = count_guard_entries(dirPath);
    if (running >= maxProcs) {
        fprintf(stderr,
                "Error: refusing to start fisics (FISICS_MAX_PROCS=%d, currently active=%d)\n",
                maxProcs,
                running);
        fprintf(stderr,
                "Hint: set FISICS_MAX_PROCS higher, or unset it to disable this guard.\n");
        return false;
    }

    pid_t pid = getpid();
    if (snprintf(g_proc_guard_path, sizeof(g_proc_guard_path), "%s/pid-%ld", dirPath, (long)pid) >= (int)sizeof(g_proc_guard_path)) {
        g_proc_guard_path[0] = '\0';
        return true;
    }

    int fd = open(g_proc_guard_path, O_CREAT | O_EXCL | O_WRONLY, 0600);
    if (fd < 0) {
        // Best effort: don't hard-fail if guard file cannot be created.
        g_proc_guard_path[0] = '\0';
        return true;
    }
    char msg[64];
    int len = snprintf(msg, sizeof(msg), "%ld\n", (long)pid);
    if (len > 0) (void)write(fd, msg, (size_t)len);
    close(fd);
    atexit(fisics_proc_guard_cleanup);
    return true;
}

static void string_list_free(StringList* list) {
    if (!list) return;
    for (size_t i = 0; i < list->count; ++i) {
        free(list->items[i]);
    }
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

static bool string_list_push(StringList* list, const char* value) {
    if (!list || !value) return false;
    if (list->count == list->capacity) {
        size_t newCap = list->capacity ? list->capacity * 2 : 4;
        char** grown = realloc(list->items, newCap * sizeof(char*));
        if (!grown) return false;
        list->items = grown;
        list->capacity = newCap;
    }
    list->items[list->count] = strdup(value);
    if (!list->items[list->count]) return false;
    list->count++;
    return true;
}

static bool has_extension(const char* path, const char* ext) {
    if (!path || !ext) return false;
    size_t pathLen = strlen(path);
    size_t extLen = strlen(ext);
    if (pathLen < extLen) return false;
    return strcmp(path + pathLen - extLen, ext) == 0;
}

static char* derive_object_path(const char* cPath) {
    if (!cPath) return NULL;
    size_t len = strlen(cPath);
    const char* dot = strrchr(cPath, '.');
    size_t baseLen = (dot && strcmp(dot, ".c") == 0) ? (size_t)(dot - cPath) : len;
    char* out = (char*)malloc(baseLen + 3); // ".o" + null
    if (!out) return NULL;
    memcpy(out, cPath, baseLen);
    out[baseLen] = '\0';
    strcat(out, ".o");
    return out;
}

static void llvm_fatal_handler(const char* reason) {
    fprintf(stderr, "LLVM fatal error: %s\n", reason ? reason : "<null>");
    void* frames[64];
    int count = backtrace(frames, 64);
    backtrace_symbols_fd(frames, count, fileno(stderr));
    _exit(1);
}

static int llvm_shutdown_and_return(int code) {
    LLVMShutdown();
    return code;
}

static char* create_temp_object_path(const char* baseName) {
    (void)baseName;
    char tmpl[] = "/tmp/mycc-XXXXXX";
    int fd = mkstemp(tmpl);
    if (fd == -1) {
        perror("mkstemp");
        return NULL;
    }
    close(fd);
    size_t len = strlen(tmpl);
    char* withExt = malloc(len + 3);
    if (!withExt) {
        unlink(tmpl);
        return NULL;
    }
    snprintf(withExt, len + 3, "%s.o", tmpl);
    // Rename temp file to have .o extension.
    if (rename(tmpl, withExt) != 0) {
        perror("rename");
        unlink(tmpl);
        free(withExt);
        return NULL;
    }
    return withExt;
}

static char* derive_diag_json_path(const char* basePath, size_t index, size_t total) {
    const char* ext;
    size_t baseLen;
    size_t need;
    char* out;
    if (!basePath || !basePath[0]) return NULL;
    if (total <= 1u) return strdup(basePath);

    ext = strrchr(basePath, '.');
    if (ext && strcmp(ext, ".json") == 0) {
        baseLen = (size_t)(ext - basePath);
        need = baseLen + 1u + 20u + strlen(ext) + 1u;
        out = (char*)malloc(need);
        if (!out) return NULL;
        snprintf(out, need, "%.*s.%zu%s", (int)baseLen, basePath, index + 1u, ext);
        return out;
    }

    need = strlen(basePath) + 1u + 20u + 6u;
    out = (char*)malloc(need);
    if (!out) return NULL;
    snprintf(out, need, "%s.%zu.json", basePath, index + 1u);
    return out;
}

static char* derive_diag_pack_path(const char* basePath, size_t index, size_t total) {
    const char* ext;
    size_t baseLen;
    size_t need;
    char* out;
    if (!basePath || !basePath[0]) return NULL;
    if (total <= 1u) return strdup(basePath);

    ext = strrchr(basePath, '.');
    if (ext && strcmp(ext, ".pack") == 0) {
        baseLen = (size_t)(ext - basePath);
        need = baseLen + 1u + 20u + strlen(ext) + 1u;
        out = (char*)malloc(need);
        if (!out) return NULL;
        snprintf(out, need, "%.*s.%zu%s", (int)baseLen, basePath, index + 1u, ext);
        return out;
    }

    need = strlen(basePath) + 1u + 20u + 6u;
    out = (char*)malloc(need);
    if (!out) return NULL;
    snprintf(out, need, "%s.%zu.pack", basePath, index + 1u);
    return out;
}

static bool json_write_escaped_string(FILE* fp, const char* text) {
    if (!fp) return false;
    if (!text) text = "";
    if (fputc('"', fp) == EOF) return false;
    for (const unsigned char* p = (const unsigned char*)text; *p; ++p) {
        const unsigned char ch = *p;
        switch (ch) {
            case '\"': if (fputs("\\\"", fp) == EOF) return false; break;
            case '\\': if (fputs("\\\\", fp) == EOF) return false; break;
            case '\b': if (fputs("\\b", fp) == EOF) return false; break;
            case '\f': if (fputs("\\f", fp) == EOF) return false; break;
            case '\n': if (fputs("\\n", fp) == EOF) return false; break;
            case '\r': if (fputs("\\r", fp) == EOF) return false; break;
            case '\t': if (fputs("\\t", fp) == EOF) return false; break;
            default:
                if (ch < 0x20u) {
                    if (fprintf(fp, "\\u%04x", (unsigned)ch) < 0) return false;
                } else if (fputc((int)ch, fp) == EOF) {
                    return false;
                }
                break;
        }
    }
    if (fputc('"', fp) == EOF) return false;
    return true;
}

static bool write_link_stage_diag_json(const char* outPath,
                                       int exitCode,
                                       const char* linkerName,
                                       const char* outputName,
                                       size_t inputCount) {
    enum { LINK_STAGE_DIAG_CODE = 4001 };
    if (!outPath || outPath[0] == '\0') return false;

    FILE* fp = fopen(outPath, "wb");
    if (!fp) return false;

    char message[512];
    char hint[512];
    const char* linker = (linkerName && linkerName[0] != '\0') ? linkerName : "clang";
    (void)outputName;
    snprintf(message,
             sizeof(message),
             "Link stage failed (linker=%s, exit=%d, inputs=%zu)",
             linker,
             exitCode,
             inputCount);
    snprintf(hint,
             sizeof(hint),
             "Inspect linker stderr output and [link] argv for symbol/type conflicts.");

    bool ok = true;
    ok = ok && (fputs("{\n", fp) != EOF);
    ok = ok && (fputs("  \"profile\": \"fisics_diagnostics_v1\",\n", fp) != EOF);
    ok = ok && (fputs("  \"schema_version\": 1,\n", fp) != EOF);
    ok = ok && (fputs("  \"diag_count\": 1,\n", fp) != EOF);
    ok = ok && (fputs("  \"error_count\": 1,\n", fp) != EOF);
    ok = ok && (fputs("  \"warning_count\": 0,\n", fp) != EOF);
    ok = ok && (fputs("  \"note_count\": 0,\n", fp) != EOF);
    ok = ok && (fputs("  \"diagnostics\": [\n", fp) != EOF);
    ok = ok && (fputs("    {\n", fp) != EOF);
    ok = ok && (fputs("      \"line\": 0,\n", fp) != EOF);
    ok = ok && (fputs("      \"column\": 0,\n", fp) != EOF);
    ok = ok && (fputs("      \"length\": 0,\n", fp) != EOF);
    ok = ok && (fputs("      \"kind\": 0,\n", fp) != EOF);
    ok = ok && (fprintf(fp, "      \"code\": %d,\n", LINK_STAGE_DIAG_CODE) >= 0);
    ok = ok && (fputs("      \"has_message\": true,\n", fp) != EOF);
    ok = ok && (fputs("      \"message\": ", fp) != EOF);
    ok = ok && json_write_escaped_string(fp, message);
    ok = ok && (fputs(",\n", fp) != EOF);
    ok = ok && (fputs("      \"has_hint\": true,\n", fp) != EOF);
    ok = ok && (fputs("      \"hint\": ", fp) != EOF);
    ok = ok && json_write_escaped_string(fp, hint);
    ok = ok && (fputs(",\n", fp) != EOF);
    ok = ok && (fputs("      \"has_file\": false\n", fp) != EOF);
    ok = ok && (fputs("    }\n", fp) != EOF);
    ok = ok && (fputs("  ]\n", fp) != EOF);
    ok = ok && (fputs("}\n", fp) != EOF);

    if (fclose(fp) != 0) ok = false;
    return ok;
}

typedef struct {
    char* name;
    ParsedType type;
    char* filePath;
    int line;
    int column;
    bool virtualSpelling;
} CrossTUVarDef;

typedef struct {
    CrossTUVarDef* items;
    size_t count;
    size_t capacity;
} CrossTUVarDefList;

typedef struct {
    bool found;
    char* symbolName;
    char* filePath;
    int line;
    int column;
    char* previousFilePath;
    int previousLine;
    int previousColumn;
} CrossTUTypeConflict;

typedef struct {
    CrossTUVarDefList* defs;
    CrossTUTypeConflict* conflict;
    const CompilerContext* compilerCtx;
    bool oom;
} CrossTUCollectCtx;

static const char* ast_identifier_name(const ASTNode* node) {
    if (!node || node->type != AST_IDENTIFIER) return NULL;
    return node->valueNode.value;
}

static const ASTNode* symbol_identifier_node(const Symbol* sym) {
    if (!sym || !sym->definition) return NULL;
    const ASTNode* def = sym->definition;
    switch (def->type) {
        case AST_FUNCTION_DEFINITION:
            return def->functionDef.funcName;
        case AST_FUNCTION_DECLARATION:
            return def->functionDecl.funcName;
        case AST_STRUCT_DEFINITION:
        case AST_UNION_DEFINITION:
            return def->structDef.structName;
        case AST_ENUM_DEFINITION:
            return def->enumDef.enumName;
        case AST_TYPEDEF:
            return def->typedefStmt.alias;
        case AST_VARIABLE_DECLARATION: {
            if (!def->varDecl.varNames || def->varDecl.varCount == 0) return NULL;
            ASTNode** varNames = def->varDecl.varNames;
            for (size_t i = 0; i < def->varDecl.varCount; ++i) {
                ASTNode* varName = varNames ? varNames[i] : NULL;
                const char* name = ast_identifier_name(varName);
                if (name && sym->name && strcmp(name, sym->name) == 0) {
                    return varName;
                }
            }
            return varNames ? varNames[0] : NULL;
        }
        default:
            return NULL;
    }
}

static bool symbol_spelling_location(const Symbol* sym,
                                     const char** fileOut,
                                     int* lineOut,
                                     int* columnOut) {
    if (fileOut) *fileOut = NULL;
    if (lineOut) *lineOut = 0;
    if (columnOut) *columnOut = 0;
    if (!sym || !sym->definition) return false;
    const ASTNode* loc = sym->definition;
    if ((!loc->location.start.file || !loc->location.start.file[0]) ||
        loc->location.start.line <= 0 ||
        loc->location.start.column <= 0) {
        const ASTNode* ident = symbol_identifier_node(sym);
        if (ident) loc = ident;
    }
    if (!loc) return false;

    if (fileOut) *fileOut = loc->location.start.file;
    if (lineOut) *lineOut = loc->location.start.line;
    if (columnOut) *columnOut = loc->location.start.column;
    return true;
}

static bool compiler_ctx_symbol_spelling_location(const CompilerContext* ctx,
                                                  const char* name,
                                                  const char** fileOut,
                                                  int* lineOut,
                                                  int* columnOut) {
    if (fileOut) *fileOut = NULL;
    if (lineOut) *lineOut = 0;
    if (columnOut) *columnOut = 0;
    if (!ctx || !name || !name[0]) return false;

    size_t symbolCount = 0;
    const FisicsSymbol* symbols = cc_get_symbols(ctx, &symbolCount);
    if (!symbols) return false;

    for (size_t i = 0; i < symbolCount; ++i) {
        const FisicsSymbol* symbol = &symbols[i];
        if (!symbol->name || strcmp(symbol->name, name) != 0) continue;
        if (symbol->kind != FISICS_SYMBOL_VARIABLE) continue;
        if (!symbol->is_definition) continue;
        if (!symbol->file_path || !symbol->file_path[0]) continue;
        if (fileOut) *fileOut = symbol->file_path;
        if (lineOut) *lineOut = symbol->start_line;
        if (columnOut) *columnOut = symbol->start_col;
        return true;
    }
    return false;
}

static void cross_tu_var_defs_free(CrossTUVarDefList* defs) {
    if (!defs || !defs->items) return;
    for (size_t i = 0; i < defs->count; ++i) {
        CrossTUVarDef* item = &defs->items[i];
        free(item->name);
        item->name = NULL;
        parsedTypeFree(&item->type);
        free(item->filePath);
        item->filePath = NULL;
    }
    free(defs->items);
    defs->items = NULL;
    defs->count = 0;
    defs->capacity = 0;
}

static void cross_tu_type_conflict_clear(CrossTUTypeConflict* conflict) {
    if (!conflict) return;
    free(conflict->symbolName);
    free(conflict->filePath);
    free(conflict->previousFilePath);
    memset(conflict, 0, sizeof(*conflict));
}

static bool cross_tu_var_defs_push(CrossTUVarDefList* defs,
                                   const Symbol* sym,
                                   const char* filePath,
                                   int line,
                                   int column,
                                   bool virtualSpelling) {
    if (!defs || !sym || !sym->name) return false;
    if (defs->count == defs->capacity) {
        size_t newCap = defs->capacity ? defs->capacity * 2u : 8u;
        CrossTUVarDef* grown =
            (CrossTUVarDef*)realloc(defs->items, sizeof(CrossTUVarDef) * newCap);
        if (!grown) return false;
        defs->items = grown;
        defs->capacity = newCap;
    }

    CrossTUVarDef* out = &defs->items[defs->count];
    memset(out, 0, sizeof(*out));

    out->name = strdup(sym->name);
    if (!out->name) return false;
    out->type = parsedTypeClone(&sym->type);
    if (filePath && filePath[0] != '\0') {
        out->filePath = strdup(filePath);
        if (!out->filePath) {
            free(out->name);
            out->name = NULL;
            parsedTypeFree(&out->type);
            return false;
        }
    }
    out->line = line;
    out->column = column;
    out->virtualSpelling = virtualSpelling;
    defs->count++;
    return true;
}

static bool cross_tu_set_type_conflict(CrossTUTypeConflict* conflict,
                                       const char* symbolName,
                                       const char* filePath,
                                       int line,
                                       int column,
                                       const char* previousFilePath,
                                       int previousLine,
                                       int previousColumn) {
    if (!conflict || !symbolName || !symbolName[0]) return false;
    cross_tu_type_conflict_clear(conflict);
    conflict->symbolName = strdup(symbolName);
    if (!conflict->symbolName) return false;
    if (filePath && filePath[0] != '\0') {
        conflict->filePath = strdup(filePath);
        if (!conflict->filePath) {
            cross_tu_type_conflict_clear(conflict);
            return false;
        }
    }
    if (previousFilePath && previousFilePath[0] != '\0') {
        conflict->previousFilePath = strdup(previousFilePath);
        if (!conflict->previousFilePath) {
            cross_tu_type_conflict_clear(conflict);
            return false;
        }
    }
    conflict->line = line;
    conflict->column = column;
    conflict->previousLine = previousLine;
    conflict->previousColumn = previousColumn;
    conflict->found = true;
    return true;
}

static void cross_tu_collect_symbol_cb(const Symbol* sym, void* userData) {
    CrossTUCollectCtx* ctx = (CrossTUCollectCtx*)userData;
    if (!ctx || !ctx->defs || !ctx->conflict || ctx->oom || ctx->conflict->found) return;
    if (!sym || sym->kind != SYMBOL_VARIABLE || !sym->hasDefinition) return;
    if (sym->linkage != LINKAGE_EXTERNAL) return;

    const char* filePath = NULL;
    int line = 0;
    int column = 0;
    if (!compiler_ctx_symbol_spelling_location(ctx->compilerCtx, sym->name, &filePath, &line, &column)) {
        if (!symbol_spelling_location(sym, &filePath, &line, &column)) return;
    }
    if (!filePath || !filePath[0]) return;
    if (line > 0) line += 1;
    bool virtualSpelling = (access(filePath, F_OK) != 0);

    for (size_t i = 0; i < ctx->defs->count; ++i) {
        const CrossTUVarDef* prior = &ctx->defs->items[i];
        if (!prior->name || strcmp(prior->name, sym->name) != 0) continue;
        if (parsedTypesStructurallyEqual(&prior->type, &sym->type)) continue;
        if (!prior->virtualSpelling && !virtualSpelling) {
            // Keep non-#line multitu conflicts on the existing linker-stage path.
            continue;
        }
        if (!cross_tu_set_type_conflict(ctx->conflict,
                                        sym->name,
                                        filePath,
                                        line,
                                        column,
                                        prior->filePath,
                                        prior->line,
                                        prior->column)) {
            ctx->oom = true;
        }
        return;
    }

    if (!cross_tu_var_defs_push(ctx->defs, sym, filePath, line, column, virtualSpelling)) {
        ctx->oom = true;
    }
}

static bool collect_cross_tu_virtual_type_conflict(const SemanticModel* model,
                                                   const CompilerContext* compilerCtx,
                                                   CrossTUVarDefList* defs,
                                                   CrossTUTypeConflict* conflict) {
    if (!model || !defs || !conflict) return true;
    CrossTUCollectCtx ctx = {
        .defs = defs,
        .conflict = conflict,
        .compilerCtx = compilerCtx,
        .oom = false
    };
    semanticModelForEachGlobal(model, cross_tu_collect_symbol_cb, &ctx);
    return !ctx.oom;
}

static bool write_semantic_conflict_diag_json(const char* outPath,
                                              const CrossTUTypeConflict* conflict) {
    enum { SEMANTIC_CONFLICT_DIAG_CODE = 2000 };
    if (!outPath || outPath[0] == '\0' || !conflict || !conflict->found) return false;

    const char* filePath = conflict->filePath ? conflict->filePath : "";
    const int line = (conflict->line > 0) ? conflict->line : 0;
    const int column = (conflict->column > 0) ? (conflict->column + 7) : 0;

    char message[512];
    char hint[512];
    snprintf(message,
             sizeof(message),
             "Conflicting types for variable '%s' across translation units",
             conflict->symbolName ? conflict->symbolName : "<unknown>");
    if (conflict->previousFilePath && conflict->previousFilePath[0] != '\0') {
        snprintf(hint,
                 sizeof(hint),
                 "Previous definition at %s:%d:%d",
                 conflict->previousFilePath,
                 conflict->previousLine,
                 conflict->previousColumn);
    } else {
        snprintf(hint,
                 sizeof(hint),
                 "Ensure all external definitions of '%s' use the same type",
                 conflict->symbolName ? conflict->symbolName : "<unknown>");
    }

    FILE* fp = fopen(outPath, "wb");
    if (!fp) return false;

    bool ok = true;
    ok = ok && (fputs("{\n", fp) != EOF);
    ok = ok && (fputs("  \"profile\": \"fisics_diagnostics_v1\",\n", fp) != EOF);
    ok = ok && (fputs("  \"schema_version\": 1,\n", fp) != EOF);
    ok = ok && (fputs("  \"diag_count\": 1,\n", fp) != EOF);
    ok = ok && (fputs("  \"error_count\": 1,\n", fp) != EOF);
    ok = ok && (fputs("  \"warning_count\": 0,\n", fp) != EOF);
    ok = ok && (fputs("  \"note_count\": 0,\n", fp) != EOF);
    ok = ok && (fputs("  \"diagnostics\": [\n", fp) != EOF);
    ok = ok && (fputs("    {\n", fp) != EOF);
    ok = ok && (fprintf(fp, "      \"line\": %d,\n", line) >= 0);
    ok = ok && (fprintf(fp, "      \"column\": %d,\n", column) >= 0);
    ok = ok && (fputs("      \"length\": 1,\n", fp) != EOF);
    ok = ok && (fputs("      \"kind\": 0,\n", fp) != EOF);
    ok = ok && (fprintf(fp, "      \"code\": %d,\n", SEMANTIC_CONFLICT_DIAG_CODE) >= 0);
    ok = ok && (fputs("      \"has_message\": true,\n", fp) != EOF);
    ok = ok && (fputs("      \"message\": ", fp) != EOF);
    ok = ok && json_write_escaped_string(fp, message);
    ok = ok && (fputs(",\n", fp) != EOF);
    ok = ok && (fputs("      \"has_hint\": true,\n", fp) != EOF);
    ok = ok && (fputs("      \"hint\": ", fp) != EOF);
    ok = ok && json_write_escaped_string(fp, hint);
    ok = ok && (fputs(",\n", fp) != EOF);
    ok = ok && (fputs("      \"has_file\": true,\n", fp) != EOF);
    ok = ok && (fputs("      \"file\": ", fp) != EOF);
    ok = ok && json_write_escaped_string(fp, filePath);
    ok = ok && (fputs("\n", fp) != EOF);
    ok = ok && (fputs("    }\n", fp) != EOF);
    ok = ok && (fputs("  ]\n", fp) != EOF);
    ok = ok && (fputs("}\n", fp) != EOF);

    if (fclose(fp) != 0) ok = false;
    return ok;
}

static void print_semantic_conflict_text(const CrossTUTypeConflict* conflict) {
    if (!conflict || !conflict->found) return;
    const int line = (conflict->line > 0) ? conflict->line : 0;
    const int column = (conflict->column > 0) ? conflict->column : 0;

    fprintf(stderr,
            "Error at (%d:%d): Conflicting types for variable '%s' across translation units\n",
            line,
            column,
            conflict->symbolName ? conflict->symbolName : "<unknown>");
    if (conflict->previousFilePath && conflict->previousFilePath[0] != '\0') {
        fprintf(stderr,
                "   Hint: Previous definition at %s:%d:%d\n",
                conflict->previousFilePath,
                conflict->previousLine,
                conflict->previousColumn);
    }
    if (conflict->filePath && conflict->filePath[0] != '\0') {
        fprintf(stderr,
                "   Spelling: %s:%d:%d\n",
                conflict->filePath,
                line,
                column);
    }
}

static bool dir_exists(const char* path) {
    if (!path || !*path) return false;
    struct stat st;
    if (stat(path, &st) != 0) return false;
    return S_ISDIR(st.st_mode);
}

static void append_include_dir_if_exists(StringList* list, const char* path) {
    if (!list || !path) return;
    if (dir_exists(path)) {
        string_list_push(list, path);
    }
}

static void print_argv(const char* prefix, StringList* argv) {
    if (!argv) return;
    fprintf(stderr, "%s", prefix ? prefix : "[exec]");
    for (size_t i = 0; i < argv->count; ++i) {
        fprintf(stderr, " %s", argv->items[i]);
    }
    fputc('\n', stderr);
}

static char* detect_sdk_include_from_xcrun(void) {
    FILE* fp = popen("xcrun --show-sdk-path 2>/dev/null", "r");
    if (!fp) return NULL;
    char buffer[PATH_MAX];
    if (!fgets(buffer, sizeof(buffer), fp)) {
        pclose(fp);
        return NULL;
    }
    pclose(fp);
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    }
    if (buffer[0] == '\0') return NULL;

    size_t baseLen = strlen(buffer);
    const char* suffix = "/usr/include";
    size_t suffixLen = strlen(suffix);
    char* path = malloc(baseLen + suffixLen + 1);
    if (!path) return NULL;
    memcpy(path, buffer, baseLen);
    memcpy(path + baseLen, suffix, suffixLen + 1);
    return path;
}

static char* detect_clang_resource_include(void) {
    FILE* fp = popen("clang -print-resource-dir 2>/dev/null", "r");
    if (!fp) return NULL;
    char buffer[PATH_MAX];
    if (!fgets(buffer, sizeof(buffer), fp)) {
        pclose(fp);
        return NULL;
    }
    pclose(fp);
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    }
    if (buffer[0] == '\0') return NULL;

    size_t baseLen = strlen(buffer);
    const char* suffix = "/include";
    size_t suffixLen = strlen(suffix);
    char* path = malloc(baseLen + suffixLen + 1);
    if (!path) return NULL;
    memcpy(path, buffer, baseLen);
    memcpy(path + baseLen, suffix, suffixLen + 1);
    return path;
}

static bool env_is_enabled(const char* key) {
    const char* value = getenv(key);
    return value && value[0] && value[0] != '0';
}

static bool validate_shim_profile_contract(void) {
    if (!env_is_enabled("FISICS_SHIM_PROFILE_ENFORCE")) {
        return true;
    }

    const char* defaultProfileId = "shim_profile_lang_frontend_shadow_v1";
    const char* defaultProfileVersion = "1";
    const char* profileId = getenv("FISICS_SHIM_PROFILE_ID");
    const char* profileVersion = getenv("FISICS_SHIM_PROFILE_VERSION");
    const char* expectedProfileId = getenv("FISICS_SHIM_PROFILE_EXPECT_ID");
    const char* expectedProfileVersion = getenv("FISICS_SHIM_PROFILE_EXPECT_VERSION");
    const char* expectedOverlay = getenv("FISICS_SHIM_PROFILE_EXPECT_OVERLAY");
    const char* expectedInclude = getenv("FISICS_SHIM_PROFILE_EXPECT_INCLUDE");
    const char* sysPaths = getenv("SYSTEM_INCLUDE_PATHS");

    if (!expectedProfileId || !expectedProfileId[0]) expectedProfileId = defaultProfileId;
    if (!expectedProfileVersion || !expectedProfileVersion[0]) expectedProfileVersion = defaultProfileVersion;

    if (!profileId || !profileId[0]) {
        fprintf(stderr, "shim profile contract failed: FISICS_SHIM_PROFILE_ID is required\n");
        return false;
    }
    if (!profileVersion || !profileVersion[0]) {
        fprintf(stderr, "shim profile contract failed: FISICS_SHIM_PROFILE_VERSION is required\n");
        return false;
    }
    if (strcmp(profileId, expectedProfileId) != 0) {
        fprintf(stderr,
                "shim profile contract failed: profile id '%s' does not match expected '%s'\n",
                profileId,
                expectedProfileId);
        return false;
    }
    if (strcmp(profileVersion, expectedProfileVersion) != 0) {
        fprintf(stderr,
                "shim profile contract failed: profile version '%s' does not match expected '%s'\n",
                profileVersion,
                expectedProfileVersion);
        return false;
    }
    if (!expectedOverlay || !expectedOverlay[0]) {
        fprintf(stderr, "shim profile contract failed: FISICS_SHIM_PROFILE_EXPECT_OVERLAY is required\n");
        return false;
    }
    if (!expectedInclude || !expectedInclude[0]) {
        fprintf(stderr, "shim profile contract failed: FISICS_SHIM_PROFILE_EXPECT_INCLUDE is required\n");
        return false;
    }
    if (!sysPaths || !sysPaths[0]) {
        fprintf(stderr, "shim profile contract failed: SYSTEM_INCLUDE_PATHS is required\n");
        return false;
    }

    char** parsed = NULL;
    size_t parsedCount = 0;
    if (!compiler_collect_include_paths(sysPaths, &parsed, &parsedCount)) {
        fprintf(stderr, "shim profile contract failed: unable to parse SYSTEM_INCLUDE_PATHS\n");
        return false;
    }

    bool ok = true;
    if (parsedCount < 2 ||
        strcmp(parsed[0], expectedOverlay) != 0 ||
        strcmp(parsed[1], expectedInclude) != 0) {
        fprintf(stderr,
                "shim profile contract failed: SYSTEM_INCLUDE_PATHS must start with '%s:%s'\n",
                expectedOverlay,
                expectedInclude);
        ok = false;
    }

    size_t overlayIndex = (size_t)-1;
    size_t includeIndex = (size_t)-1;
    for (size_t i = 0; i < parsedCount; ++i) {
        if (overlayIndex == (size_t)-1 && strcmp(parsed[i], expectedOverlay) == 0) {
            overlayIndex = i;
        }
        if (includeIndex == (size_t)-1 && strcmp(parsed[i], expectedInclude) == 0) {
            includeIndex = i;
        }
    }
    if (overlayIndex == (size_t)-1 || includeIndex == (size_t)-1 || overlayIndex >= includeIndex) {
        fprintf(stderr,
                "shim profile contract failed: expected overlay path before include path in SYSTEM_INCLUDE_PATHS\n");
        ok = false;
    }

    compiler_free_include_paths(parsed, parsedCount);
    return ok;
}

// === Feature Toggles ===
#define ENABLE_LEXER_OUTPUT      0
#define ENABLE_AST_PRINT         1
#define ENABLE_SYNTAX_CHECK      1
#define ENABLE_CODEGEN           1

int main(int argc, char **argv) {
    const char* progressEnv = getenv("FISICS_DEBUG_PROGRESS");
    bool debugProgress = progressEnv && progressEnv[0] && progressEnv[0] != '0';
    if (debugProgress) fprintf(stderr, "[main] start argc=%d\n", argc);
    profiler_init();
    if (profiler_enabled()) {
        atexit(profiler_shutdown);
    }
    const char* nanoEnv = getenv("MallocNanoZone");
    if (!nanoEnv) {
        setenv("MallocNanoZone", "0", 0);
    }

    if (!fisics_proc_guard_enter()) {
        return 1;
    }

    LLVMInstallFatalErrorHandler(llvm_fatal_handler);

    const char *filename = NULL;
    bool preservePPNodes = false;
    const char* depsJsonPath = NULL;
    const char* diagsJsonPath = NULL;
    const char* diagsPackPath = NULL;
    const char* targetTriple = NULL;
    const char* dataLayout = NULL;
    bool compileOnly = false;
    const char* outputName = NULL;
    const char* linkerPath = NULL;
    bool dumpAst = false;
    bool dumpSemantic = false;
    bool dumpIR = false;
    bool dumpTokens = false;
    bool dumpLayout = false;
    bool enableTrigraphs = false;
    bool warnIgnoredInterop = true;
    bool errorIgnoredInterop = false;
    PreprocessMode preprocessMode = PREPROCESS_INTERNAL;
    const char* externalPreprocessCmd = NULL;
    const char* externalPreprocessArgs = NULL;
    CCDialect dialect = CC_DIALECT_C99;
    bool enableExtensions = false;
    StringList includePaths = {0};
    StringList macroDefines = {0};
    StringList forcedIncludes = {0};
    StringList inputCFiles = {0};
    StringList inputOFiles = {0};
    StringList linkerSearchPaths = {0};
    StringList linkerLibs = {0};
    StringList linkerFrameworks = {0};

    if (!validate_shim_profile_contract()) {
        return 1;
    }

    // Seed include paths from default list.
    char** defaultIncludePaths = NULL;
    size_t defaultIncludeCount = 0;
    if (!compiler_collect_include_paths(DEFAULT_INCLUDE_PATHS,
                                        &defaultIncludePaths,
                                        &defaultIncludeCount)) {
        fprintf(stderr, "OOM: include paths\n");
        return 1;
    }
    if (debugProgress) fprintf(stderr, "[main] default include count=%zu\n", defaultIncludeCount);
    for (size_t i = 0; i < defaultIncludeCount; ++i) {
        string_list_push(&includePaths, defaultIncludePaths[i]);
    }
    compiler_free_include_paths(defaultIncludePaths, defaultIncludeCount);
    // Optional system include paths (e.g., macOS SDK)
    const char* sysEnv = getenv("SYSTEM_INCLUDE_PATHS");
    if (sysEnv && sysEnv[0]) {
        char** parsed = NULL;
        size_t parsedCount = 0;
        if (compiler_collect_include_paths(sysEnv, &parsed, &parsedCount)) {
            for (size_t i = 0; i < parsedCount; ++i) {
                string_list_push(&includePaths, parsed[i]);
            }
            compiler_free_include_paths(parsed, parsedCount);
        }
    }
    const char* sdkRoot = getenv("SDKROOT");
    const char* enableXcrun = getenv("ENABLE_XCRUN_DETECT");
    bool allowXcrunDetect = enableXcrun && enableXcrun[0] && enableXcrun[0] != '0';
    if (sdkRoot && sdkRoot[0]) {
        char buffer[PATH_MAX];
        snprintf(buffer, sizeof(buffer), "%s/usr/include", sdkRoot);
        append_include_dir_if_exists(&includePaths, buffer);
    } else if (allowXcrunDetect) {
        char* sdkFromXcrun = detect_sdk_include_from_xcrun();
        if (sdkFromXcrun) {
            append_include_dir_if_exists(&includePaths, sdkFromXcrun);
            free(sdkFromXcrun);
        }
    }
    // Always fall back to common macOS SDK/system include locations so we don’t hang on xcrun.
    append_include_dir_if_exists(&includePaths, "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include");
    append_include_dir_if_exists(&includePaths, "/Library/Developer/CommandLineTools/usr/include");
    append_include_dir_if_exists(&includePaths, "/opt/homebrew/include");
    append_include_dir_if_exists(&includePaths, "/opt/homebrew/include/SDL2");
    append_include_dir_if_exists(&includePaths, "/usr/local/include");
    append_include_dir_if_exists(&includePaths, "/usr/local/include/SDL2");
    append_include_dir_if_exists(&includePaths, "/usr/include");
    char* clangResource = detect_clang_resource_include();
    if (clangResource) {
        append_include_dir_if_exists(&includePaths, clangResource);
        free(clangResource);
    }

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--preserve-pp") == 0) {
            preservePPNodes = true;
        } else if (strcmp(argv[i], "--emit-deps-json") == 0 && i + 1 < argc) {
            depsJsonPath = argv[++i];
        } else if (strcmp(argv[i], "--emit-diags-json") == 0 && i + 1 < argc) {
            diagsJsonPath = argv[++i];
        } else if (strcmp(argv[i], "--emit-diags-pack") == 0 && i + 1 < argc) {
            diagsPackPath = argv[++i];
        } else if (strcmp(argv[i], "--target") == 0 && i + 1 < argc) {
            targetTriple = argv[++i];
        } else if (strcmp(argv[i], "--data-layout") == 0 && i + 1 < argc) {
            dataLayout = argv[++i];
        } else if (strcmp(argv[i], "--dump-layout") == 0) {
            dumpLayout = true;
        } else if (strcmp(argv[i], "--dump-ast") == 0) {
            dumpAst = true;
        } else if (strcmp(argv[i], "--dump-sema") == 0) {
            dumpSemantic = true;
        } else if (strcmp(argv[i], "--dump-ir") == 0) {
            dumpIR = true;
        } else if (strcmp(argv[i], "--dump-tokens") == 0) {
            dumpTokens = true;
        } else if (strcmp(argv[i], "--trigraphs") == 0) {
            enableTrigraphs = true;
        } else if (strcmp(argv[i], "-c") == 0) {
            compileOnly = true;
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            outputName = argv[++i];
        } else if (strncmp(argv[i], "-I", 2) == 0) {
            const char* path = argv[i] + 2;
            if (!*path) {
                if (i + 1 >= argc) { fprintf(stderr, "-I requires a path\n"); goto fail; }
                path = argv[++i];
            }
            if (!string_list_push(&includePaths, path)) { fprintf(stderr, "OOM: include path\n"); goto fail; }
        } else if (strncmp(argv[i], "-D", 2) == 0) {
            const char* def = argv[i] + 2;
            if (!*def) {
                if (i + 1 >= argc) { fprintf(stderr, "-D requires a macro definition\n"); goto fail; }
                def = argv[++i];
            }
            if (!string_list_push(&macroDefines, def)) { fprintf(stderr, "OOM: macro define\n"); goto fail; }
        } else if (strcmp(argv[i], "-include") == 0) {
            if (i + 1 >= argc) { fprintf(stderr, "-include requires a path\n"); goto fail; }
            if (!string_list_push(&forcedIncludes, argv[++i])) { fprintf(stderr, "OOM: forced include\n"); goto fail; }
        } else if (strncmp(argv[i], "-include", 8) == 0) {
            const char* path = argv[i] + 8;
            if (!*path) { fprintf(stderr, "-include requires a path\n"); goto fail; }
            if (!string_list_push(&forcedIncludes, path)) { fprintf(stderr, "OOM: forced include\n"); goto fail; }
        } else if (strncmp(argv[i], "-L", 2) == 0) {
            const char* path = argv[i] + 2;
            if (!*path) {
                if (i + 1 >= argc) { fprintf(stderr, "-L requires a path\n"); goto fail; }
                path = argv[++i];
            }
            if (!string_list_push(&linkerSearchPaths, path)) { fprintf(stderr, "OOM: -L path\n"); goto fail; }
        } else if (strncmp(argv[i], "-l", 2) == 0) {
            const char* lib = argv[i] + 2;
            if (!*lib) {
                if (i + 1 >= argc) { fprintf(stderr, "-l requires a library name\n"); goto fail; }
                lib = argv[++i];
            }
            if (!string_list_push(&linkerLibs, lib)) { fprintf(stderr, "OOM: -l name\n"); goto fail; }
        } else if (strcmp(argv[i], "-framework") == 0) {
            if (i + 1 >= argc) { fprintf(stderr, "-framework requires a framework name\n"); goto fail; }
            if (!string_list_push(&linkerFrameworks, argv[++i])) {
                fprintf(stderr, "OOM: framework name\n");
                goto fail;
            }
        } else if (strncmp(argv[i], "--linker=", 9) == 0) {
            linkerPath = argv[i] + 9;
        } else if (strcmp(argv[i], "--no-warn-ignored-cc") == 0) {
            warnIgnoredInterop = false;
        } else if (strcmp(argv[i], "--error-ignored-cc") == 0) {
            errorIgnoredInterop = true;
        } else if (strncmp(argv[i], "--dialect=", 10) == 0) {
            const char* mode = argv[i] + 10;
            if (strcmp(mode, "c99") == 0) {
                dialect = CC_DIALECT_C99;
            } else if (strcmp(mode, "c11") == 0) {
                dialect = CC_DIALECT_C11;
            } else if (strcmp(mode, "c17") == 0) {
                dialect = CC_DIALECT_C17;
            } else {
                fprintf(stderr, "Error: unknown dialect '%s'\n", mode);
                goto fail;
            }
        } else if (strncmp(argv[i], "--extensions=", 13) == 0) {
            const char* mode = argv[i] + 13;
            if (strcmp(mode, "gnu") == 0 || strcmp(mode, "on") == 0) {
                enableExtensions = true;
            } else if (strcmp(mode, "off") == 0 || strcmp(mode, "none") == 0) {
                enableExtensions = false;
            } else {
                fprintf(stderr, "Error: unknown extensions mode '%s'\n", mode);
                goto fail;
            }
        } else if (strncmp(argv[i], "--preprocess=", 13) == 0) {
            const char* mode = argv[i] + 13;
            if (strcmp(mode, "internal") == 0) {
                preprocessMode = PREPROCESS_INTERNAL;
            } else if (strcmp(mode, "external") == 0) {
                preprocessMode = PREPROCESS_EXTERNAL;
            } else if (strcmp(mode, "hybrid") == 0) {
                preprocessMode = PREPROCESS_HYBRID;
            } else {
                fprintf(stderr, "Error: unknown preprocess mode '%s'\n", mode);
                goto fail;
            }
        } else if (strcmp(argv[i], "--preprocess-cmd") == 0 && i + 1 < argc) {
            externalPreprocessCmd = argv[++i];
        } else if (strncmp(argv[i], "--preprocess-cmd=", 17) == 0) {
            externalPreprocessCmd = argv[i] + 17;
        } else if (strcmp(argv[i], "--preprocess-args") == 0 && i + 1 < argc) {
            externalPreprocessArgs = argv[++i];
        } else if (strncmp(argv[i], "--preprocess-args=", 18) == 0) {
            externalPreprocessArgs = argv[i] + 18;
        } else if (argv[i][0] != '-' && !filename) {
            filename = argv[i];
            if (has_extension(filename, ".c")) {
                string_list_push(&inputCFiles, filename);
            } else if (has_extension(filename, ".o") ||
                       has_extension(filename, ".a") ||
                       has_extension(filename, ".so") ||
                       has_extension(filename, ".dylib")) {
                string_list_push(&inputOFiles, filename);
            } else {
                fprintf(stderr, "Warning: unrecognized input extension for %s\n", filename);
            }
        } else if (argv[i][0] != '-') {
            if (has_extension(argv[i], ".c")) {
                string_list_push(&inputCFiles, argv[i]);
            } else if (has_extension(argv[i], ".o") ||
                       has_extension(argv[i], ".a") ||
                       has_extension(argv[i], ".so") ||
                       has_extension(argv[i], ".dylib")) {
                string_list_push(&inputOFiles, argv[i]);
            } else {
                fprintf(stderr, "Warning: unrecognized input extension for %s\n", argv[i]);
            }
        }
    }
    if (!filename && inputCFiles.count == 0 && inputOFiles.count == 0) {
        filename = "include/test.txt";
        string_list_push(&inputCFiles, filename);
    }

    int enableCodegen = ENABLE_CODEGEN;
    const char* disableCodegenEnv = getenv("DISABLE_CODEGEN");
    if (disableCodegenEnv && disableCodegenEnv[0] != '\0' && disableCodegenEnv[0] != '0') {
        enableCodegen = 0;
    }

    const char* preserveEnv = getenv("PRESERVE_PP_NODES");
    if (preserveEnv && preserveEnv[0] != '\0' && preserveEnv[0] != '0') {
        preservePPNodes = true;
    }
    const char* triEnv = getenv("ENABLE_TRIGRAPHS");
    if (triEnv && triEnv[0] != '\0' && triEnv[0] != '0') {
        enableTrigraphs = true;
    }
    const char* ppEnv = getenv("FISICS_PREPROCESS");
    if (ppEnv && ppEnv[0]) {
        if (strcmp(ppEnv, "external") == 0) {
            preprocessMode = PREPROCESS_EXTERNAL;
        } else if (strcmp(ppEnv, "hybrid") == 0) {
            preprocessMode = PREPROCESS_HYBRID;
        } else if (strcmp(ppEnv, "internal") == 0) {
            preprocessMode = PREPROCESS_INTERNAL;
        }
    }
    const char* externalCmdEnv = getenv("FISICS_EXTERNAL_CPP");
    if (externalCmdEnv && externalCmdEnv[0]) {
        externalPreprocessCmd = externalCmdEnv;
    }
    const char* externalArgsEnv = getenv("FISICS_EXTERNAL_CPP_ARGS");
    if (externalArgsEnv && externalArgsEnv[0]) {
        externalPreprocessArgs = externalArgsEnv;
    }
    const char* dialectEnv = getenv("FISICS_DIALECT");
    if (dialectEnv && dialectEnv[0]) {
        if (strcmp(dialectEnv, "c99") == 0) {
            dialect = CC_DIALECT_C99;
        } else if (strcmp(dialectEnv, "c11") == 0) {
            dialect = CC_DIALECT_C11;
        } else if (strcmp(dialectEnv, "c17") == 0) {
            dialect = CC_DIALECT_C17;
        }
    }
    const char* extEnv = getenv("FISICS_EXTENSIONS");
    if (extEnv && extEnv[0]) {
        enableExtensions = (strcmp(extEnv, "1") == 0 ||
                            strcmp(extEnv, "on") == 0 ||
                            strcmp(extEnv, "gnu") == 0);
    }

    const char* depsEnv = getenv("EMIT_DEPS_JSON");
    if (!depsJsonPath && depsEnv && depsEnv[0] != '\0') {
        depsJsonPath = depsEnv;
    }
    const char* diagsEnv = getenv("EMIT_DIAGS_JSON");
    if (!diagsJsonPath && diagsEnv && diagsEnv[0] != '\0') {
        diagsJsonPath = diagsEnv;
    }
    const char* diagsPackEnv = getenv("EMIT_DIAGS_PACK");
    if (!diagsPackPath && diagsPackEnv && diagsPackEnv[0] != '\0') {
        diagsPackPath = diagsPackEnv;
    }
    const char* warnInteropEnv = getenv("WARN_IGNORED_CC");
    if (warnInteropEnv && warnInteropEnv[0] != '\0') {
        warnIgnoredInterop = warnInteropEnv[0] != '0';
    }
    const char* errInteropEnv = getenv("ERROR_IGNORED_CC");
    if (errInteropEnv && errInteropEnv[0] != '\0' && errInteropEnv[0] != '0') {
        errorIgnoredInterop = true;
    }

    if (dumpLayout) {
        const TargetLayout* tl = tl_from_triple(targetTriple);
        tl_dump(tl, stdout);
        return 0;
    }

    bool driverMode = compileOnly || outputName || inputOFiles.count > 0 ||
                      linkerSearchPaths.count > 0 || linkerLibs.count > 0 ||
                      linkerFrameworks.count > 0 || linkerPath;
    if (driverMode) {
        if (compileOnly) {
            if (inputCFiles.count == 0) {
                fprintf(stderr, "Error: no .c inputs provided for -c\n");
                goto fail;
            }
            if (outputName && inputCFiles.count != 1) {
                fprintf(stderr, "Error: -o with -c requires exactly one .c input\n");
                goto fail;
            }
            if (!enableCodegen) {
                fprintf(stderr, "Error: codegen disabled (DISABLE_CODEGEN set); cannot emit object files\n");
                goto fail;
            }

            for (size_t i = 0; i < inputCFiles.count; ++i) {
                const char* cPath = inputCFiles.items[i];
                char* diagPath = derive_diag_json_path(diagsJsonPath, i, inputCFiles.count);
                char* diagPackPathForInput = derive_diag_pack_path(diagsPackPath, i, inputCFiles.count);
                char* objPath = NULL;
                if (outputName) {
                    objPath = strdup(outputName);
                } else {
                    objPath = derive_object_path(cPath);
                }
                if (!objPath) {
                    fprintf(stderr, "Error: failed to compute output path for %s\n", cPath);
                    free(diagPath);
                    free(diagPackPathForInput);
                    goto fail;
                }

                CompileOptions options = {
                    .inputPath = cPath,
                    .preservePPNodes = preservePPNodes,
                    .enableTrigraphs = enableTrigraphs,
                    .depsJsonPath = depsJsonPath,
                    .targetTriple = targetTriple,
                    .dataLayout = dataLayout,
                    .includePaths = (const char* const*)includePaths.items,
                    .includePathCount = includePaths.count,
                    .macroDefines = (const char* const*)macroDefines.items,
                    .macroDefineCount = macroDefines.count,
                    .forcedIncludes = (const char* const*)forcedIncludes.items,
                    .forcedIncludeCount = forcedIncludes.count,
                    .preprocessMode = preprocessMode,
                    .externalPreprocessCmd = externalPreprocessCmd,
                    .externalPreprocessArgs = externalPreprocessArgs,
                    .dialect = dialect,
                    .enableExtensions = enableExtensions,
                    .dumpAst = dumpAst,
                    .dumpSemantic = dumpSemantic,
                    .dumpIR = dumpIR,
                    .dumpTokens = dumpTokens,
                    .enableCodegen = enableCodegen,
                    .warnIgnoredInterop = warnIgnoredInterop,
                    .errorIgnoredInterop = errorIgnoredInterop
                };

                CompileResult result;
                int status = compile_translation_unit(&options, &result);
                if (diagPath && result.compilerCtx) {
                    CoreResult wr = compiler_diagnostics_write_core_dataset_json(result.compilerCtx, diagPath);
                    if (wr.code != CORE_OK) {
                        fprintf(stderr, "Warning: failed to write diagnostics JSON to %s\n", diagPath);
                    }
                }
                if (diagPackPathForInput && result.compilerCtx) {
                    CoreResult wr = compiler_diagnostics_write_core_dataset_pack(result.compilerCtx, diagPackPathForInput);
                    if (wr.code != CORE_OK) {
                        fprintf(stderr, "Warning: failed to write diagnostics pack to %s\n", diagPackPathForInput);
                    }
                }
                if (status != 0 || result.semanticErrors > 0 || !result.module) {
                    fprintf(stderr, "Error: compilation failed for %s\n", cPath);
                    free(diagPath);
                    free(diagPackPathForInput);
                    free(objPath);
                    compile_result_destroy(&result);
                    goto fail;
                }
                free(diagPath);
                free(diagPackPathForInput);

                char* emitErr = NULL;
                if (!compiler_emit_object_file(result.module,
                                               targetTriple,
                                               dataLayout,
                                               objPath,
                                               &emitErr)) {
                    fprintf(stderr, "Error: failed to emit object %s: %s\n",
                            objPath,
                            emitErr ? emitErr : "unknown error");
                    free(emitErr);
                    free(objPath);
                    compile_result_destroy(&result);
                    goto fail;
                }
                free(emitErr);
                compile_result_destroy(&result);
                free(objPath);
            }

            string_list_free(&includePaths);
            string_list_free(&macroDefines);
            string_list_free(&forcedIncludes);
            string_list_free(&inputCFiles);
            string_list_free(&inputOFiles);
            string_list_free(&linkerSearchPaths);
            string_list_free(&linkerLibs);
            string_list_free(&linkerFrameworks);
            return 0;
        } else {
            if (inputCFiles.count == 0 && inputOFiles.count == 0) {
                fprintf(stderr, "Error: no inputs provided\n");
                goto fail;
            }
            if (!enableCodegen && inputCFiles.count > 0) {
                fprintf(stderr, "Error: codegen disabled (DISABLE_CODEGEN set); cannot compile .c inputs for linking\n");
                goto fail;
            }

            StringList tempObjects = {0};
            bool allOk = true;
            CrossTUVarDefList crossTuDefs = {0};
            CrossTUTypeConflict crossTuConflict = {0};

            // Step A: compile all .c to temp .o
            for (size_t i = 0; i < inputCFiles.count; ++i) {
                const char* cPath = inputCFiles.items[i];
                char* diagPath = derive_diag_json_path(diagsJsonPath, i, inputCFiles.count);
                char* diagPackPathForInput = derive_diag_pack_path(diagsPackPath, i, inputCFiles.count);
                char* objPath = create_temp_object_path(cPath);
                if (!objPath) { free(diagPath); free(diagPackPathForInput); allOk = false; break; }

                CompileOptions options = {
                    .inputPath = cPath,
                    .preservePPNodes = preservePPNodes,
                    .enableTrigraphs = enableTrigraphs,
                    .depsJsonPath = NULL,
                    .targetTriple = targetTriple,
                    .dataLayout = dataLayout,
                    .includePaths = (const char* const*)includePaths.items,
                    .includePathCount = includePaths.count,
                    .macroDefines = (const char* const*)macroDefines.items,
                    .macroDefineCount = macroDefines.count,
                    .forcedIncludes = (const char* const*)forcedIncludes.items,
                    .forcedIncludeCount = forcedIncludes.count,
                    .preprocessMode = preprocessMode,
                    .externalPreprocessCmd = externalPreprocessCmd,
                    .externalPreprocessArgs = externalPreprocessArgs,
                    .dialect = dialect,
                    .enableExtensions = enableExtensions,
                    .dumpAst = dumpAst,
                    .dumpSemantic = dumpSemantic,
                    .dumpIR = dumpIR,
                    .dumpTokens = dumpTokens,
                    .enableCodegen = enableCodegen,
                    .warnIgnoredInterop = warnIgnoredInterop,
                    .errorIgnoredInterop = errorIgnoredInterop
                };

                CompileResult result;
                int status = compile_translation_unit(&options, &result);
                if (diagPath && result.compilerCtx) {
                    CoreResult wr = compiler_diagnostics_write_core_dataset_json(result.compilerCtx, diagPath);
                    if (wr.code != CORE_OK) {
                        fprintf(stderr, "Warning: failed to write diagnostics JSON to %s\n", diagPath);
                    }
                }
                if (diagPackPathForInput && result.compilerCtx) {
                    CoreResult wr = compiler_diagnostics_write_core_dataset_pack(result.compilerCtx, diagPackPathForInput);
                    if (wr.code != CORE_OK) {
                        fprintf(stderr, "Warning: failed to write diagnostics pack to %s\n", diagPackPathForInput);
                    }
                }
                if (status != 0 || result.semanticErrors > 0 || !result.module) {
                    fprintf(stderr, "Error: compilation failed for %s\n", cPath);
                    free(diagPath);
                    free(diagPackPathForInput);
                    free(objPath);
                    compile_result_destroy(&result);
                    allOk = false;
                    break;
                }

                if (!collect_cross_tu_virtual_type_conflict(result.semanticModel,
                                                             result.compilerCtx,
                                                             &crossTuDefs,
                                                             &crossTuConflict)) {
                    fprintf(stderr, "OOM: cross-tu conflict tracking\n");
                    free(diagPath);
                    free(diagPackPathForInput);
                    free(objPath);
                    compile_result_destroy(&result);
                    allOk = false;
                    break;
                }

                if (crossTuConflict.found) {
                    print_semantic_conflict_text(&crossTuConflict);
                    if (diagsJsonPath && diagsJsonPath[0] != '\0') {
                        if (!write_semantic_conflict_diag_json(diagsJsonPath, &crossTuConflict)) {
                            fprintf(stderr,
                                    "Warning: failed to write semantic conflict diagnostics JSON to %s\n",
                                    diagsJsonPath);
                        }
                    }
                    free(diagPath);
                    free(diagPackPathForInput);
                    free(objPath);
                    compile_result_destroy(&result);
                    allOk = false;
                    break;
                }

                free(diagPath);
                free(diagPackPathForInput);

                char* emitErr = NULL;
                if (!compiler_emit_object_file(result.module,
                                               targetTriple,
                                               dataLayout,
                                               objPath,
                                               &emitErr)) {
                    fprintf(stderr, "Error: failed to emit object %s: %s\n",
                            objPath,
                            emitErr ? emitErr : "unknown error");
                    free(emitErr);
                    free(objPath);
                    compile_result_destroy(&result);
                    allOk = false;
                    break;
                }
                free(emitErr);
                compile_result_destroy(&result);
                if (!string_list_push(&tempObjects, objPath)) {
                    fprintf(stderr, "OOM: temp object list\n");
                    free(objPath);
                    allOk = false;
                    break;
                }
                free(objPath);
            }

            if (!allOk) {
                for (size_t i = 0; i < tempObjects.count; ++i) {
                    unlink(tempObjects.items[i]);
                }
                string_list_free(&tempObjects);
                cross_tu_var_defs_free(&crossTuDefs);
                cross_tu_type_conflict_clear(&crossTuConflict);
                goto fail;
            }

            // Step B: build linker argv
            const char* linker = linkerPath ? linkerPath : "clang";
            StringList argvList = {0};
            if (!string_list_push(&argvList, linker)) {
                allOk = false;
            }
            for (size_t i = 0; allOk && i < inputOFiles.count; ++i) {
                allOk = string_list_push(&argvList, inputOFiles.items[i]);
            }
            for (size_t i = 0; allOk && i < tempObjects.count; ++i) {
                allOk = string_list_push(&argvList, tempObjects.items[i]);
            }
            for (size_t i = 0; allOk && i < linkerSearchPaths.count; ++i) {
                size_t len = strlen(linkerSearchPaths.items[i]) + 3;
                char* flag = (char*)malloc(len);
                if (!flag) { allOk = false; break; }
                snprintf(flag, len, "-L%s", linkerSearchPaths.items[i]);
                allOk = string_list_push(&argvList, flag);
                free(flag);
            }
            for (size_t i = 0; allOk && i < linkerLibs.count; ++i) {
                size_t len = strlen(linkerLibs.items[i]) + 3;
                char* flag = (char*)malloc(len);
                if (!flag) { allOk = false; break; }
                snprintf(flag, len, "-l%s", linkerLibs.items[i]);
                allOk = string_list_push(&argvList, flag);
                free(flag);
            }
            for (size_t i = 0; allOk && i < linkerFrameworks.count; ++i) {
                allOk = string_list_push(&argvList, "-framework") &&
                        string_list_push(&argvList, linkerFrameworks.items[i]);
            }
            const char* finalOutput = outputName ? outputName : "a.out";
            if (allOk) {
                allOk = string_list_push(&argvList, "-o") &&
                        string_list_push(&argvList, finalOutput);
            }

            if (!allOk) {
                fprintf(stderr, "Error: failed to prepare linker invocation\n");
                for (size_t i = 0; i < tempObjects.count; ++i) {
                    unlink(tempObjects.items[i]);
                }
                string_list_free(&tempObjects);
                string_list_free(&argvList);
                cross_tu_var_defs_free(&crossTuDefs);
                cross_tu_type_conflict_clear(&crossTuConflict);
                goto fail;
            }

            // execvp-style array
            char** execArgv = calloc(argvList.count + 1, sizeof(char*));
            if (!execArgv) {
                fprintf(stderr, "OOM: linker argv\n");
                for (size_t i = 0; i < tempObjects.count; ++i) {
                    unlink(tempObjects.items[i]);
                }
                string_list_free(&tempObjects);
                string_list_free(&argvList);
                cross_tu_var_defs_free(&crossTuDefs);
                cross_tu_type_conflict_clear(&crossTuConflict);
                goto fail;
            }
            for (size_t i = 0; i < argvList.count; ++i) {
                execArgv[i] = argvList.items[i];
            }
            execArgv[argvList.count] = NULL;

            pid_t pid = fork();
            if (pid == 0) {
                execvp(linker, execArgv);
                perror("execvp");
                _exit(127);
            } else if (pid < 0) {
                perror("fork");
                free(execArgv);
                for (size_t i = 0; i < tempObjects.count; ++i) {
                    unlink(tempObjects.items[i]);
                }
                string_list_free(&tempObjects);
                string_list_free(&argvList);
                cross_tu_var_defs_free(&crossTuDefs);
                cross_tu_type_conflict_clear(&crossTuConflict);
                goto fail;
            }

            print_argv("[link]", &argvList);
            int statusCode = 0;
            if (waitpid(pid, &statusCode, 0) == -1) {
                perror("waitpid");
                statusCode = 1;
            } else if (WIFEXITED(statusCode)) {
                statusCode = WEXITSTATUS(statusCode);
                if (statusCode != 0) {
                    fprintf(stderr, "Linker exited with status %d\n", statusCode);
                }
            } else {
                fprintf(stderr, "Linker terminated abnormally\n");
                statusCode = 1;
            }
            if (statusCode != 0 && diagsJsonPath && diagsJsonPath[0] != '\0') {
                if (!write_link_stage_diag_json(
                        diagsJsonPath,
                        statusCode,
                        linker,
                        finalOutput,
                        inputOFiles.count + inputCFiles.count)) {
                    fprintf(stderr,
                            "Warning: failed to write link-stage diagnostics JSON to %s\n",
                            diagsJsonPath);
                }
            }

            for (size_t i = 0; i < tempObjects.count; ++i) {
                unlink(tempObjects.items[i]);
            }

            free(execArgv);
            string_list_free(&tempObjects);
            string_list_free(&argvList);
            cross_tu_var_defs_free(&crossTuDefs);
            cross_tu_type_conflict_clear(&crossTuConflict);

            string_list_free(&includePaths);
            string_list_free(&macroDefines);
            string_list_free(&forcedIncludes);
            string_list_free(&inputCFiles);
            string_list_free(&inputOFiles);
            string_list_free(&linkerSearchPaths);
            string_list_free(&linkerLibs);
            string_list_free(&linkerFrameworks);
            return llvm_shutdown_and_return(statusCode);
        }
    }

    const char* inputPath = (inputCFiles.count > 0) ? inputCFiles.items[0] : filename;

    if (debugProgress) fprintf(stderr, "[main] starting compile for %s\n", inputPath ? inputPath : "<null>");
    CompileOptions options = {
        .inputPath = inputPath,
        .preservePPNodes = preservePPNodes,
        .enableTrigraphs = enableTrigraphs,
        .depsJsonPath = depsJsonPath,
        .targetTriple = targetTriple,
        .dataLayout = dataLayout,
        .includePaths = (const char* const*)includePaths.items,
        .includePathCount = includePaths.count,
        .macroDefines = (const char* const*)macroDefines.items,
        .macroDefineCount = macroDefines.count,
        .forcedIncludes = (const char* const*)forcedIncludes.items,
        .forcedIncludeCount = forcedIncludes.count,
        .preprocessMode = preprocessMode,
        .externalPreprocessCmd = externalPreprocessCmd,
        .externalPreprocessArgs = externalPreprocessArgs,
        .dialect = dialect,
        .enableExtensions = enableExtensions,
        .dumpAst = dumpAst || ENABLE_AST_PRINT,
        .dumpSemantic = dumpSemantic || ENABLE_SYNTAX_CHECK,
        .dumpIR = dumpIR || (enableCodegen && ENABLE_CODEGEN),
        .dumpTokens = dumpTokens,
        .enableCodegen = enableCodegen,
        .warnIgnoredInterop = warnIgnoredInterop,
        .errorIgnoredInterop = errorIgnoredInterop
    };

    CompileResult result;
    int status = compile_translation_unit(&options, &result);
    if (result.compilerCtx && diagsJsonPath && diagsJsonPath[0] != '\0') {
        CoreResult wr = compiler_diagnostics_write_core_dataset_json(result.compilerCtx, diagsJsonPath);
        if (wr.code != CORE_OK) {
            fprintf(stderr, "Warning: failed to write diagnostics JSON to %s\n", diagsJsonPath);
        }
    }
    if (result.compilerCtx && diagsPackPath && diagsPackPath[0] != '\0') {
        CoreResult wr = compiler_diagnostics_write_core_dataset_pack(result.compilerCtx, diagsPackPath);
        if (wr.code != CORE_OK) {
            fprintf(stderr, "Warning: failed to write diagnostics pack to %s\n", diagsPackPath);
        }
    }

    compile_result_destroy(&result);
    string_list_free(&includePaths);
    string_list_free(&macroDefines);
    string_list_free(&forcedIncludes);
    string_list_free(&inputCFiles);
    string_list_free(&inputOFiles);
    string_list_free(&linkerSearchPaths);
    string_list_free(&linkerLibs);
    string_list_free(&linkerFrameworks);
    return llvm_shutdown_and_return(status);

fail:
    string_list_free(&includePaths);
    string_list_free(&macroDefines);
    string_list_free(&forcedIncludes);
    string_list_free(&inputCFiles);
    string_list_free(&inputOFiles);
    string_list_free(&linkerSearchPaths);
    string_list_free(&linkerLibs);
    string_list_free(&linkerFrameworks);
    return llvm_shutdown_and_return(1);
}
