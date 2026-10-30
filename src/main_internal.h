// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>
#include <stddef.h>

#include <llvm-c/Core.h>

#include "AST/ast_node.h"
#include "Compiler/pipeline.h"
#include "Syntax/semantic_model.h"

typedef struct {
    char** items;
    size_t count;
    size_t capacity;
} StringList;

bool main_physical_options_valid(FisicsOverlayFeatures,const char*,size_t,size_t,bool,PreprocessMode);
void string_list_free(StringList* list);
bool string_list_push(StringList* list, const char* value);

typedef struct {
    bool compileOnly;
    bool preservePPNodes;
    const char* depsJsonPath;
    const char* buildGraphJsonPath;
    const char* semanticMetadataJsonPath;
    const char* unitsMetadataJsonPath;
    const char* diagsJsonPath;
    const char* diagsPackPath;
    const char* targetTriple;
    const char* dataLayout;
    const char* outputName;
    const char* linkerPath;
    bool dumpAst;
    bool dumpSemantic;
    bool dumpIR;
    bool dumpTokens;
    bool enableTrigraphs;
    bool warnIgnoredInterop;
    bool errorIgnoredInterop;
    PreprocessMode preprocessMode;
    const char* externalPreprocessCmd;
    const char* externalPreprocessArgs;
    CCDialect dialect;
    CCCompatFeatures compatFeatures;
    FisicsOverlayFeatures overlayFeatures;
    const char* physicalPolicyPath;
    int enableCodegen;
    const StringList* includePaths;
    const StringList* macroDefines;
    const StringList* forcedIncludes;
    const StringList* inputCFiles;
    const StringList* inputOFiles;
    const StringList* linkerSearchPaths;
    const StringList* linkerLibs;
    const StringList* linkerFrameworks;
} MainDriverConfig;

typedef struct {
    char* name;
    SymbolKind kind;
    ParsedType type;
    FunctionSignature signature;
    char* filePath;
    int line;
    int column;
    bool virtualSpelling;
    bool hasDefinition;
    bool isTentative;
    StorageClass storage;
    size_t nextInBucket; /* Ledger index, never a pointer into the movable array. */
} CrossTUVarDef;

#define CROSS_TU_INDEX_NONE ((size_t)-1)

typedef struct {
    size_t head;
    size_t tail;
} CrossTUIndexBucket;

typedef struct {
    CrossTUVarDef* items;
    size_t count;
    size_t capacity;
    CrossTUIndexBucket* buckets;
    size_t bucketCount;
} CrossTUVarDefList;

/* reserve may rehash existing entries; append cannot allocate or fail. */
bool main_cross_tu_index_reserve(CrossTUVarDefList* defs);
void main_cross_tu_index_append(CrossTUVarDefList* defs, size_t entry);
size_t main_cross_tu_index_first(const CrossTUVarDefList* defs,
                               const char* name, SymbolKind kind);
void main_cross_tu_index_free(CrossTUVarDefList* defs);

typedef struct {
    bool found;
    char* symbolName;
    SymbolKind kind;
    char* filePath;
    int line;
    int column;
    char* previousFilePath;
    int previousLine;
    int previousColumn;
} CrossTUTypeConflict;

char* main_create_temp_object_path(const char* baseName);
char* main_derive_diag_json_path(const char* basePath, size_t index, size_t total);
char* main_derive_diag_pack_path(const char* basePath, size_t index, size_t total);
bool main_write_link_stage_diag_json(const char* outPath,
                                     int exitCode,
                                     const char* linkerName,
                                     const char* outputName,
                                     size_t inputCount);
void main_cross_tu_var_defs_free(CrossTUVarDefList* defs);
void main_cross_tu_type_conflict_clear(CrossTUTypeConflict* conflict);
bool main_collect_cross_tu_virtual_type_conflict(const SemanticModel* model,
                                                 const CompilerContext* compilerCtx,
                                                 CrossTUVarDefList* defs,
                                                 CrossTUTypeConflict* conflict);
bool main_write_semantic_conflict_diag_json(const char* outPath,
                                            const CrossTUTypeConflict* conflict);
void main_print_semantic_conflict_text(const CrossTUTypeConflict* conflict);

int main_run_driver_mode(const MainDriverConfig* config);

CompileOptions main_make_compile_options(const MainDriverConfig* config,
    const char* inputPath,const char* depsJsonPath);
#include "Extensions/Units/units_coverage_policy.h"
int main_run_coverage_mode(const MainDriverConfig* config,const char* function,
    const char* jsonPath,const char* textPath,FisicsCoveragePolicy policy);

#include "Extensions/PhysicalSemantics/physical_coverage.h"
int main_run_physical_coverage(const MainDriverConfig *,const char *,const char *,PhysicalCoveragePolicy);
