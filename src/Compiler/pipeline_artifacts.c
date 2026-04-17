// SPDX-License-Identifier: Apache-2.0

#include "Compiler/pipeline_internal.h"

#include <stdlib.h>
#include <string.h>

#include "Parser/Helpers/parsed_type_format.h"

typedef struct {
    FisicsSymbol* items;
    size_t count;
    size_t capacity;
} SymbolBuffer;

typedef struct {
    SymbolBuffer* buf;
    const char* file_path;
    bool include_system_symbols;
} SymbolCollectCtx;

static bool symbuf_append(SymbolBuffer* buf, const FisicsSymbol* sym) {
    if (!buf || !sym) return false;
    if (buf->count == buf->capacity) {
        size_t newCap = buf->capacity ? buf->capacity * 2 : 8;
        FisicsSymbol* grown = (FisicsSymbol*)realloc(buf->items, newCap * sizeof(FisicsSymbol));
        if (!grown) return false;
        buf->items = grown;
        buf->capacity = newCap;
    }
    buf->items[buf->count++] = *sym;
    return true;
}

static const char* identifier_name(const ASTNode* node) {
    if (!node) return NULL;
    if (node->type == AST_IDENTIFIER) {
        return node->valueNode.value;
    }
    return NULL;
}

static FisicsSymbolKind map_symbol_kind(SymbolKind kind) {
    switch (kind) {
        case SYMBOL_FUNCTION: return FISICS_SYMBOL_FUNCTION;
        case SYMBOL_STRUCT: return FISICS_SYMBOL_STRUCT;
        case SYMBOL_ENUM: return FISICS_SYMBOL_ENUM;
        case SYMBOL_TYPEDEF: return FISICS_SYMBOL_TYPEDEF;
        case SYMBOL_VARIABLE: return FISICS_SYMBOL_VARIABLE;
        default: return FISICS_SYMBOL_UNKNOWN;
    }
}

static FisicsSymbolKind map_definition_kind(const Symbol* sym) {
    if (!sym || !sym->definition) return map_symbol_kind(sym ? sym->kind : SYMBOL_VARIABLE);
    switch (sym->definition->type) {
        case AST_STRUCT_DEFINITION: return FISICS_SYMBOL_STRUCT;
        case AST_UNION_DEFINITION: return FISICS_SYMBOL_UNION;
        case AST_ENUM_DEFINITION: return FISICS_SYMBOL_ENUM;
        default: return map_symbol_kind(sym->kind);
    }
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
            if (def->varDecl.varNames && def->varDecl.varCount > 0) {
                ASTNode** varNames = def->varDecl.varNames;
                for (size_t i = 0; i < def->varDecl.varCount; ++i) {
                    ASTNode* varName = varNames ? varNames[i] : NULL;
                    const char* name = identifier_name(varName);
                    if (name && sym->name && strcmp(name, sym->name) == 0) {
                        return varName;
                    }
                }
                return varNames ? varNames[0] : NULL;
            }
            return NULL;
        }
        default:
            return NULL;
    }
}

static const char* param_decl_name(const ASTNode* paramDecl) {
    if (!paramDecl || paramDecl->type != AST_VARIABLE_DECLARATION) return NULL;
    if (!paramDecl->varDecl.varNames || paramDecl->varDecl.varCount == 0) return NULL;
    ASTNode** varNames = paramDecl->varDecl.varNames;
    return identifier_name(varNames ? varNames[0] : NULL);
}

