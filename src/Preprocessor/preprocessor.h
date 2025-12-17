#ifndef PREPROCESSOR_DRIVER_H
#define PREPROCESSOR_DRIVER_H

#include <stdbool.h>

#include "Lexer/token_buffer.h"
#include "Preprocessor/macro_table.h"
#include "Preprocessor/macro_expander.h"
#include "Preprocessor/pp_expr.h"
#include "Preprocessor/include_resolver.h"
#include "Compiler/compiler_context.h"

struct PPIncludeFrame;

typedef struct {
    MacroTable* table;
    MacroExpander expander;
    PPExprEvaluator exprEval;
    IncludeResolver* resolver;
    IncludeGraph includeGraph;
    // Logical location tracking for #line and built-ins
    char* baseFile;
    char* logicalFile;
    int lineOffset;
    int counter;
    char dateString[16];
    char timeString[16];
    char** logicalFilePool;
    size_t logicalFileCount;
    size_t logicalFileCap;
    // Include processing stack (non-owning path pointers held by resolver)
    struct {
        struct PPIncludeFrame* frames;
        size_t depth;
        size_t capacity;
    } includeStack;
    bool preserveDirectives;
    bool lenientMissingIncludes;
    bool enableTrigraphs;
    CompilerContext* ctx;
} Preprocessor;

bool preprocessor_init(Preprocessor* pp,
                       CompilerContext* ctx,
                       bool preserveDirectives,
                       bool lenientMissingIncludes,
                       bool enableTrigraphs,
                       const char* const* includePaths,
                       size_t includePathCount,
                       const char* const* macroDefines,
                       size_t macroDefineCount);
void preprocessor_destroy(Preprocessor* pp);

bool preprocessor_run(Preprocessor* pp,
                      const TokenBuffer* input,
                      PPTokenBuffer* output);

MacroTable* preprocessor_get_macro_table(Preprocessor* pp);
IncludeResolver* preprocessor_get_resolver(Preprocessor* pp);
IncludeGraph* preprocessor_get_include_graph(Preprocessor* pp);

// Convenience helpers for directive drivers to evaluate #if/#elif expressions
bool preprocessor_eval_tokens(Preprocessor* pp,
                              const Token* tokens,
                              size_t count,
                              int32_t* outValue);

bool preprocessor_eval_range(Preprocessor* pp,
                             const TokenBuffer* buffer,
                             size_t start,
                             size_t end,
                             int32_t* outValue);

#endif /* PREPROCESSOR_DRIVER_H */
