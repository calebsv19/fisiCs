// SPDX-License-Identifier: Apache-2.0

#include "main_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "Compiler/compiler_context.h"

static char* dup_with_indexed_ext(const char* basePath,
                                  const char* extName,
                                  size_t index,
                                  size_t total) {
    if (!basePath || !basePath[0]) return NULL;
    if (total <= 1u) return strdup(basePath);

    const char* ext = strrchr(basePath, '.');
    size_t extLen = strlen(extName);
    if (ext && strcmp(ext, extName) == 0) {
        size_t baseLen = (size_t)(ext - basePath);
        size_t need = baseLen + 1u + 20u + extLen + 1u;
        char* out = (char*)malloc(need);
        if (!out) return NULL;
        snprintf(out, need, "%.*s.%zu%s", (int)baseLen, basePath, index + 1u, ext);
        return out;
    }

    size_t need = strlen(basePath) + 1u + 20u + extLen + 1u;
    char* out = (char*)malloc(need);
    if (!out) return NULL;
    snprintf(out, need, "%s.%zu%s", basePath, index + 1u, extName);
    return out;
}

char* main_create_temp_object_path(const char* baseName) {
    (void)baseName;
    char tmpl[] = "/tmp/mycc-XXXXXX";
    int fd = mkstemp(tmpl);
    if (fd == -1) {
        perror("mkstemp");
        return NULL;
    }
    close(fd);
    size_t len = strlen(tmpl);
    char* withExt = (char*)malloc(len + 3u);
    if (!withExt) {
        unlink(tmpl);
        return NULL;
    }
    snprintf(withExt, len + 3u, "%s.o", tmpl);
    if (rename(tmpl, withExt) != 0) {
        perror("rename");
        unlink(tmpl);
        free(withExt);
        return NULL;
    }
    return withExt;
}

char* main_derive_diag_json_path(const char* basePath, size_t index, size_t total) {
    return dup_with_indexed_ext(basePath, ".json", index, total);
}

