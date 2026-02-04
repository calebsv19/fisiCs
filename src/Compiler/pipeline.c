#include "Compiler/pipeline.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "AST/ast_node.h"
#include "AST/ast_printer.h"
#include "CodeGen/code_gen.h"
#include "Compiler/compiler_context.h"
#include "Syntax/target_layout.h"
#include "Lexer/lexer.h"
#include "Lexer/token_buffer.h"
#include "Parser/Helpers/designated_init.h"
#include "Parser/Helpers/parser_helpers.h"
#include "Parser/parser.h"
#include "Preprocessor/preprocessor.h"
#include "Syntax/semantic_model.h"
#include "Syntax/semantic_model_printer.h"
#include "Syntax/semantic_pass.h"
#include "Parser/Helpers/parsed_type_format.h"
#include "Utils/utils.h"
#include "Compiler/diagnostics.h"

typedef struct {
    FisicsSymbol* items;
    size_t count;
    size_t capacity;
} SymbolBuffer;

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

static const char* diag_kind_str(DiagKind kind) {
    switch (kind) {
        case DIAG_ERROR: return "error";
        case DIAG_WARNING: return "warning";
        default: return "note";
    }
}

static void pipeline_print_diagnostics(CompilerContext* ctx) {
    if (!ctx) return;
    size_t count = 0;
    const FisicsDiagnostic* diags = compiler_diagnostics_data(ctx, &count);
    for (size_t i = 0; i < count; ++i) {
        const FisicsDiagnostic* d = &diags[i];
        const char* path = d->file_path ? d->file_path : "<unknown>";
        fprintf(stderr, "%s:%d:%d: %s: %s\n",
                path,
                d->line,
                d->column,
                diag_kind_str(d->kind),
                d->message ? d->message : "<unknown>");
        if (d->hint && d->hint[0]) {
            fprintf(stderr, "  hint: %s\n", d->hint);
        }
    }
}

static const char* identifier_name(const ASTNode* node) {
    if (!node) return NULL;
    if (node->type == AST_IDENTIFIER) {
        return node->valueNode.value;
    }
    return NULL;
}