static void append_struct_field_symbols(SymbolBuffer* buf, const Symbol* sym, const ASTNode* def) {
    if (!buf || !sym || !def) return;
    if (def->type != AST_STRUCT_DEFINITION && def->type != AST_UNION_DEFINITION) return;

    const FisicsSymbolKind parentKind = (def->type == AST_UNION_DEFINITION)
        ? FISICS_SYMBOL_UNION
        : FISICS_SYMBOL_STRUCT;

    ASTNode** fields = def->structDef.fields;
    for (size_t i = 0; i < def->structDef.fieldCount; ++i) {
        ASTNode* field = fields ? fields[i] : NULL;
        if (!field || field->type != AST_VARIABLE_DECLARATION) continue;
        size_t varCount = field->varDecl.varCount;
        ASTNode** varNames = field->varDecl.varNames;
        for (size_t v = 0; v < varCount; ++v) {
            ASTNode* nameNode = varNames ? varNames[v] : NULL;
            const char* name = identifier_name(nameNode);
            if (!name || !name[0]) continue;

            const ParsedType* fieldType = NULL;
            ParsedType* declaredTypes = field->varDecl.declaredTypes;
            if (declaredTypes && v < field->varDecl.varCount) {
                fieldType = declaredTypes + v;
            } else {
                fieldType = &field->varDecl.declaredType;
            }

            const ASTNode* locNode = nameNode ? nameNode : field;

            FisicsSymbol child = {0};
            child.name = name;
            child.file_path = locNode->location.start.file;
            child.start_line = locNode->location.start.line;
            child.start_col = locNode->location.start.column;
            child.end_line = locNode->location.end.line ? locNode->location.end.line : child.start_line;
            child.end_col = locNode->location.end.column ? locNode->location.end.column : child.start_col;
            child.kind = FISICS_SYMBOL_FIELD;
            child.parent_name = sym->name;
            child.parent_kind = parentKind;
            child.is_definition = true;
            if (fieldType) {
                child.return_type = parsed_type_to_string(fieldType);
            }
            symbuf_append(buf, &child);
        }
    }
}

static void append_enum_member_symbols(SymbolBuffer* buf, const Symbol* sym, const ASTNode* def) {
    if (!buf || !sym || !def || def->type != AST_ENUM_DEFINITION) return;
    ASTNode** members = def->enumDef.members;
    for (size_t i = 0; i < def->enumDef.memberCount; ++i) {
        ASTNode* member = members ? members[i] : NULL;
        const char* name = identifier_name(member);
        if (!name || !name[0]) continue;

        const ASTNode* locNode = member ? member : def;
        FisicsSymbol child = {0};
        child.name = name;
        child.file_path = locNode->location.start.file;
        child.start_line = locNode->location.start.line;
        child.start_col = locNode->location.start.column;
        child.end_line = locNode->location.end.line ? locNode->location.end.line : child.start_line;
        child.end_col = locNode->location.end.column ? locNode->location.end.column : child.start_col;
        child.kind = FISICS_SYMBOL_ENUM_MEMBER;
        child.parent_name = sym->name;
        child.parent_kind = FISICS_SYMBOL_ENUM;
        child.is_definition = true;
        symbuf_append(buf, &child);
    }
}

static void append_macro_symbols(SymbolBuffer* buf,
                                 const MacroTable* table,
                                 const char* file_path,
                                 bool include_system_symbols) {
    if (!buf || !table) return;
    for (size_t i = 0; i < table->macroCount; ++i) {
        const MacroDefinition* def = &table->macros[i];
        if (!def || !def->name || !def->name[0]) continue;

        const char* defPath = def->definitionRange.start.file;
        if (!include_system_symbols) {
            if (!defPath || !file_path || strcmp(defPath, file_path) != 0) {
                continue;
            }
        } else if (!defPath || !defPath[0]) {
            continue;
        }

        FisicsSymbol sym = {0};
        sym.name = def->name;
        sym.kind = FISICS_SYMBOL_MACRO;
        sym.is_definition = true;
        sym.is_variadic = def->params.variadic;
        sym.file_path = defPath;
        sym.start_line = def->definitionRange.start.line;
        sym.start_col = def->definitionRange.start.column;
        sym.end_line = def->definitionRange.end.line ? def->definitionRange.end.line : sym.start_line;
        sym.end_col = def->definitionRange.end.column ? def->definitionRange.end.column : sym.start_col;

        if (def->kind == MACRO_FUNCTION && def->params.count > 0) {
            sym.param_count = def->params.count;
            sym.param_names = (const char**)calloc(sym.param_count, sizeof(char*));
            if (sym.param_names) {
                for (size_t p = 0; p < sym.param_count; ++p) {
                    sym.param_names[p] = def->params.names[p];
                }
            }
        }

        symbuf_append(buf, &sym);
    }
}

