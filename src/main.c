#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Lexer/lexer.h"
#include "Lexer/token_buffer.h"
#include "Parser/parser.h"
#include "Parser/Helpers/designated_init.h"
#include "Parser/Helpers/parser_helpers.h"

#include "Syntax/semantic_pass.h"
#include "Syntax/semantic_model.h"
#include "Syntax/semantic_model_printer.h"

#include "AST/ast_printer.h"

#include "Utils/utils.h"

#include "Compiler/compiler_context.h"

#include "CodeGen/code_gen.h"
#include "Preprocessor/preprocessor.h"

// === Feature Toggles ===
#define ENABLE_LEXER_OUTPUT      0
#define ENABLE_AST_PRINT         1
#define ENABLE_SYNTAX_CHECK      1
#define ENABLE_CODEGEN           1

int main(int argc, char **argv) {
    const char *filename = NULL;
    bool preservePPNodes = false;
    const char* depsJsonPath = NULL;
    const char* targetTriple = NULL;
    const char* dataLayout = NULL;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--preserve-pp") == 0) {
            preservePPNodes = true;
        } else if (strcmp(argv[i], "--emit-deps-json") == 0 && i + 1 < argc) {
            depsJsonPath = argv[++i];
        } else if (strcmp(argv[i], "--target") == 0 && i + 1 < argc) {
            targetTriple = argv[++i];
        } else if (strcmp(argv[i], "--data-layout") == 0 && i + 1 < argc) {
            dataLayout = argv[++i];
        } else if (argv[i][0] != '-' && !filename) {
            filename = argv[i];
        }
    }
    if (!filename) filename = "include/test.txt";

    int enableCodegen = ENABLE_CODEGEN;
    const char* disableCodegenEnv = getenv("DISABLE_CODEGEN");
    if (disableCodegenEnv && disableCodegenEnv[0] != '\0' && disableCodegenEnv[0] != '0') {
        enableCodegen = 0;
    }

    CompilerContext* ctx = cc_create();
    if (!ctx) { fprintf(stderr, "OOM: CompilerContext\n"); return 1; }
    cc_seed_builtins(ctx);
    if (targetTriple) {
        cc_set_target_triple(ctx, targetTriple);
    }
    if (dataLayout) {
        cc_set_data_layout(ctx, dataLayout);
    }

    const char* preserveEnv = getenv("PRESERVE_PP_NODES");
    if (preserveEnv && preserveEnv[0] != '\0' && preserveEnv[0] != '0') {
        preservePPNodes = true;
    }

    const char* depsEnv = getenv("EMIT_DEPS_JSON");
    if (!depsJsonPath && depsEnv && depsEnv[0] != '\0') {
        depsJsonPath = depsEnv;
    }

    size_t includePathCount = 0;
    char* includePathCopy = NULL;
    char** includePaths = NULL;
    if (DEFAULT_INCLUDE_PATHS[0] != '\0') {
        includePathCopy = strdup(DEFAULT_INCLUDE_PATHS);
        if (!includePathCopy) { fprintf(stderr, "OOM: include paths\n"); return 1; }
        size_t segments = 1;
        for (const char* p = includePathCopy; *p; ++p) {
            if (*p == ':') segments++;
        }
        includePaths = calloc(segments, sizeof(char*));
        if (!includePaths) { fprintf(stderr, "OOM: include path array\n"); free(includePathCopy); return 1; }
        char* saveptr = NULL;
        char* tok = strtok_r(includePathCopy, ":", &saveptr);
        while (tok) {
            includePaths[includePathCount++] = strdup(tok);
            tok = strtok_r(NULL, ":", &saveptr);
        }
    }

    Preprocessor preprocessor;
    if (!preprocessor_init(&preprocessor,
                           preservePPNodes,
                           (const char* const*)includePaths,
                           includePathCount)) {
        fprintf(stderr, "Error: failed to initialize preprocessor\n");
        if (includePaths) {
            for (size_t i = 0; i < includePathCount; ++i) {
                free(includePaths[i]);
            }
            free(includePaths);
        }
        free(includePathCopy);
        cc_destroy(ctx);
        return 1;
    }
    if (includePaths) {
        for (size_t i = 0; i < includePathCount; ++i) {
            free(includePaths[i]);
        }
        free(includePaths);
    }
    free(includePathCopy);

    const IncludeFile* rootFile = include_resolver_load(preprocessor_get_resolver(&preprocessor),
                                                        NULL,
                                                        filename,
                                                        false);
    if (!rootFile) {
        fprintf(stderr, "Error: failed to load source file %s\n", filename);
        preprocessor_destroy(&preprocessor);
        cc_destroy(ctx);
        return 1;
    }

    // === Lexing Phase ===
    Lexer lexer;
    initLexer(&lexer, rootFile->contents, rootFile->path);

    TokenBuffer tokenBuffer;
    token_buffer_init(&tokenBuffer);
    if (!token_buffer_fill_from_lexer(&tokenBuffer, &lexer)) {
        fprintf(stderr, "Error: failed to lex tokens into buffer\n");
        preprocessor_destroy(&preprocessor);
        cc_destroy(ctx);
        return 1;
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

    PPTokenBuffer preprocessed = {0};
    if (!preprocessor_run(&preprocessor, &tokenBuffer, &preprocessed)) {
        fprintf(stderr, "Error: preprocessing failed\n");
        preprocessor_destroy(&preprocessor);
        token_buffer_destroy(&tokenBuffer);
        cc_destroy(ctx);
        return 1;
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
    TokenBuffer parserTokens = {
        .tokens = preprocessed.tokens,
        .count = preprocessed.count,
        .capacity = preprocessed.capacity
    };
    preprocessed.tokens = NULL;
    preprocessed.count = 0;
    preprocessed.capacity = 0;

    // Snapshot include graph for downstream tooling
    cc_set_include_graph(ctx, preprocessor_get_include_graph(&preprocessor));



    // === Parsing Phase ===
    Parser parser;
    initParser(&parser, &parserTokens, PARSER_MODE_PRATT, ctx, preservePPNodes);

    ASTNode *root = parse(&parser);


#if ENABLE_AST_PRINT
    printf(" AST Output:\n");
    printAST(root, 0);
#endif


#if ENABLE_SYNTAX_CHECK
    printf("\n Semantic Analysis:\n");
#endif
    MacroTable* macroSnapshot = macro_table_clone(preprocessor_get_macro_table(&preprocessor));
    SemanticModel* semanticModel = analyzeSemanticsBuildModel(root, ctx, false,
                                                             macroSnapshot, true);
    if (!semanticModel) {
        token_buffer_destroy(&parserTokens);
        preprocessor_destroy(&preprocessor);
        cc_destroy(ctx);
        return 1;
    }

    size_t semanticErrors = semanticModelGetErrorCount(semanticModel);

#if ENABLE_SYNTAX_CHECK
    printf("\n Semantic Model Dump:\n");
    semanticModelDump(semanticModel);
#endif


#if ENABLE_CODEGEN
    printf("\n️ LLVM Code Generation:\n");
    if (!enableCodegen) {
        printf("Skipping LLVM code generation (disabled via environment).\n");
    } else if (semanticErrors == 0) {
        CodegenContext* codegenCtx = codegen_context_create("compiler_module", semanticModel);
        if (!codegenCtx) {
            fprintf(stderr, "Error: Failed to initialize LLVM code generation context\n");
        } else {
            LLVMValueRef result = codegen_generate(codegenCtx, root);
            (void)result;
            LLVMDumpModule(codegen_get_module(codegenCtx));
            codegen_context_destroy(codegenCtx);
        }
    } else {
        printf("Skipping LLVM code generation due to semantic errors.\n");
    }
#endif


    token_buffer_destroy(&parserTokens);
    semanticModelDestroy(semanticModel);
    if (depsJsonPath && depsJsonPath[0] != '\0') {
        const IncludeGraph* graph = preprocessor_get_include_graph(&preprocessor);
        if (!include_graph_write_json(graph, depsJsonPath)) {
            fprintf(stderr, "Warning: failed to write deps JSON to %s\n", depsJsonPath);
        }
    }
    preprocessor_destroy(&preprocessor);
    cc_destroy(ctx);
    return 0;
}
