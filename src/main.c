#include <stdio.h>
#include <stdlib.h>
#include "Lexer/lexer.h"
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

// === Feature Toggles ===
#define ENABLE_LEXER_OUTPUT      0
#define ENABLE_AST_PRINT         1
#define ENABLE_SYNTAX_CHECK      1
#define ENABLE_CODEGEN           1

int main(int argc, char **argv) {
    const char *filename = (argc > 1) ? argv[1] : "include/test.txt";
    char *sourceCode = readFile(filename);
    int enableCodegen = ENABLE_CODEGEN;
    const char* disableCodegenEnv = getenv("DISABLE_CODEGEN");
    if (disableCodegenEnv && disableCodegenEnv[0] != '\0' && disableCodegenEnv[0] != '0') {
        enableCodegen = 0;
    }

    CompilerContext* ctx = cc_create();
    if (!ctx) { fprintf(stderr, "OOM: CompilerContext\n"); return 1; }
    cc_seed_builtins(ctx);


    // === Lexing Phase ===
    Lexer lexer;
    initLexer(&lexer, sourceCode);

#if ENABLE_LEXER_OUTPUT
    Token token;
    do {
        token = getNextToken(&lexer);
        printf("Token: Type=%d, Value=%s\n", token.type, token.value);
    } while (token.type != TOKEN_EOF);
#endif




    // === Parsing Phase ===
    Parser parser;
    initParser(&parser, &lexer, PARSER_MODE_PRATT, ctx);

    ASTNode *root = parse(&parser);


#if ENABLE_AST_PRINT
    printf(" AST Output:\n");
    printAST(root, 0);
#endif


#if ENABLE_SYNTAX_CHECK
    printf("\n Semantic Analysis:\n");
#endif
    SemanticModel* semanticModel = analyzeSemanticsBuildModel(root, ctx, false);
    if (!semanticModel) {
        free(sourceCode);
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


    free(sourceCode);
    semanticModelDestroy(semanticModel);
    cc_destroy(ctx);
    return 0;
}
