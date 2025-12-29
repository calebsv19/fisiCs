#include "codegen_private.h"

#include <stdio.h>

LLVMValueRef codegenNode(CodegenContext* ctx, ASTNode* node) {
    if (!node) {
        fprintf(stderr, "Error: NULL node in codegen\n");
        return NULL;
    }

    CG_DEBUG("[CG] Enter node type %d\n", node->type);

    switch (node->type) {
        case AST_PROGRAM:
            return codegenProgram(ctx, node);
        case AST_BLOCK:
            return codegenBlock(ctx, node);
        case AST_BINARY_EXPRESSION:
            return codegenBinaryExpression(ctx, node);
        case AST_UNARY_EXPRESSION:
            return codegenUnaryExpression(ctx, node);
        case AST_TERNARY_EXPRESSION:
            return codegenTernaryExpression(ctx, node);
        case AST_ASSIGNMENT:
            return codegenAssignment(ctx, node);
        case AST_CAST_EXPRESSION:
            return codegenCastExpression(ctx, node);
        case AST_VARIABLE_DECLARATION:
            return codegenVariableDeclaration(ctx, node);
        case AST_ARRAY_ACCESS:
            return codegenArrayAccess(ctx, node);
        case AST_POINTER_ACCESS:
            return codegenPointerAccess(ctx, node);
        case AST_DOT_ACCESS:
            return codegenDotAccess(ctx, node);
        case AST_POINTER_DEREFERENCE:
            return codegenPointerDereference(ctx, node);
        case AST_IF_STATEMENT:
            return codegenIfStatement(ctx, node);
        case AST_WHILE_LOOP:
            return codegenWhileLoop(ctx, node);
        case AST_FOR_LOOP:
            return codegenForLoop(ctx, node);
        case AST_FUNCTION_DEFINITION:
            return codegenFunctionDefinition(ctx, node);
        case AST_FUNCTION_DECLARATION:
            return NULL; // prototypes do not emit code
        case AST_INCLUDE_DIRECTIVE:
        case AST_DEFINE_DIRECTIVE:
        case AST_CONDITIONAL_DIRECTIVE:
        case AST_ENDIF_DIRECTIVE:
            return NULL; // preserved preprocessor artifacts are non-code
        case AST_FUNCTION_CALL:
            return codegenFunctionCall(ctx, node);
        case AST_TYPEDEF:
            return codegenTypedef(ctx, node);
        case AST_ENUM_DEFINITION:
            return codegenEnumDefinition(ctx, node);
        case AST_RETURN:
            return codegenReturn(ctx, node);
        case AST_BREAK:
            return codegenBreak(ctx, node);
        case AST_CONTINUE:
            return codegenContinue(ctx, node);
        case AST_SWITCH:
            return codegenSwitch(ctx, node);
        case AST_LABEL_DECLARATION:
            return codegenLabel(ctx, node);
        case AST_GOTO_STATEMENT:
            return codegenGoto(ctx, node);
        case AST_NUMBER_LITERAL:
            return codegenNumberLiteral(ctx, node);
        case AST_CHAR_LITERAL:
            return codegenCharLiteral(ctx, node);
        case AST_STRING_LITERAL:
            return codegenStringLiteral(ctx, node);
        case AST_IDENTIFIER:
            return codegenIdentifier(ctx, node);
        case AST_SIZEOF:
            return codegenSizeof(ctx, node);
        case AST_ALIGNOF:
            return codegenAlignof(ctx, node);
        case AST_STATEMENT_EXPRESSION:
            return codegenStatementExpression(ctx, node);
        case AST_PARSED_TYPE:
            return NULL; /* handled contextually (e.g., sizeof) */
        case AST_COMPOUND_LITERAL:
            return codegenCompoundLiteral(ctx, node);
        case AST_STRUCT_DEFINITION:
            return (LLVMValueRef)codegenStructDefinition(ctx, node);
        case AST_UNION_DEFINITION:
            return (LLVMValueRef)codegenStructDefinition(ctx, node);
        case AST_STRUCT_FIELD_ACCESS:
            return codegenStructFieldAccess(ctx, node);
        case AST_HEAP_ALLOCATION:
            return codegenHeapAllocation(ctx, node);
        default:
            fprintf(stderr, "Error: Unhandled AST node type %d\n", node->type);
            return NULL;
    }
}
