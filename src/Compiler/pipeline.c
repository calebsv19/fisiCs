#include "Compiler/pipeline.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

    TokenBuffer tokenBuffer;
    token_buffer_init(&tokenBuffer);

    TokenBuffer parserTokens = {0};
    PPTokenBuffer preprocessed = {0};
    Preprocessor preprocessor;
    memset(&preprocessor, 0, sizeof(preprocessor));

    CompilerContext* ctx = cc_create();
    if (!ctx) {
        fprintf(stderr, "OOM: CompilerContext\n");
        goto cleanup;
    }
    result.compilerCtx = ctx;
    cc_seed_builtins(ctx);

    if (options->targetTriple) {
        cc_set_target_triple(ctx, options->targetTriple);
    }
    if (options->dataLayout) {
        cc_set_data_layout(ctx, options->dataLayout);
    }

    if (!preprocessor_init(&preprocessor,
                           options->preservePPNodes,
                           options->includePaths,
                           options->includePathCount)) {
        fprintf(stderr, "Error: failed to initialize preprocessor\n");
        goto cleanup;
    }

    const IncludeFile* rootFile = include_resolver_load(preprocessor_get_resolver(&preprocessor),
                                                        NULL,
                                                        options->inputPath,
                                                        false);
    if (!rootFile) {
        fprintf(stderr, "Error: failed to load source file %s\n", options->inputPath);
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
    initParser(&parser, &parserTokens, PARSER_MODE_PRATT, ctx, options->preservePPNodes);

    ASTNode* root = parse(&parser);
    result.ast = root;

    if (options->dumpAst) {
        printf(" AST Output:\n");
        printAST(root, 0);
    }

    if (options->dumpSemantic) {
        printf("\n Semantic Analysis:\n");
    }

    MacroTable* macroSnapshot = macro_table_clone(preprocessor_get_macro_table(&preprocessor));
    SemanticModel* semanticModel = analyzeSemanticsBuildModel(root,
                                                              ctx,
                                                              false,
                                                              macroSnapshot,
                                                              true);
    if (!semanticModel) {
        goto cleanup;
    }

    result.semanticModel = semanticModel;
    size_t semanticErrors = semanticModelGetErrorCount(semanticModel);
    result.semanticErrors = semanticErrors;

    if (options->dumpSemantic) {
        printf("\n Semantic Model Dump:\n");
        semanticModelDump(semanticModel);
    }

    if (options->enableCodegen) {
        printf("\n️ LLVM Code Generation:\n");
        if (semanticErrors == 0) {
            CodegenContext* codegenCtx = codegen_context_create("compiler_module", semanticModel);
            if (!codegenCtx) {
                fprintf(stderr, "Error: Failed to initialize LLVM code generation context\n");
            } else {
                LLVMValueRef resultValue = codegen_generate(codegenCtx, root);
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
        const IncludeGraph* graph = preprocessor_get_include_graph(&preprocessor);
        if (!include_graph_write_json(graph, options->depsJsonPath)) {
            fprintf(stderr, "Warning: failed to write deps JSON to %s\n", options->depsJsonPath);
        }
    }

    status = 0;

cleanup:
    pp_token_buffer_destroy(&preprocessed);
    token_buffer_destroy(&tokenBuffer);
    preprocessor_destroy(&preprocessor);
    token_buffer_destroy(&parserTokens);

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
