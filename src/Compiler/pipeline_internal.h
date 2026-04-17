// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "Compiler/compiler_context.h"
#include "AST/ast_node.h"
#include "Preprocessor/macro_table.h"
#include "Preprocessor/macro_expander.h"
#include "Syntax/semantic_model.h"

void pipeline_collect_top_symbols(ASTNode* root, CompilerContext* ctx);
void pipeline_collect_semantic_symbols(const SemanticModel* model,
                                       CompilerContext* ctx,
                                       const char* file_path,
                                       bool include_system_symbols,
                                       const MacroTable* macros);
void pipeline_capture_token_spans(CompilerContext* ctx, const PPTokenBuffer* preprocessed);
bool pipeline_path_exists(const char* path);
char* pipeline_read_file_all(const char* path, size_t* outLen);
bool pipeline_run_external_preprocessor(const char* cmd,
                                        const char* args,
                                        const char* inputPath,
                                        const char* const* includePaths,
                                        size_t includePathCount,
                                        char** outBuffer,
                                        size_t* outLength);
