// SPDX-License-Identifier: Apache-2.0

#include "Compiler/pipeline.h"
#include "Compiler/pipeline_internal.h"

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
#include "Syntax/syntax_errors.h"
#include "Utils/utils.h"
#include "Compiler/diagnostics.h"
#include "Utils/profiler.h"

static const char* diag_kind_str(DiagKind kind) {
    switch (kind) {
        case DIAG_ERROR: return "error";
        case DIAG_WARNING: return "warning";
        default: return "note";
    }
}

static bool pipeline_path_list_push(const char*** items,
                                    size_t* count,
                                    size_t* capacity,
                                    const char* value) {
    if (!items || !count || !capacity || !value || !value[0]) return false;
    if (*count == *capacity) {
        size_t newCap = (*capacity == 0) ? 8 : (*capacity * 2);
        const char** grown = (const char**)realloc((void*)(*items), newCap * sizeof(const char*));
        if (!grown) return false;
        *items = grown;
        *capacity = newCap;
    }
    (*items)[(*count)++] = value;
    return true;
}

static void pipeline_print_diagnostics(CompilerContext* ctx) {
    if (!ctx) return;
    size_t count = 0;
    const FisicsDiagnostic* diags = compiler_diagnostics_data(ctx, &count);
    for (size_t i = 0; i < count; ++i) {
        const FisicsDiagnostic* d = &diags[i];
        if (d->code == CDIAG_GENERIC) {
            continue;
        }
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
        case TOKEN_THREAD_LOCAL: return "TOKEN_THREAD_LOCAL";
        case TOKEN_CONST: return "TOKEN_CONST";
        case TOKEN_VOLATILE: return "TOKEN_VOLATILE";
        case TOKEN_RESTRICT: return "TOKEN_RESTRICT";
        case TOKEN_INLINE: return "TOKEN_INLINE";
        case TOKEN_ATOMIC: return "TOKEN_ATOMIC";
        case TOKEN_COMPLEX: return "TOKEN_COMPLEX";
        case TOKEN_IMAGINARY: return "TOKEN_IMAGINARY";
        case TOKEN_NULL: return "TOKEN_NULL";
        case TOKEN_SIZEOF: return "TOKEN_SIZEOF";
        case TOKEN_ALIGNOF: return "TOKEN_ALIGNOF";
        case TOKEN_STATIC_ASSERT: return "TOKEN_STATIC_ASSERT";
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
                                           PreprocessMode preprocessMode,
                                           const char* externalPreprocessCmd,
                                           const char* externalPreprocessArgs,
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
    initErrorList(ctx);
    lexer_set_diag_context(ctx);

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
                           preprocessMode,
                           includePaths,
                           includePathCount,
                           externalPreprocessCmd,
                           externalPreprocessArgs,
                           macroDefines,
                           macroDefineCount)) {
        fprintf(stderr, "Error: failed to initialize preprocessor\n");
        goto cleanup;
    }
    log_layout_state(ctx, "after preprocessor_init");

    const IncludeFile* rootFile = NULL;
    ProfilerScope scope = profiler_begin("load_root");
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
    profiler_end(scope);
    if (!rootFile) {
        fprintf(stderr, "Error: failed to load source file %s\n", file_path ? file_path : "<null>");
        goto cleanup;
    }
    log_layout_state(ctx, "after root load");

    if (debugProgress) fprintf(stderr, "[pipeline] lexing %s\n", rootFile->path);
    Lexer lexer;
    initLexer(&lexer, rootFile->contents, rootFile->path, preprocessor.enableTrigraphs);

    scope = profiler_begin("lex");
    if (!token_buffer_fill_from_lexer(&tokenBuffer, &lexer)) {
        destroyLexer(&lexer);
        profiler_end(scope);
        fprintf(stderr, "Error: failed to lex tokens into buffer\n");
        goto cleanup;
    }
    if (lexer.fatalErrorCount > 0) {
        destroyLexer(&lexer);
        profiler_end(scope);
        goto cleanup;
    }
    profiler_end(scope);
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
    scope = profiler_begin("preprocess");
    bool ppOk = preprocessor_run(&preprocessor, &tokenBuffer, &preprocessed);
    profiler_end(scope);
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

    pipeline_capture_token_spans(ctx, &preprocessed);

    const char* statsEnv = getenv("FISICS_PP_STATS");
    if (statsEnv && statsEnv[0] != '\0' && statsEnv[0] != '0') {
        fprintf(stderr, "pp_stats: raw_tokens=%zu preprocessed_tokens=%zu\n",
                tokenBuffer.count,
                preprocessed.count);
    }

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

    scope = profiler_begin("parse");
    root = parse(&parser);
    profiler_end(scope);
    if (!root) {
        reportErrors();
        if (outSemanticErrors) *outSemanticErrors = getErrorCount();
        freeErrorList();
        preprocessor_destroy(&preprocessor);
        token_buffer_destroy(&parserTokens);
        lexer_set_diag_context(NULL);
        return false;
    }
    size_t diagErrors = compiler_diagnostics_error_count(ctx);
    size_t parserErrors = compiler_diagnostics_parser_error_count(ctx);
    if (getErrorCount() > 0 || parser.fatalParseErrors > 0 || diagErrors > parserErrors) {
        reportErrors();
        if (outSemanticErrors) *outSemanticErrors = getErrorCount();
        freeErrorList();
        preprocessor_destroy(&preprocessor);
        token_buffer_destroy(&parserTokens);
        lexer_set_diag_context(NULL);
        return false;
    }
    cc_set_translation_unit(ctx, root);
    pipeline_collect_top_symbols(root, ctx);
    log_layout_state(ctx, "after parse");

    if (dumpAst) {
        printf(" AST Output:\n");
        printAST(root, 0);
    }

    if (dumpSemantic) {
        printf("\n Semantic Analysis:\n");
    }

    MacroTable* macroSnapshot = macro_table_clone(preprocessor_get_macro_table(&preprocessor));
    scope = profiler_begin("semantic");
    semanticModel = analyzeSemanticsBuildModel(root,
                                               ctx,
                                               false,
                                               macroSnapshot,
                                               true);
    profiler_end(scope);
    if (!semanticModel) {
        goto cleanup;
    }
    pipeline_collect_semantic_symbols(semanticModel, ctx, file_path, includeSystemSymbols, macroSnapshot);
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
    lexer_set_diag_context(NULL);
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

    lexer_set_diag_context(NULL);
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

    const char* const* effectiveIncludePaths = includePaths;
    size_t effectiveIncludeCount = includePathCount;
    const char** mergedIncludePaths = NULL;
    size_t mergedIncludeCount = 0;
    size_t mergedIncludeCap = 0;
    char** defaultParsed = NULL;
    size_t defaultParsedCount = 0;
    char** sysParsed = NULL;
    size_t sysParsedCount = 0;

    for (size_t i = 0; i < includePathCount; ++i) {
        pipeline_path_list_push(&mergedIncludePaths,
                                &mergedIncludeCount,
                                &mergedIncludeCap,
                                includePaths[i]);
    }

    if (compiler_collect_include_paths(DEFAULT_INCLUDE_PATHS, &defaultParsed, &defaultParsedCount)) {
        for (size_t i = 0; i < defaultParsedCount; ++i) {
            pipeline_path_list_push(&mergedIncludePaths,
                                    &mergedIncludeCount,
                                    &mergedIncludeCap,
                                    defaultParsed[i]);
        }
    }

    const char* sysEnv = getenv("SYSTEM_INCLUDE_PATHS");
    if (sysEnv && sysEnv[0] &&
        compiler_collect_include_paths(sysEnv, &sysParsed, &sysParsedCount)) {
        for (size_t i = 0; i < sysParsedCount; ++i) {
            pipeline_path_list_push(&mergedIncludePaths,
                                    &mergedIncludeCount,
                                    &mergedIncludeCap,
                                    sysParsed[i]);
        }
    }

    const char* commonIncludeRoots[] = {
        "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include",
        "/Library/Developer/CommandLineTools/usr/include",
        "/opt/homebrew/include",
        "/opt/homebrew/include/SDL2",
        "/usr/local/include",
        "/usr/local/include/SDL2",
        "/usr/include"
    };
    for (size_t i = 0; i < sizeof(commonIncludeRoots) / sizeof(commonIncludeRoots[0]); ++i) {
        if (pipeline_path_exists(commonIncludeRoots[i])) {
            pipeline_path_list_push(&mergedIncludePaths,
                                    &mergedIncludeCount,
                                    &mergedIncludeCap,
                                    commonIncludeRoots[i]);
        }
    }

    effectiveIncludePaths = mergedIncludePaths;
    effectiveIncludeCount = mergedIncludeCount;

    bool ok = compiler_run_frontend_internal(ctx,
                                             file_path,
                                             source,
                                             length,
                                             preservePPNodes,
                                             enableTrigraphs,
                                             PREPROCESS_INTERNAL,
                                             NULL,
                                             NULL,
                                             effectiveIncludePaths,
                                             effectiveIncludeCount,
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

    if (defaultParsed) {
        compiler_free_include_paths(defaultParsed, defaultParsedCount);
    }
    if (sysParsed) {
        compiler_free_include_paths(sysParsed, sysParsedCount);
    }
    free((void*)mergedIncludePaths);
    return ok;
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
    cc_set_language_dialect(ctx, options->dialect);
    cc_set_compat_features(ctx, options->compatFeatures);
    cc_set_overlay_features(ctx, options->overlayFeatures);
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
    char* externalSource = NULL;
    char* loadedInputSource = NULL;
    char* forcedIncludeSource = NULL;
    size_t externalLength = 0;
    const char* sourceForFrontend = NULL;
    size_t sourceLength = 0;

    if (options->preprocessMode == PREPROCESS_EXTERNAL) {
        const char* cmd = options->externalPreprocessCmd;
        if (!cmd || !cmd[0]) {
            cmd = "clang";
        }
        const char* args = options->externalPreprocessArgs;
        ProfilerScope scope = profiler_begin("external_preprocess");
        bool ok = pipeline_run_external_preprocessor(cmd,
                                                     args,
                                                     options->inputPath,
                                                     options->includePaths,
                                                     options->includePathCount,
                                                     &externalSource,
                                                     &externalLength);
        profiler_end(scope);
        if (!ok) {
            fprintf(stderr, "Error: external preprocessing failed (cmd=%s)\n", cmd);
            goto cleanup;
        }
        sourceForFrontend = externalSource;
        sourceLength = externalLength;
        const char* statsEnv = getenv("FISICS_PP_STATS");
        if (statsEnv && statsEnv[0] != '\0' && statsEnv[0] != '0') {
            fprintf(stderr, "pp_stats: external_preprocess bytes=%zu\n", externalLength);
        }
    }

    if (options->forcedIncludes && options->forcedIncludeCount > 0) {
        const char* baseSource = sourceForFrontend;
        size_t baseLength = sourceLength;
        if (!baseSource) {
            loadedInputSource = pipeline_read_file_all(options->inputPath, &baseLength);
            if (!loadedInputSource) {
                fprintf(stderr, "Error: failed to read source file %s for forced includes\n",
                        options->inputPath);
                goto cleanup;
            }
            baseSource = loadedInputSource;
        }

        size_t alloc = baseLength + 1 + strlen(options->inputPath) + 64;
        for (size_t i = 0; i < options->forcedIncludeCount; ++i) {
            const char* inc = options->forcedIncludes[i];
            if (!inc) continue;
            alloc += strlen(inc) + 32;
        }
        forcedIncludeSource = (char*)malloc(alloc);
        if (!forcedIncludeSource) {
            fprintf(stderr, "OOM: forced include wrapper\n");
            goto cleanup;
        }

        size_t used = 0;
        for (size_t i = 0; i < options->forcedIncludeCount; ++i) {
            const char* inc = options->forcedIncludes[i];
            if (!inc || !inc[0]) continue;
            int wrote = snprintf(forcedIncludeSource + used,
                                 alloc - used,
                                 "#include \"%s\"\n",
                                 inc);
            if (wrote < 0 || (size_t)wrote >= alloc - used) {
                fprintf(stderr, "Error: forced include wrapper overflow\n");
                goto cleanup;
            }
            used += (size_t)wrote;
        }
        int lineWrote = snprintf(forcedIncludeSource + used,
                                 alloc - used,
                                 "#line 1 \"%s\"\n",
                                 options->inputPath);
        if (lineWrote < 0 || (size_t)lineWrote >= alloc - used) {
            fprintf(stderr, "Error: forced include wrapper overflow\n");
            goto cleanup;
        }
        used += (size_t)lineWrote;
        memcpy(forcedIncludeSource + used, baseSource, baseLength);
        used += baseLength;
        forcedIncludeSource[used] = '\0';

        sourceForFrontend = forcedIncludeSource;
        sourceLength = used;
    }

    if (!compiler_run_frontend_internal(ctx,
                                        options->inputPath,
                                        sourceForFrontend,
                                        sourceLength,
                                        options->preservePPNodes,
                                        options->enableTrigraphs,
                                        options->preprocessMode,
                                        options->externalPreprocessCmd,
                                        options->externalPreprocessArgs,
                                        options->includePaths,
                                        options->includePathCount,
                                        options->macroDefines,
                                        options->macroDefineCount,
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
        printf("\nLLVM Code Generation:\n");
        if (semaErrors == 0) {
            ProfilerScope codegenScope = profiler_begin("codegen");
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
            profiler_end(codegenScope);
        } else {
            printf("Skipping LLVM code generation due to semantic errors.\n");
        }
    } else {
        printf("\nLLVM Code Generation:\n");
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
    if (outResult) {
        *outResult = result;
        memset(&result, 0, sizeof(result));
    } else if (status != 0) {
        compile_result_destroy(&result);
    }
    free(externalSource);
    free(loadedInputSource);
    free(forcedIncludeSource);

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