static const char* token_type_name(TokenType type) {
    switch (type) {
        case TOKEN_INT: return "TOKEN_INT";
        case TOKEN_FLOAT: return "TOKEN_FLOAT";
        case TOKEN_CHAR: return "TOKEN_CHAR";
        case TOKEN_DOUBLE: return "TOKEN_DOUBLE";
        case TOKEN_LONG: return "TOKEN_LONG";
        case TOKEN_SHORT: return "TOKEN_SHORT";
        case TOKEN_SIGNED: return "TOKEN_SIGNED";
        case TOKEN_UNSIGNED: return "TOKEN_UNSIGNED";
        case TOKEN_VOID: return "TOKEN_VOID";
        case TOKEN_BOOL: return "TOKEN_BOOL";
        case TOKEN_ENUM: return "TOKEN_ENUM";
        case TOKEN_UNION: return "TOKEN_UNION";
        case TOKEN_STRUCT: return "TOKEN_STRUCT";
        case TOKEN_TYPEDEF: return "TOKEN_TYPEDEF";
        case TOKEN_IF: return "TOKEN_IF";
        case TOKEN_ELSE: return "TOKEN_ELSE";
        case TOKEN_WHILE: return "TOKEN_WHILE";
        case TOKEN_FOR: return "TOKEN_FOR";
        case TOKEN_DO: return "TOKEN_DO";
        case TOKEN_SWITCH: return "TOKEN_SWITCH";
        case TOKEN_CASE: return "TOKEN_CASE";
        case TOKEN_DEFAULT: return "TOKEN_DEFAULT";
        case TOKEN_RETURN: return "TOKEN_RETURN";
        case TOKEN_GOTO: return "TOKEN_GOTO";
        case TOKEN_BREAK: return "TOKEN_BREAK";
        case TOKEN_CONTINUE: return "TOKEN_CONTINUE";
        case TOKEN_EXTERN: return "TOKEN_EXTERN";
        case TOKEN_STATIC: return "TOKEN_STATIC";
        case TOKEN_AUTO: return "TOKEN_AUTO";
        case TOKEN_REGISTER: return "TOKEN_REGISTER";
        case TOKEN_CONST: return "TOKEN_CONST";
        case TOKEN_VOLATILE: return "TOKEN_VOLATILE";
        case TOKEN_RESTRICT: return "TOKEN_RESTRICT";
        case TOKEN_INLINE: return "TOKEN_INLINE";
        case TOKEN_COMPLEX: return "TOKEN_COMPLEX";
        case TOKEN_IMAGINARY: return "TOKEN_IMAGINARY";
        case TOKEN_NULL: return "TOKEN_NULL";
        case TOKEN_SIZEOF: return "TOKEN_SIZEOF";
        case TOKEN_ALIGNOF: return "TOKEN_ALIGNOF";
        case TOKEN_IDENTIFIER: return "TOKEN_IDENTIFIER";
        case TOKEN_INCLUDE: return "TOKEN_INCLUDE";
        case TOKEN_INCLUDE_NEXT: return "TOKEN_INCLUDE_NEXT";
        case TOKEN_DEFINE: return "TOKEN_DEFINE";
        case TOKEN_UNDEF: return "TOKEN_UNDEF";
        case TOKEN_IFDEF: return "TOKEN_IFDEF";
        case TOKEN_IFNDEF: return "TOKEN_IFNDEF";
        case TOKEN_ENDIF: return "TOKEN_ENDIF";
        case TOKEN_PRAGMA: return "TOKEN_PRAGMA";
        case TOKEN_ONCE: return "TOKEN_ONCE";
        case TOKEN_PREPROCESSOR_OTHER: return "TOKEN_PREPROCESSOR_OTHER";
        case TOKEN_PP_IF: return "TOKEN_PP_IF";
        case TOKEN_PP_ELIF: return "TOKEN_PP_ELIF";
        case TOKEN_PP_ELSE: return "TOKEN_PP_ELSE";
        case TOKEN_NUMBER: return "TOKEN_NUMBER";
        case TOKEN_FLOAT_LITERAL: return "TOKEN_FLOAT_LITERAL";
        case TOKEN_STRING: return "TOKEN_STRING";
        case TOKEN_CHAR_LITERAL: return "TOKEN_CHAR_LITERAL";
        case TOKEN_ASSIGN: return "TOKEN_ASSIGN";
        case TOKEN_PLUS_ASSIGN: return "TOKEN_PLUS_ASSIGN";
        case TOKEN_MINUS_ASSIGN: return "TOKEN_MINUS_ASSIGN";
        case TOKEN_MULT_ASSIGN: return "TOKEN_MULT_ASSIGN";
        case TOKEN_DIV_ASSIGN: return "TOKEN_DIV_ASSIGN";
        case TOKEN_MOD_ASSIGN: return "TOKEN_MOD_ASSIGN";
        case TOKEN_PLUS: return "TOKEN_PLUS";
        case TOKEN_MINUS: return "TOKEN_MINUS";
        case TOKEN_ASTERISK: return "TOKEN_ASTERISK";
        case TOKEN_DIVIDE: return "TOKEN_DIVIDE";
        case TOKEN_MODULO: return "TOKEN_MODULO";
        case TOKEN_INCREMENT: return "TOKEN_INCREMENT";
        case TOKEN_DECREMENT: return "TOKEN_DECREMENT";
        case TOKEN_EQUAL: return "TOKEN_EQUAL";
        case TOKEN_NOT_EQUAL: return "TOKEN_NOT_EQUAL";
        case TOKEN_LESS: return "TOKEN_LESS";
        case TOKEN_LESS_EQUAL: return "TOKEN_LESS_EQUAL";
        case TOKEN_GREATER: return "TOKEN_GREATER";
        case TOKEN_GREATER_EQUAL: return "TOKEN_GREATER_EQUAL";
        case TOKEN_LOGICAL_AND: return "TOKEN_LOGICAL_AND";
        case TOKEN_LOGICAL_OR: return "TOKEN_LOGICAL_OR";
        case TOKEN_LOGICAL_NOT: return "TOKEN_LOGICAL_NOT";
        case TOKEN_BITWISE_AND: return "TOKEN_BITWISE_AND";
        case TOKEN_BITWISE_OR: return "TOKEN_BITWISE_OR";
        case TOKEN_BITWISE_XOR: return "TOKEN_BITWISE_XOR";
        case TOKEN_BITWISE_NOT: return "TOKEN_BITWISE_NOT";
        case TOKEN_LEFT_SHIFT: return "TOKEN_LEFT_SHIFT";
        case TOKEN_RIGHT_SHIFT: return "TOKEN_RIGHT_SHIFT";
        case TOKEN_BITWISE_AND_ASSIGN: return "TOKEN_BITWISE_AND_ASSIGN";
        case TOKEN_BITWISE_OR_ASSIGN: return "TOKEN_BITWISE_OR_ASSIGN";
        case TOKEN_BITWISE_XOR_ASSIGN: return "TOKEN_BITWISE_XOR_ASSIGN";
        case TOKEN_LEFT_SHIFT_ASSIGN: return "TOKEN_LEFT_SHIFT_ASSIGN";
        case TOKEN_RIGHT_SHIFT_ASSIGN: return "TOKEN_RIGHT_SHIFT_ASSIGN";
        case TOKEN_SEMICOLON: return "TOKEN_SEMICOLON";
        case TOKEN_COLON: return "TOKEN_COLON";
        case TOKEN_COMMA: return "TOKEN_COMMA";
        case TOKEN_DOT: return "TOKEN_DOT";
        case TOKEN_ELLIPSIS: return "TOKEN_ELLIPSIS";
        case TOKEN_QUESTION: return "TOKEN_QUESTION";
        case TOKEN_HASH: return "TOKEN_HASH";
        case TOKEN_DOUBLE_HASH: return "TOKEN_DOUBLE_HASH";
        case TOKEN_LPAREN: return "TOKEN_LPAREN";
        case TOKEN_RPAREN: return "TOKEN_RPAREN";
        case TOKEN_LBRACE: return "TOKEN_LBRACE";
        case TOKEN_RBRACE: return "TOKEN_RBRACE";
        case TOKEN_LBRACKET: return "TOKEN_LBRACKET";
        case TOKEN_RBRACKET: return "TOKEN_RBRACKET";
        case TOKEN_ARROW: return "TOKEN_ARROW";
        case TOKEN_LINE_COMMENT: return "TOKEN_LINE_COMMENT";
        case TOKEN_BLOCK_COMMENT: return "TOKEN_BLOCK_COMMENT";
        case TOKEN_ASM: return "TOKEN_ASM";
        case TOKEN_EOF: return "TOKEN_EOF";
        case TOKEN_UNKNOWN: return "TOKEN_UNKNOWN";
        default: return "TOKEN_UNKNOWN";
    }
}