static void free_symbol_buffer(SymbolBuffer* buf) {
    if (!buf || !buf->items) return;
    for (size_t i = 0; i < buf->count; ++i) {
        free((char*)buf->items[i].return_type);
        if (buf->items[i].param_types) {
            for (size_t p = 0; p < buf->items[i].param_count; ++p) {
                free((char*)buf->items[i].param_types[p]);
            }
            free(buf->items[i].param_types);
        }
        free((char*)buf->items[i].param_names);
    }
    free(buf->items);
    buf->items = NULL;
    buf->count = 0;
    buf->capacity = 0;
}

static void collect_symbol_cb(const Symbol* sym, void* userData) {
    SymbolCollectCtx* ctx = (SymbolCollectCtx*)userData;
    if (!ctx || !ctx->buf || !sym || !sym->name) return;

    const ASTNode* ident = NULL;
    const ASTNode* locNode = NULL;
    if (!ctx->include_system_symbols) {
        if (!sym->definition) return;
        ident = symbol_identifier_node(sym);
        locNode = ident ? ident : sym->definition;
        const char* defPath = locNode->location.start.file;
        if (!defPath || !ctx->file_path || strcmp(defPath, ctx->file_path) != 0) {
            return;
        }
    }

    FisicsSymbol out = {0};
    out.name = sym->name;
    out.kind = map_definition_kind(sym);
    out.is_definition = sym->hasDefinition;
    out.is_variadic = sym->signature.isVariadic;

    if (sym->kind == SYMBOL_FUNCTION) {
        out.return_type = parsed_type_to_string(&sym->type);
        size_t sigCount = sym->signature.paramCount;
        out.param_count = sigCount;
        if (sym->signature.params && sigCount > 0) {
            out.param_types = (const char**)calloc(sigCount, sizeof(char*));
            if (out.param_types) {
                for (size_t i = 0; i < sigCount; ++i) {
                    out.param_types[i] = parsed_type_to_string(&sym->signature.params[i]);
                }
            }
        }
    }

    if (sym->definition) {
        if (!locNode) {
            ident = symbol_identifier_node(sym);
            locNode = ident ? ident : sym->definition;
        }
        out.file_path = locNode->location.start.file;
        out.start_line = locNode->location.start.line;
        out.start_col = locNode->location.start.column;
        out.end_line = locNode->location.end.line ? locNode->location.end.line : out.start_line;
        out.end_col = locNode->location.end.column ? locNode->location.end.column : out.start_col;

        if (sym->kind == SYMBOL_FUNCTION) {
            const ASTNode* def = sym->definition;
            const ASTNode** params = NULL;
            size_t paramCount = 0;
            if (def->type == AST_FUNCTION_DEFINITION) {
                params = (const ASTNode**)def->functionDef.parameters;
                paramCount = def->functionDef.paramCount;
            } else if (def->type == AST_FUNCTION_DECLARATION) {
                params = (const ASTNode**)def->functionDecl.parameters;
                paramCount = def->functionDecl.paramCount;
            }
            if (params && paramCount > 0) {
                size_t sigCount = out.param_count;
                if (sigCount == 0) {
                    sigCount = paramCount;
                    out.param_count = sigCount;
                }
                size_t nameCount = paramCount < sigCount ? paramCount : sigCount;
                out.param_names = (const char**)calloc(sigCount, sizeof(char*));
                if (out.param_names) {
                    for (size_t i = 0; i < nameCount; ++i) {
                        out.param_names[i] = param_decl_name(params[i]);
                    }
                }
            }
        }
    }

    symbuf_append(ctx->buf, &out);

    if (sym->definition) {
        const ASTNode* def = sym->definition;
        if (def->type == AST_STRUCT_DEFINITION || def->type == AST_UNION_DEFINITION) {
            append_struct_field_symbols(ctx->buf, sym, def);
        } else if (def->type == AST_ENUM_DEFINITION) {
            append_enum_member_symbols(ctx->buf, sym, def);
        }
    }
}

