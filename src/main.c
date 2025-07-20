#include <stdio.h>
#include <stdlib.h>
#include "Lexer/lexer.h"
#include "Parser/parser.h"
#include "Parser/designated_init.h"
#include "Parser/parser_helpers.h"
#include "Syntax/semantic_pass.h"
#include "AST/ast_printer.h"
#include "Utils/utils.h"
#include "code_gen.h"

// === Feature Toggles ===
#define ENABLE_LEXER_OUTPUT      0
#define ENABLE_AST_PRINT         1
#define ENABLE_SYNTAX_CHECK      1
#define ENABLE_CODEGEN           0

int main(void) {
    const char *filename = "include/test.txt";
    char *sourceCode = readFile(filename);

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
    initParser(&parser, &lexer, PARSER_MODE_PRATT);
    ASTNode *root = parse(&parser);


#if ENABLE_AST_PRINT
    printf(" AST Output:\n");
    printAST(root, 0);
#endif


#if ENABLE_SYNTAX_CHECK
    printf("\n Semantic Analysis:\n");
    analyzeSemantics(root);
#endif


#if ENABLE_CODEGEN
    printf("\n️ LLVM Code Generation:\n");
    initializeLLVM();
    LLVMValueRef result = codegen(root);
    LLVMDumpModule(TheModule);
#endif

    return 0;
}