static char* escape_token_text(const char* text) {
    if (!text) return strdup("<null>");
    size_t len = 0;
    for (const unsigned char* p = (const unsigned char*)text; *p; ++p) {
        switch (*p) {
            case '\n':
            case '\r':
            case '\t':
            case '\\':
            case '"':
                len += 2;
                break;
            default:
                if (*p < 32 || *p == 127) {
                    len += 4;
                } else {
                    len += 1;
                }
                break;
        }
    }
    char* out = (char*)malloc(len + 1);
    if (!out) return NULL;
    char* w = out;
    for (const unsigned char* p = (const unsigned char*)text; *p; ++p) {
        switch (*p) {
            case '\n': *w++ = '\\'; *w++ = 'n'; break;
            case '\r': *w++ = '\\'; *w++ = 'r'; break;
            case '\t': *w++ = '\\'; *w++ = 't'; break;
            case '\\': *w++ = '\\'; *w++ = '\\'; break;
            case '"': *w++ = '\\'; *w++ = '"'; break;
            default:
                if (*p < 32 || *p == 127) {
                    static const char hex[] = "0123456789ABCDEF";
                    *w++ = '\\';
                    *w++ = 'x';
                    *w++ = hex[(*p >> 4) & 0xF];
                    *w++ = hex[*p & 0xF];
                } else {
                    *w++ = (char)*p;
                }
                break;
        }
    }
    *w = '\0';
    return out;
}