static FisicsTokenKind map_token_kind(TokenType t) {
    switch (t) {
        case TOKEN_IDENTIFIER: return FISICS_TOK_IDENTIFIER;
        case TOKEN_NUMBER:
        case TOKEN_FLOAT_LITERAL: return FISICS_TOK_NUMBER;
        case TOKEN_STRING: return FISICS_TOK_STRING;
        case TOKEN_CHAR_LITERAL: return FISICS_TOK_CHAR;
        case TOKEN_LINE_COMMENT:
        case TOKEN_BLOCK_COMMENT: return FISICS_TOK_COMMENT;
        case TOKEN_INCLUDE: case TOKEN_DEFINE: case TOKEN_UNDEF:
        case TOKEN_IFDEF: case TOKEN_IFNDEF: case TOKEN_ENDIF:
        case TOKEN_PRAGMA: case TOKEN_PP_IF: case TOKEN_PP_ELIF: case TOKEN_PP_ELSE:
        case TOKEN_INT: case TOKEN_FLOAT: case TOKEN_CHAR: case TOKEN_DOUBLE: case TOKEN_LONG: case TOKEN_SHORT:
        case TOKEN_SIGNED: case TOKEN_UNSIGNED: case TOKEN_VOID: case TOKEN_BOOL: case TOKEN_ENUM: case TOKEN_UNION:
        case TOKEN_STRUCT: case TOKEN_TYPEDEF: case TOKEN_IF: case TOKEN_ELSE: case TOKEN_WHILE: case TOKEN_FOR:
        case TOKEN_DO: case TOKEN_SWITCH: case TOKEN_CASE: case TOKEN_DEFAULT: case TOKEN_RETURN: case TOKEN_GOTO:
        case TOKEN_BREAK: case TOKEN_CONTINUE: case TOKEN_EXTERN: case TOKEN_STATIC: case TOKEN_AUTO:
        case TOKEN_REGISTER: case TOKEN_THREAD_LOCAL: case TOKEN_CONST: case TOKEN_VOLATILE:
        case TOKEN_RESTRICT: case TOKEN_INLINE: case TOKEN_ATOMIC: case TOKEN_NULL:
        case TOKEN_SIZEOF: case TOKEN_ALIGNOF: case TOKEN_STATIC_ASSERT: case TOKEN_ASM:
            return FISICS_TOK_KEYWORD;
        case TOKEN_LPAREN: case TOKEN_RPAREN: case TOKEN_LBRACE: case TOKEN_RBRACE:
        case TOKEN_LBRACKET: case TOKEN_RBRACKET: case TOKEN_SEMICOLON: case TOKEN_COMMA:
        case TOKEN_COLON: case TOKEN_DOT: case TOKEN_ELLIPSIS: case TOKEN_ARROW:
        case TOKEN_QUESTION:
            return FISICS_TOK_PUNCT;
        default:
            return FISICS_TOK_OPERATOR;
    }
}