char* main_derive_diag_pack_path(const char* basePath, size_t index, size_t total) {
    return dup_with_indexed_ext(basePath, ".pack", index, total);
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

bool main_write_link_stage_diag_json(const char* outPath,
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

void main_cross_tu_var_defs_free(CrossTUVarDefList* defs) {
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

void main_cross_tu_type_conflict_clear(CrossTUTypeConflict* conflict) {
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
    out->hasDefinition = sym->hasDefinition;
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
    main_cross_tu_type_conflict_clear(conflict);
    conflict->symbolName = strdup(symbolName);
    if (!conflict->symbolName) return false;
    if (filePath && filePath[0] != '\0') {
        conflict->filePath = strdup(filePath);
        if (!conflict->filePath) {
            main_cross_tu_type_conflict_clear(conflict);
            return false;
        }
    }
    if (previousFilePath && previousFilePath[0] != '\0') {
        conflict->previousFilePath = strdup(previousFilePath);
        if (!conflict->previousFilePath) {
            main_cross_tu_type_conflict_clear(conflict);
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

static bool cross_tu_array_bounds_compatible(const ParsedType* lhs, const ParsedType* rhs) {
    if (!lhs || !rhs) return false;

    size_t lhsArrayCount = parsedTypeCountDerivationsOfKind(lhs, TYPE_DERIVATION_ARRAY);
    size_t rhsArrayCount = parsedTypeCountDerivationsOfKind(rhs, TYPE_DERIVATION_ARRAY);
    if (lhsArrayCount != rhsArrayCount) {
        return false;
    }

    for (size_t i = 0; i < lhsArrayCount; ++i) {
        const TypeDerivation* lhsArr = parsedTypeGetArrayDerivation(lhs, i);
        const TypeDerivation* rhsArr = parsedTypeGetArrayDerivation(rhs, i);
        if (!lhsArr || !rhsArr) {
            return false;
        }
        if (lhsArr->as.array.isVLA != rhsArr->as.array.isVLA) {
            return false;
        }

        long long lhsBound = 0;
        long long rhsBound = 0;
        bool lhsKnown = false;
        bool rhsKnown = false;
        if (!lhsArr->as.array.isVLA && !lhsArr->as.array.isFlexible) {
            if (lhsArr->as.array.hasConstantSize) {
                lhsKnown = true;
                lhsBound = lhsArr->as.array.constantSize;
            } else if (lhsArr->as.array.sizeExpr &&
                       (lhsArr->as.array.sizeExpr->type == AST_NUMBER_LITERAL ||
                        lhsArr->as.array.sizeExpr->type == AST_CHAR_LITERAL) &&
                       lhsArr->as.array.sizeExpr->valueNode.value) {
                char* end = NULL;
                long long parsed = strtoll(lhsArr->as.array.sizeExpr->valueNode.value, &end, 0);
                if (end && *end == '\0') {
                    lhsKnown = true;
                    lhsBound = parsed;
                }
            }
        }
        if (!rhsArr->as.array.isVLA && !rhsArr->as.array.isFlexible) {
            if (rhsArr->as.array.hasConstantSize) {
                rhsKnown = true;
                rhsBound = rhsArr->as.array.constantSize;
            } else if (rhsArr->as.array.sizeExpr &&
                       (rhsArr->as.array.sizeExpr->type == AST_NUMBER_LITERAL ||
                        rhsArr->as.array.sizeExpr->type == AST_CHAR_LITERAL) &&
                       rhsArr->as.array.sizeExpr->valueNode.value) {
                char* end = NULL;
                long long parsed = strtoll(rhsArr->as.array.sizeExpr->valueNode.value, &end, 0);
                if (end && *end == '\0') {
                    rhsKnown = true;
                    rhsBound = parsed;
                }
            }
        }
        bool lhsIncomplete =
            !lhsKnown &&
            !lhsArr->as.array.isVLA &&
            !lhsArr->as.array.isFlexible &&
            lhsArr->as.array.sizeExpr == NULL;
        bool rhsIncomplete =
            !rhsKnown &&
            !rhsArr->as.array.isVLA &&
            !rhsArr->as.array.isFlexible &&
            rhsArr->as.array.sizeExpr == NULL;

        if (lhsKnown && rhsKnown && lhsBound != rhsBound) {
            return false;
        }
        if (lhsKnown != rhsKnown && !(lhsIncomplete || rhsIncomplete)) {
            return false;
        }
    }

    return true;
}

typedef struct {
    CrossTUVarDefList* defs;
    CrossTUTypeConflict* conflict;
    const CompilerContext* compilerCtx;
    bool oom;
} CrossTUCollectCtx;

static void cross_tu_collect_symbol_cb(const Symbol* sym, void* userData) {
    CrossTUCollectCtx* ctx = (CrossTUCollectCtx*)userData;
    if (!ctx || !ctx->defs || !ctx->conflict || ctx->oom || ctx->conflict->found) return;
    if (!sym || sym->kind != SYMBOL_VARIABLE) return;
    if (sym->linkage != LINKAGE_EXTERNAL) return;
    if (!sym->hasDefinition && sym->storage != STORAGE_EXTERN) return;

    const char* filePath = NULL;
    int line = 0;
    int column = 0;
    if (!compiler_ctx_symbol_spelling_location(ctx->compilerCtx, sym->name, &filePath, &line, &column)) {
        if (!symbol_spelling_location(sym, &filePath, &line, &column)) {
            filePath = NULL;
            line = 0;
            column = 0;
        }
    }
    if (line > 0) line += 1;
    bool virtualSpelling = filePath && filePath[0] && (access(filePath, F_OK) != 0);

    for (size_t i = 0; i < ctx->defs->count; ++i) {
        const CrossTUVarDef* prior = &ctx->defs->items[i];
        if (!prior->name || strcmp(prior->name, sym->name) != 0) continue;
        bool structEqual = parsedTypesStructurallyEqual(&prior->type, &sym->type);
        bool arrayBoundsCompatible = cross_tu_array_bounds_compatible(&prior->type, &sym->type);
        if (structEqual && arrayBoundsCompatible) continue;
        if (!(prior->hasDefinition || sym->hasDefinition)) continue;
        bool arrayBoundsConflict = structEqual && !arrayBoundsCompatible;
        if (!arrayBoundsConflict && !prior->virtualSpelling && !virtualSpelling) {
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

bool main_collect_cross_tu_virtual_type_conflict(const SemanticModel* model,
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

bool main_write_semantic_conflict_diag_json(const char* outPath,
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

void main_print_semantic_conflict_text(const CrossTUTypeConflict* conflict) {
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