static void collect_top_symbols(ASTNode* root, CompilerContext* ctx) {
    if (!root || root->type != AST_PROGRAM || !ctx) return;
    SymbolBuffer buf = {0};

    for (size_t i = 0; i < root->block.statementCount; ++i) {
        ASTNode* stmt = root->block.statements ? root->block.statements[i] : NULL;
        if (!stmt) continue;

        const char* name = NULL;
        FisicsSymbolKind kind = FISICS_SYMBOL_UNKNOWN;
        bool isDefinition = true;
        switch (stmt->type) {
            case AST_FUNCTION_DEFINITION:
                name = identifier_name(stmt->functionDef.funcName);
                kind = FISICS_SYMBOL_FUNCTION;
                isDefinition = true;
                break;
            case AST_FUNCTION_DECLARATION:
                name = identifier_name(stmt->functionDecl.funcName);
                kind = FISICS_SYMBOL_FUNCTION;
                isDefinition = false;
                break;
            case AST_STRUCT_DEFINITION:
            case AST_UNION_DEFINITION:
                name = identifier_name(stmt->structDef.structName);
                kind = (stmt->type == AST_STRUCT_DEFINITION) ? FISICS_SYMBOL_STRUCT : FISICS_SYMBOL_UNION;
                break;
            case AST_ENUM_DEFINITION:
                name = identifier_name(stmt->enumDef.enumName);
                kind = FISICS_SYMBOL_ENUM;
                break;
            case AST_TYPEDEF:
                name = identifier_name(stmt->typedefStmt.alias);
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
                for (size_t i = 0; i < def->varDecl.varCount; ++i) {
                    const char* name = identifier_name(def->varDecl.varNames[i]);
                    if (name && sym->name && strcmp(name, sym->name) == 0) {
                        return def->varDecl.varNames[i];
                    }
                }
                return def->varDecl.varNames[0];
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
    return identifier_name(paramDecl->varDecl.varNames[0]);
}

static void append_struct_field_symbols(SymbolBuffer* buf, const Symbol* sym, const ASTNode* def) {
    if (!buf || !sym || !def) return;
    if (def->type != AST_STRUCT_DEFINITION && def->type != AST_UNION_DEFINITION) return;

    const FisicsSymbolKind parentKind = (def->type == AST_UNION_DEFINITION)
        ? FISICS_SYMBOL_UNION
        : FISICS_SYMBOL_STRUCT;

    for (size_t i = 0; i < def->structDef.fieldCount; ++i) {
        ASTNode* field = def->structDef.fields ? def->structDef.fields[i] : NULL;
        if (!field || field->type != AST_VARIABLE_DECLARATION) continue;
        size_t varCount = field->varDecl.varCount;
        for (size_t v = 0; v < varCount; ++v) {
            ASTNode* nameNode = field->varDecl.varNames ? field->varDecl.varNames[v] : NULL;
            const char* name = identifier_name(nameNode);
            if (!name || !name[0]) continue;

            const ParsedType* fieldType = NULL;
            if (field->varDecl.declaredTypes && v < field->varDecl.varCount) {
                fieldType = &field->varDecl.declaredTypes[v];
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
    for (size_t i = 0; i < def->enumDef.memberCount; ++i) {
        ASTNode* member = def->enumDef.members ? def->enumDef.members[i] : NULL;
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

typedef struct {
    SymbolBuffer* buf;
    const char* file_path;
    bool include_system_symbols;
} SymbolCollectCtx;

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

static void collect_semantic_symbols(const SemanticModel* model,
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
        case TOKEN_REGISTER: case TOKEN_CONST: case TOKEN_VOLATILE: case TOKEN_RESTRICT: case TOKEN_INLINE:
        case TOKEN_NULL: case TOKEN_SIZEOF: case TOKEN_ASM:
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

static void capture_token_spans(CompilerContext* ctx, const PPTokenBuffer* preprocessed) {
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

static bool append_include_path(char*** paths, size_t* count, size_t* capacity, const char* path) {
    if (!paths || !count || !capacity || !path) return false;
    if (*count == *capacity) {
        size_t newCap = (*capacity == 0) ? 4 : (*capacity * 2);
        char** grown = realloc(*paths, newCap * sizeof(char*));
        if (!grown) return false;
        *paths = grown;
        *capacity = newCap;
    }
    (*paths)[*count] = strdup(path);
    if (!(*paths)[*count]) return false;
    (*count)++;
    return true;
}

bool compiler_collect_include_paths(const char* pathList, char*** pathsOut, size_t* countOut) {
    if (pathsOut) *pathsOut = NULL;
    if (countOut) *countOut = 0;
    if (!pathList || pathList[0] == '\0') return true;

    char* copy = strdup(pathList);
    if (!copy) return false;

    char** paths = NULL;
    size_t count = 0;
    size_t capacity = 0;

    char* saveptr = NULL;
    char* tok = strtok_r(copy, ":", &saveptr);
    while (tok) {
        if (tok[0] != '\0') {
            if (!append_include_path(&paths, &count, &capacity, tok)) {
                compiler_free_include_paths(paths, count);
                free(copy);
                return false;
            }
        }
        tok = strtok_r(NULL, ":", &saveptr);
    }

    free(copy);
    if (pathsOut) *pathsOut = paths;
    if (countOut) *countOut = count;
    return true;
}

void compiler_free_include_paths(char** paths, size_t count) {
    if (!paths) return;
    for (size_t i = 0; i < count; ++i) {
        free(paths[i]);
    }
    free(paths);
}

static bool debug_layout_enabled(void) {
    static int initialized = 0;
    static bool enabled = false;
    if (!initialized) {
        const char* env = getenv("FISICS_DEBUG_LAYOUT");
        enabled = (env && env[0] && env[0] != '0');
        initialized = 1;
    }
    return enabled;
}

static void log_layout_state(CompilerContext* ctx, const char* stage) {
    if (!ctx || !debug_layout_enabled()) return;
    LOG_WARN("codegen", "[%s] dataLayout=%p canaries=%llx/%llx",
             stage,
             (void*)cc_get_data_layout(ctx),
             (unsigned long long)ctx->dl_canary_front,
             (unsigned long long)ctx->dl_canary_back);
}

static bool compiler_run_frontend_internal(CompilerContext* ctx,
                                           const char* file_path,
                                           const char* source,
                                           size_t length,
                                           bool preservePPNodes,
                                           bool enableTrigraphs,
                                           const char* const* includePaths,
                                           size_t includePathCount,
                                           const char* const* macroDefines,
                                           size_t macroDefineCount,
                                           bool lenientIncludes,
                                           bool includeSystemSymbols,
                                           bool dumpAst,
                                           bool dumpSemantic,
                                           bool dumpTokens,
                                           ASTNode** outAst,
                                           SemanticModel** outModel,
                                           size_t* outSemanticErrors) {
    (void)ctx; // ctx is valid for logging below
    log_layout_state(ctx, "frontend entry");
    const char* progressEnv = getenv("FISICS_DEBUG_PROGRESS");
    bool debugProgress = progressEnv && progressEnv[0] && progressEnv[0] != '0';

    TokenBuffer tokenBuffer;
    token_buffer_init(&tokenBuffer);

    TokenBuffer parserTokens = {0};
    PPTokenBuffer preprocessed = {0};
    Preprocessor preprocessor;
    memset(&preprocessor, 0, sizeof(preprocessor));
    ASTNode* root = NULL;
    SemanticModel* semanticModel = NULL;

    log_layout_state(ctx, "frontend entry");

    if (!preprocessor_init(&preprocessor,
                           ctx,
                           preservePPNodes,
                           lenientIncludes,
                           enableTrigraphs,
                           includePaths,
                           includePathCount,
                           macroDefines,
                           macroDefineCount)) {
        fprintf(stderr, "Error: failed to initialize preprocessor\n");
        goto cleanup;
    }
    log_layout_state(ctx, "after preprocessor_init");

    const IncludeFile* rootFile = NULL;
    if (source) {
        char* owned = (char*)malloc(length + 1);
        if (!owned) goto cleanup;
        memcpy(owned, source, length);
        owned[length] = '\0';
        if (!include_resolver_set_root_buffer(preprocessor_get_resolver(&preprocessor),
                                              file_path ? file_path : "<buffer>",
                                              owned,
                                              0)) {
            free(owned);
            goto cleanup;
        }
        rootFile = include_resolver_load(preprocessor_get_resolver(&preprocessor),
                                         NULL,
                                         file_path ? file_path : "<buffer>",
                                         false,
                                         false,
                                         NULL,
                                         NULL);
    } else {
        rootFile = include_resolver_load(preprocessor_get_resolver(&preprocessor),
                                         NULL,
                                         file_path,
                                         false,
                                         false,
                                         NULL,
                                         NULL);
    }
    if (!rootFile) {
        fprintf(stderr, "Error: failed to load source file %s\n", file_path ? file_path : "<null>");
        goto cleanup;
    }
    log_layout_state(ctx, "after root load");

    if (debugProgress) fprintf(stderr, "[pipeline] lexing %s\n", rootFile->path);
    Lexer lexer;
    initLexer(&lexer, rootFile->contents, rootFile->path, preprocessor.enableTrigraphs);

    if (!token_buffer_fill_from_lexer(&tokenBuffer, &lexer)) {
        destroyLexer(&lexer);
        fprintf(stderr, "Error: failed to lex tokens into buffer\n");
        goto cleanup;
    }
    destroyLexer(&lexer);
    log_layout_state(ctx, "after lex");

    const char* debugPP = getenv("DEBUG_PP_COUNT");
    if (debugPP && debugPP[0] != '\0' && debugPP[0] != '0') {
        fprintf(stderr, "DEBUG: raw tokens=%zu\n", tokenBuffer.count);
        for (size_t i = 0; i < tokenBuffer.count && i < 128; ++i) {
            const char* val = tokenBuffer.tokens[i].value ? tokenBuffer.tokens[i].value : "<null>";
            fprintf(stderr, "  RAW[%zu]: type=%d value=%s loc=%s:%d:%d\n",
                    i,
                    tokenBuffer.tokens[i].type,
                    val,
                    tokenBuffer.tokens[i].location.start.file ? tokenBuffer.tokens[i].location.start.file : "<null>",
                    tokenBuffer.tokens[i].location.start.line,
                    tokenBuffer.tokens[i].location.start.column);
        }
    }

    if (debugProgress) fprintf(stderr, "[pipeline] preprocessing\n");
    bool ppOk = preprocessor_run(&preprocessor, &tokenBuffer, &preprocessed);
    if (!ppOk) {
        fprintf(stderr, "Error: preprocessing failed\n");
        pipeline_print_diagnostics(ctx);
        if (!lenientIncludes || preprocessed.count == 0) {
            goto cleanup;
        }
        if (debugProgress) {
            fprintf(stderr, "[pipeline] preprocessing failed; continuing with partial tokens\n");
        }
    }
    log_layout_state(ctx, "after preprocessor_run");

    capture_token_spans(ctx, &preprocessed);

    if (dumpTokens) {
        printf("Token Stream:\n");
        for (size_t i = 0; i < preprocessed.count; ++i) {
            const Token* tok = &preprocessed.tokens[i];
            const char* file = tok->location.start.file ? tok->location.start.file : "<unknown>";
            const char* typeName = token_type_name(tok->type);
            const char* rawText = tok->value ? tok->value : "<null>";
            char* escaped = escape_token_text(rawText);
            const char* text = escaped ? escaped : "<oom>";
            printf("%zu %s %s %s:%d:%d\n",
                   i,
                   typeName,
                   text,
                   file,
                   tok->location.start.line,
                   tok->location.start.column);
            free(escaped);
        }
        printf("\n");
    }

    if (debugPP && debugPP[0] != '\0' && debugPP[0] != '0') {
        fprintf(stderr, "DEBUG: preprocessed tokens=%zu\n", preprocessed.count);
        for (size_t i = 0; i < preprocessed.count; ++i) {
            const char* val = preprocessed.tokens[i].value ? preprocessed.tokens[i].value : "<null>";
            fprintf(stderr, "  PP[%zu]: type=%d value=%s loc=%s:%d:%d\n",
                    i,
                    preprocessed.tokens[i].type,
                    val,
                    preprocessed.tokens[i].location.start.file ? preprocessed.tokens[i].location.start.file : "<null>",
                    preprocessed.tokens[i].location.start.line,
                    preprocessed.tokens[i].location.start.column);
        }
    }

    token_buffer_destroy(&tokenBuffer);
    parserTokens.tokens = preprocessed.tokens;
    parserTokens.count = preprocessed.count;
    parserTokens.capacity = preprocessed.capacity;
    preprocessed.tokens = NULL;
    preprocessed.count = 0;
    preprocessed.capacity = 0;

    cc_set_include_graph(ctx, preprocessor_get_include_graph(&preprocessor));
    log_layout_state(ctx, "after include graph copy");

    if (debugProgress) fprintf(stderr, "[pipeline] parsing\n");
    Parser parser;
    initParser(&parser, &parserTokens, PARSER_MODE_PRATT, ctx, preservePPNodes);

    root = parse(&parser);
    cc_set_translation_unit(ctx, root);
    collect_top_symbols(root, ctx);
    log_layout_state(ctx, "after parse");

    if (dumpAst) {
        printf(" AST Output:\n");
        printAST(root, 0);
    }

    if (dumpSemantic) {
        printf("\n Semantic Analysis:\n");
    }

    MacroTable* macroSnapshot = macro_table_clone(preprocessor_get_macro_table(&preprocessor));
    semanticModel = analyzeSemanticsBuildModel(root,
                                               ctx,
                                               false,
                                               macroSnapshot,
                                               true);
    if (!semanticModel) {
        goto cleanup;
    }
    collect_semantic_symbols(semanticModel, ctx, file_path, includeSystemSymbols, macroSnapshot);
    log_layout_state(ctx, "after semantic");

    size_t semanticErrors = semanticModelGetErrorCount(semanticModel);

    if (dumpSemantic) {
        printf("\n Semantic Model Dump:\n");
        semanticModelDump(semanticModel);
    }

    if (outAst) *outAst = root;
    if (outModel) *outModel = semanticModel;
    if (outSemanticErrors) *outSemanticErrors = semanticErrors;

    preprocessor_destroy(&preprocessor);
    token_buffer_destroy(&parserTokens);
    return true;

cleanup:
    pp_token_buffer_destroy(&preprocessed);
    token_buffer_destroy(&tokenBuffer);
    preprocessor_destroy(&preprocessor);
    token_buffer_destroy(&parserTokens);

    if (semanticModel) {
        semanticModelDestroy(semanticModel);
    }
    (void)root;

    return false;
}

bool compiler_run_frontend(CompilerContext* ctx,
                           const char* file_path,
                           const char* source,
                           size_t length,
                           bool preservePPNodes,
                           bool enableTrigraphs,
                           const char* const* includePaths,
                           size_t includePathCount,
                           const char* const* macroDefines,
                           size_t macroDefineCount,
                           bool lenientIncludes,
                           bool includeSystemSymbols,
                           bool dumpAst,
                           bool dumpSemantic,
                           bool dumpTokens,
                           ASTNode** outAst,
                           SemanticModel** outModel,
                           size_t* outSemanticErrors) {
    if (!ctx || !file_path) {
        return false;
    }
    cc_seed_builtins(ctx);
    return compiler_run_frontend_internal(ctx,
                                          file_path,
                                          source,
                                          length,
                                          preservePPNodes,
                                          enableTrigraphs,
                                          includePaths,
                                          includePathCount,
                                          macroDefines,
                                          macroDefineCount,
                                          lenientIncludes,
                                          includeSystemSymbols,
                                          dumpAst,
                                          dumpSemantic,
                                          dumpTokens,
                                          outAst,
                                          outModel,
                                          outSemanticErrors);
}

int compile_translation_unit(const CompileOptions* options, CompileResult* outResult) {
    if (outResult) {
        memset(outResult, 0, sizeof(*outResult));
    }
    if (!options || !options->inputPath) {
        fprintf(stderr, "Error: compile_translation_unit requires an inputPath\n");
        return 1;
    }

    int status = 1;
    CompileResult result = {0};

    CompilerContext* ctx = cc_create();
    if (!ctx) {
        fprintf(stderr, "OOM: CompilerContext\n");
        return 1;
    }
    if (debug_layout_enabled()) {
        LOG_WARN("codegen", "ctx dataLayout initially %p canaries=%llx/%llx",
                 (void*)cc_get_data_layout(ctx),
                 (unsigned long long)ctx->dl_canary_front,
                 (unsigned long long)ctx->dl_canary_back);
    }
    result.compilerCtx = ctx;
    cc_seed_builtins(ctx);

    if (options->targetTriple) {
        cc_set_target_triple(ctx, options->targetTriple);
    }
    const TargetLayout* tl = tl_from_triple(options->targetTriple);
    cc_set_target_layout(ctx, tl);
    cc_set_interop_diag(ctx,
                        options->warnIgnoredInterop,
                        options->errorIgnoredInterop);
    if (options->dataLayout) {
        cc_set_data_layout(ctx, options->dataLayout);
    }

    ASTNode* ast = NULL;
    SemanticModel* model = NULL;
    size_t semaErrors = 0;

    if (!compiler_run_frontend_internal(ctx,
                                        options->inputPath,
                                        NULL,
                                        0,
                                        options->preservePPNodes,
                                        options->enableTrigraphs,
                                        options->includePaths,
                                        options->includePathCount,
                                        NULL,
                                        0,
                                        false, // lenientIncludes disabled for CLI path
                                        true,
                                        options->dumpAst,
                                        options->dumpSemantic,
                                        options->dumpTokens,
                                        &ast,
                                        &model,
                                        &semaErrors)) {
        pipeline_print_diagnostics(ctx);
        goto cleanup;
    }
    if (debug_layout_enabled()) {
        LOG_WARN("codegen", "ctx dataLayout after frontend %p canaries=%llx/%llx",
                 (void*)cc_get_data_layout(ctx),
                 (unsigned long long)ctx->dl_canary_front,
                 (unsigned long long)ctx->dl_canary_back);
    }

    result.ast = ast;
    result.semanticModel = model;
    result.semanticErrors = semaErrors;

    /* If no data layout was provided but the context somehow has one, log and clear it
     * to avoid propagating corrupted strings into LLVM. */
    if (!options->dataLayout && cc_get_data_layout(ctx)) {
        const char* strayLayout = cc_get_data_layout(ctx);
        size_t len = 0;
        unsigned char b0 = 0, b1 = 0;
        if ((uintptr_t)strayLayout > 0x1000 && strayLayout) {
            len = strlen(strayLayout);
            b0 = (unsigned char)strayLayout[0];
            b1 = (len > 1) ? (unsigned char)strayLayout[1] : 0;
        } else {
            len = (size_t)(uintptr_t)strayLayout;
        }
        if (debug_layout_enabled()) {
            LOG_WARN("codegen", "Unexpected data layout present (len=%zu, bytes=%02x %02x); clearing before codegen. Canaries=%llx/%llx",
                     len, b0, b1,
                     (unsigned long long)ctx->dl_canary_front,
                     (unsigned long long)ctx->dl_canary_back);
        }
        cc_set_data_layout(ctx, NULL);
    }

    if (options->enableCodegen) {
        printf("\n️ LLVM Code Generation:\n");
        if (semaErrors == 0) {
            CodegenContext* codegenCtx = codegen_context_create("compiler_module", model);
            if (!codegenCtx) {
                fprintf(stderr, "Error: Failed to initialize LLVM code generation context\n");
            } else {
                LLVMValueRef resultValue = codegen_generate(codegenCtx, ast);
                (void)resultValue;
                if (options->dumpIR) {
                    LLVMDumpModule(codegen_get_module(codegenCtx));
                }
                LLVMValueRef nullGlobal = LLVMGetNamedGlobal(codegen_get_module(codegenCtx), "NULL");
                if (nullGlobal) {
                    LLVMSetLinkage(nullGlobal, LLVMInternalLinkage);
                }
                LLVMValueRef trueGlobal = LLVMGetNamedGlobal(codegen_get_module(codegenCtx), "true");
                if (trueGlobal) {
                    LLVMSetLinkage(trueGlobal, LLVMInternalLinkage);
                }
                LLVMValueRef falseGlobal = LLVMGetNamedGlobal(codegen_get_module(codegenCtx), "false");
                if (falseGlobal) {
                    LLVMSetLinkage(falseGlobal, LLVMInternalLinkage);
                }
                result.codegenCtx = codegenCtx;
                result.module = codegen_get_module(codegenCtx);
            }
        } else {
            printf("Skipping LLVM code generation due to semantic errors.\n");
        }
    } else {
        printf("\n️ LLVM Code Generation:\n");
        printf("Skipping LLVM code generation (disabled via configuration).\n");
    }

    if (options->depsJsonPath && options->depsJsonPath[0] != '\0') {
        const IncludeGraph* graph = cc_get_include_graph(ctx);
        if (graph && !include_graph_write_json(graph, options->depsJsonPath)) {
            fprintf(stderr, "Warning: failed to write deps JSON to %s\n", options->depsJsonPath);
        }
    }

    status = 0;

cleanup:
    if (status != 0) {
        compile_result_destroy(&result);
    } else if (outResult) {
        *outResult = result;
    }

    return status;
}

void compile_result_destroy(CompileResult* result) {
    if (!result) return;

    if (result->codegenCtx) {
        codegen_context_destroy(result->codegenCtx);
        result->codegenCtx = NULL;
        result->module = NULL;
    }
    if (result->semanticModel) {
        semanticModelDestroy(result->semanticModel);
        result->semanticModel = NULL;
    }
    if (result->compilerCtx) {
        cc_destroy(result->compilerCtx);
        result->compilerCtx = NULL;
    }
    result->ast = NULL;
    result->semanticErrors = 0;
}