void pipeline_collect_top_symbols(ASTNode* root, CompilerContext* ctx) {
    if (!root || root->type != AST_PROGRAM || !ctx) return;
    SymbolBuffer buf = {0};

    ASTNode** statements = root->block.statements;
    for (size_t i = 0; i < root->block.statementCount; ++i) {
        ASTNode* stmt = statements ? statements[i] : NULL;
        if (!stmt) continue;

        const char* name = NULL;
        FisicsSymbolKind kind = FISICS_SYMBOL_UNKNOWN;
        bool isDefinition = true;
        switch (stmt->type) {
            case AST_FUNCTION_DEFINITION:
                {
                    ASTNode* funcName = stmt->functionDef.funcName;
                    name = identifier_name(funcName);
                }
                kind = FISICS_SYMBOL_FUNCTION;
                isDefinition = true;
                break;
            case AST_FUNCTION_DECLARATION:
                {
                    ASTNode* funcName = stmt->functionDecl.funcName;
                    name = identifier_name(funcName);
                }
                kind = FISICS_SYMBOL_FUNCTION;
                isDefinition = false;
                break;
            case AST_STRUCT_DEFINITION:
            case AST_UNION_DEFINITION:
                {
                    ASTNode* structName = stmt->structDef.structName;
                    name = identifier_name(structName);
                }
                kind = (stmt->type == AST_STRUCT_DEFINITION) ? FISICS_SYMBOL_STRUCT : FISICS_SYMBOL_UNION;
                break;
            case AST_ENUM_DEFINITION:
                {
                    ASTNode* enumName = stmt->enumDef.enumName;
                    name = identifier_name(enumName);
                }
                kind = FISICS_SYMBOL_ENUM;
                break;
            case AST_TYPEDEF:
                {
                    ASTNode* alias = stmt->typedefStmt.alias;
                    name = identifier_name(alias);
                }
                kind = FISICS_SYMBOL_TYPEDEF;
                break;
            default:
                break;
        }
        if (!name || name[0] == '\0') {
            continue;
        }

        FisicsSymbol sym = {0};
        sym.name = name;
        sym.file_path = stmt->location.start.file;
        sym.start_line = stmt->location.start.line;
        sym.start_col = stmt->location.start.column;
        sym.end_line = stmt->location.end.line ? stmt->location.end.line : sym.start_line;
        sym.end_col = stmt->location.end.column ? stmt->location.end.column : sym.start_col;
        sym.kind = kind;
        sym.is_definition = isDefinition;
        symbuf_append(&buf, &sym);
    }

    if (buf.count > 0) {
        cc_set_symbols(ctx, buf.items, buf.count);
    } else {
        cc_clear_symbols(ctx);
    }
    free(buf.items);
}

void pipeline_collect_semantic_symbols(const SemanticModel* model,
                                       CompilerContext* ctx,
                                       const char* file_path,
                                       bool include_system_symbols,
                                       const MacroTable* macros) {
    if (!model || !ctx) return;
    SymbolBuffer buf = {0};
    SymbolCollectCtx collectCtx = {
        .buf = &buf,
        .file_path = file_path,
        .include_system_symbols = include_system_symbols
    };
    semanticModelForEachGlobal(model, collect_symbol_cb, &collectCtx);
    append_macro_symbols(&buf, macros, file_path, include_system_symbols);

    if (buf.count > 0) {
        cc_set_symbols(ctx, buf.items, buf.count);
    } else {
        cc_clear_symbols(ctx);
    }
    free_symbol_buffer(&buf);
}

void pipeline_capture_token_spans(CompilerContext* ctx, const PPTokenBuffer* preprocessed) {
    if (!ctx || !preprocessed) return;
    cc_clear_token_spans(ctx);
    for (size_t i = 0; i < preprocessed->count; ++i) {
        const Token* tok = &preprocessed->tokens[i];
        FisicsTokenSpan span;
        span.line = tok->location.start.line;
        span.column = tok->location.start.column;
        int len = tok->location.end.column - tok->location.start.column;
        span.length = len > 0 ? len : 1;
        span.kind = map_token_kind(tok->type);
        cc_append_token_span(ctx, &span);
    }
}
