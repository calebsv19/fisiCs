#include "Compiler/pipeline.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "AST/ast_node.h"
#include "AST/ast_printer.h"
#include "CodeGen/code_gen.h"
#include "Compiler/compiler_context.h"
#include "Lexer/lexer.h"
#include "Lexer/token_buffer.h"
#include "Parser/Helpers/designated_init.h"
#include "Parser/Helpers/parser_helpers.h"
#include "Parser/parser.h"
#include "Preprocessor/preprocessor.h"
#include "Syntax/semantic_model.h"
#include "Syntax/semantic_model_printer.h"
#include "Syntax/semantic_pass.h"
#include "Utils/utils.h"

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

static const char* identifier_name(const ASTNode* node) {
    if (!node) return NULL;
    if (node->type == AST_IDENTIFIER) {
        return node->valueNode.value;
    }
    return NULL;
}

static void collect_top_symbols(ASTNode* root, CompilerContext* ctx) {
    if (!root || root->type != AST_PROGRAM || !ctx) return;
    SymbolBuffer buf = {0};

    for (size_t i = 0; i < root->block.statementCount; ++i) {
        ASTNode* stmt = root->block.statements ? root->block.statements[i] : NULL;
        if (!stmt) continue;

        const char* name = NULL;
        switch (stmt->type) {
            case AST_FUNCTION_DEFINITION:
                name = identifier_name(stmt->functionDef.funcName);
                break;
            case AST_FUNCTION_DECLARATION:
                name = identifier_name(stmt->functionDecl.funcName);
                break;
            case AST_STRUCT_DEFINITION:
            case AST_UNION_DEFINITION:
                name = identifier_name(stmt->structDef.structName);
                break;
            case AST_ENUM_DEFINITION:
                name = identifier_name(stmt->enumDef.enumName);
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
        symbuf_append(&buf, &sym);
    }

    if (buf.count > 0) {
        cc_set_symbols(ctx, buf.items, buf.count);
    } else {
        cc_clear_symbols(ctx);
    }
    free(buf.items);
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
        case TOKEN_NULL: case TOKEN_SIZEOF: case TOKEN_TRUE: case TOKEN_FALSE: case TOKEN_ASM:
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

static bool compiler_run_frontend_internal(CompilerContext* ctx,
                                           const char* file_path,
                                           const char* source,
                                           size_t length,
                                           bool preservePPNodes,
                                           const char* const* includePaths,
                                           size_t includePathCount,
                                           const char* const* macroDefines,
                                           size_t macroDefineCount,
                                           bool lenientIncludes,
                                           bool dumpAst,
                                           bool dumpSemantic,
                                           ASTNode** outAst,
                                           SemanticModel** outModel,
                                           size_t* outSemanticErrors) {
    TokenBuffer tokenBuffer;
    token_buffer_init(&tokenBuffer);

    TokenBuffer parserTokens = {0};
    PPTokenBuffer preprocessed = {0};
    Preprocessor preprocessor;
    memset(&preprocessor, 0, sizeof(preprocessor));
    ASTNode* root = NULL;
    SemanticModel* semanticModel = NULL;

    if (!preprocessor_init(&preprocessor,
                           ctx,
                           preservePPNodes,
                           lenientIncludes,
                           includePaths,
                           includePathCount,
                           macroDefines,
                           macroDefineCount)) {
        fprintf(stderr, "Error: failed to initialize preprocessor\n");
        goto cleanup;
    }

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
                                         NULL);
    } else {
        rootFile = include_resolver_load(preprocessor_get_resolver(&preprocessor),
                                         NULL,
                                         file_path,
                                         false,
                                         NULL);
    }
    if (!rootFile) {
        fprintf(stderr, "Error: failed to load source file %s\n", file_path ? file_path : "<null>");
        goto cleanup;
    }

    Lexer lexer;
    initLexer(&lexer, rootFile->contents, rootFile->path);

    if (!token_buffer_fill_from_lexer(&tokenBuffer, &lexer)) {
        fprintf(stderr, "Error: failed to lex tokens into buffer\n");
        goto cleanup;
    }

    const char* debugPP = getenv("DEBUG_PP_COUNT");
    if (debugPP && debugPP[0] != '\0' && debugPP[0] != '0') {
        fprintf(stderr, "DEBUG: raw tokens=%zu\n", tokenBuffer.count);
        for (size_t i = 0; i < tokenBuffer.count && i < 16; ++i) {
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

    if (!preprocessor_run(&preprocessor, &tokenBuffer, &preprocessed)) {
        fprintf(stderr, "Error: preprocessing failed\n");
        goto cleanup;
    }

    capture_token_spans(ctx, &preprocessed);

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

    Parser parser;
    initParser(&parser, &parserTokens, PARSER_MODE_PRATT, ctx, preservePPNodes);

    root = parse(&parser);
    cc_set_translation_unit(ctx, root);
    collect_top_symbols(root, ctx);

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
                           const char* const* includePaths,
                           size_t includePathCount,
                           const char* const* macroDefines,
                           size_t macroDefineCount,
                           bool lenientIncludes,
                           bool dumpAst,
                           bool dumpSemantic,
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
                                          includePaths,
                                          includePathCount,
                                          macroDefines,
                                          macroDefineCount,
                                          lenientIncludes,
                                          dumpAst,
                                          dumpSemantic,
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
    result.compilerCtx = ctx;
    cc_seed_builtins(ctx);

    if (options->targetTriple) {
        cc_set_target_triple(ctx, options->targetTriple);
    }
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
                                        options->includePaths,
                                        options->includePathCount,
                                        NULL,
                                        0,
                                        false, // lenientIncludes disabled for CLI path
                                        options->dumpAst,
                                        options->dumpSemantic,
                                        &ast,
                                        &model,
                                        &semaErrors)) {
        goto cleanup;
    }

    result.ast = ast;
    result.semanticModel = model;
    result.semanticErrors = semaErrors;

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
