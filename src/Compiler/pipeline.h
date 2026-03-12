#pragma once

#include <stdbool.h>
#include <stddef.h>

#include <llvm-c/Core.h>

#include "Compiler/preprocess_mode.h"
#include "Compiler/compiler_context.h"

struct CompilerContext;
struct SemanticModel;
struct ASTNode;
typedef struct CodegenContext CodegenContext;

typedef struct {
    const char* inputPath;
    bool preservePPNodes;
    const char* depsJsonPath;
    const char* targetTriple;
    const char* dataLayout;
    const char* const* includePaths;
    size_t includePathCount;
    const char* const* macroDefines;
    size_t macroDefineCount;
    PreprocessMode preprocessMode;
    const char* externalPreprocessCmd;
    const char* externalPreprocessArgs;
    CCDialect dialect;
    bool enableExtensions;
    bool dumpAst;
    bool dumpSemantic;
    bool dumpIR;
    bool dumpTokens;
    bool enableCodegen;
    bool enableTrigraphs;
    bool warnIgnoredInterop;
    bool errorIgnoredInterop;
} CompileOptions;

typedef struct {
    struct CompilerContext* compilerCtx;
    struct ASTNode* ast;
    struct SemanticModel* semanticModel;
    CodegenContext* codegenCtx;
    LLVMModuleRef module;
    size_t semanticErrors;
} CompileResult;

// Reusable frontend entry point: runs lex -> preprocess -> parse -> sema on an
// optional in-memory buffer. Results (diags/tokens/symbols/TU) live on ctx.
bool compiler_run_frontend(struct CompilerContext* ctx,
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
                           struct ASTNode** outAst,
                           struct SemanticModel** outModel,
                           size_t* outSemanticErrors);

// Returns 0 on successful frontend completion (semantic errors may still exist).
// When outResult is provided, it is populated on both success and failure so
// callers can consume diagnostics from compilerCtx before destroy.
int compile_translation_unit(const CompileOptions* options, CompileResult* outResult);
void compile_result_destroy(CompileResult* result);

bool compiler_collect_include_paths(const char* pathList, char*** pathsOut, size_t* countOut);
void compiler_free_include_paths(char** paths, size_t count);
