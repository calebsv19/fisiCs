#ifndef AST_NODE_H
#define AST_NODE_H

#include "AST/ast_attribute.h"
#include "Parser/Helpers/designated_init.h"
#include "Parser/Helpers/parsed_type.h"
#include "Lexer/tokens.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

typedef enum {
    LIT_ENC_NARROW = 0,
    LIT_ENC_WIDE,
    LIT_ENC_UTF8
} LiteralEncoding;

static inline LiteralEncoding ast_literal_encoding(const char* value, const char** payloadOut) {
    if (value) {
        if (strncmp(value, "W|", 2) == 0) {
            if (payloadOut) *payloadOut = value + 2;
            return LIT_ENC_WIDE;
        }
        if (strncmp(value, "U8|", 3) == 0) {
            if (payloadOut) *payloadOut = value + 3;
            return LIT_ENC_UTF8;
        }
    }
    if (payloadOut) *payloadOut = value;
    return LIT_ENC_NARROW;
}

struct DesignatedInit;

// Define AST Node Types
typedef enum {
    AST_PROGRAM,
    AST_BLOCK,
    AST_STATEMENT_EXPRESSION,
    AST_HEAP_ALLOCATION,

    AST_INCLUDE_DIRECTIVE,
    AST_DEFINE_DIRECTIVE,
    AST_CONDITIONAL_DIRECTIVE,
    AST_ENDIF_DIRECTIVE,

    AST_TYPEDEF,
    AST_VARIABLE_DECLARATION,
    AST_UNION_DEFINITION,
    AST_STRUCT_DEFINITION,
    AST_STRUCT_FIELD_ACCESS,
    AST_ENUM_DEFINITION,

    AST_ARRAY_ACCESS,
    AST_POINTER_ACCESS,
    AST_DOT_ACCESS,
    AST_POINTER_DEREFERENCE,

    AST_TERNARY_EXPRESSION,
    AST_BINARY_EXPRESSION,
    AST_UNARY_EXPRESSION,
    AST_COMMA_EXPRESSION,
    AST_CAST_EXPRESSION,

    AST_BASIC_TYPE,
    AST_PARSED_TYPE,
    AST_NUMBER_LITERAL,
    AST_CHAR_LITERAL,
    AST_STRING_LITERAL,
    AST_IDENTIFIER,
    AST_SIZEOF,
    AST_COMPOUND_LITERAL,

    AST_ASSIGNMENT,

    AST_IF_STATEMENT,
    AST_ALIGNOF,
    AST_SEQUENCE,
    AST_WHILE_LOOP,
    AST_FOR_LOOP,

    AST_RETURN,
    AST_BREAK,
    AST_CONTINUE,
    AST_GOTO_STATEMENT,
    AST_LABEL_DECLARATION,

    AST_FUNCTION_DECLARATION,
    AST_FUNCTION_DEFINITION,
    AST_FUNCTION_CALL,
    AST_FUNCTION_POINTER,
    
    AST_SWITCH,
    AST_CASE,
    AST_STATIC_ASSERT,

    AST_ASM

} ASTNodeType;


// Definition of the AST Node Structure
// Forward declaration of the ASTNode structure
typedef struct ASTNode ASTNode;

struct ASTNode {
    ASTNodeType type;
    int line;
    SourceRange location;
    SourceRange macroCallSite;
    SourceRange macroDefinition;
    ASTNode *nextStmt;
    ASTAttribute** attributes;
    size_t attributeCount;

    union {
        //  **Global Scope & Blocks**
        struct {
            ASTNode **statements;
	    size_t statementCount;
        } block;

        struct {
            ASTNode* block;
        } statementExpr;
	
	struct {
	    ASTNode* enumName;
	    ASTNode** members;
	    ASTNode** values;  // NULL if implicit
	    size_t memberCount;
	} enumDef;


	struct {
	    ASTNode *structName;
	    ASTNode **fields;
	    size_t fieldCount;
            bool hasFlexibleArray;
	} structDef;
	
	struct {
	    ASTNode *structInstance;
	    char *fieldName;
	} structFieldAccess;
	
	struct {
	    ASTNode *allocType;
	} heapAlloc;


        //  **Expressions**
        struct {
            char *op;
            ASTNode *left;
            ASTNode *right;
	    bool isPostfix;
        } expr;

        struct {
            ASTNode *condition;
            ASTNode *trueExpr;
            ASTNode *falseExpr;
        } ternaryExpr;
	
	struct {
	    ASTNode **expressions;
	    size_t exprCount;
	} commaExpr;

	struct {
	    ParsedType castType;
	    ASTNode* expression;
	} castExpr;


	// PREPROCESSORS
	// #include
	struct {
            char *filePath;
	    bool isSystem;
        } includeDirective;
	
	// Define
	struct {
	    char *macroName;
	    char *macroValue;  // NULL for flag macros
	} defineDirective;
	
	// Ifdef / Ifndef
	struct {
	    char *symbol;
	    bool isNegated;  // false = #ifdef, true = #ifndef
	    ASTNode* body;
	} conditionalDirective;



        //  **Control Flow**
        struct {
            ASTNode *condition;
            ASTNode *thenBranch;
            ASTNode *elseBranch;
        } ifStmt;

        struct {
            ASTNode *initializer;
            ASTNode *condition;
            ASTNode *increment;
            ASTNode *body;
        } forLoop;

        struct {
            ASTNode *condition;
            ASTNode *body;
            int isDoWhile;
        } whileLoop;

        struct {
            ASTNode *condition;
            ASTNode **caseList;
	    size_t caseListSize;
        } switchStmt;

        struct {
            ASTNode *caseValue;
            ASTNode **caseBody;
            ASTNode *nextCase;
	    size_t caseBodySize;
        } caseStmt;



        //  **Functions**
	struct {
	    ParsedType returnType;
	    ASTNode* funcName;
	    ASTNode** parameters;
	    size_t paramCount;
            bool isVariadic;
	} functionDecl;


        struct {
            ParsedType returnType;
            ASTNode *funcName;
            ASTNode **parameters;
            ASTNode *body;
	    size_t paramCount;
            bool isVariadic;
        } functionDef;

        struct {
            ASTNode *callee;
            ASTNode **arguments;
	    size_t argumentCount;
            bool usesPrototype;
        } functionCall;

        struct {
            ParsedType returnType;
            ASTNode* name;                // function pointer name (identifier)
            ASTNode** parameters;         // parameter list (if you keep it)
            size_t paramCount;
            ASTNode* initializer;         // NEW: optional "= <expr>" initializer
        } functionPointer;





        //  **Variables & Assignments**
        struct {
            ParsedType declaredType;
            ParsedType* declaredTypes;
            ASTNode** varNames;
	    struct DesignatedInit** initializers;
	    ASTNode* arraySize;
	    ASTNode* bitFieldWidth;
	    size_t varCount;
        } varDecl;

	struct {
	    ParsedType baseType;
	    ASTNode* alias;
	} typedefStmt;


        // **Array Access**
        struct {
            ASTNode* array;
            ASTNode* index;
        } arrayAccess;



        //  **Pointer Dereference**
        struct {
            ASTNode* pointer;
        } pointerDeref;

	struct {
     	    ASTNode* base;
    	    char* field;
	} memberAccess;
        
	struct {
            ASTNode *target;
            char *op;
            ASTNode *value;
        } assignment;



        //  **Identifiers, Types, and Literals** (Unified)
	struct {
	    ParsedType literalType;  
	    struct DesignatedInit** entries;
	    size_t entryCount;
            bool isStaticStorage;   // true when the literal has static storage duration (file scope)
	} compoundLiteral;


        struct {
            char *value;
        } valueNode;  // Used for Identifiers, Basic Types, Numbers, and Strings

        // Parsed type wrapper
	struct {
	    ParsedType parsed;
	} parsedTypeNode;


        //  **Return, Break, Continue**
        struct {
            ASTNode *returnValue;
        } returnStmt;

        struct {
            int isBreak;
            ASTNode *loopTarget;
        } loopControl;
	
	struct {
	    char* label;
	} gotoStmt;

	struct {
	    char* labelName;
	    ASTNode* statement;  // The statement this label is attached to
	} label;


	struct {
	    char* asmText;
        } asmStmt;

    };
};

static inline const ParsedType* astVarDeclTypeAt(const ASTNode* node, size_t index) {
    if (!node || node->type != AST_VARIABLE_DECLARATION) {
        return NULL;
    }
    const ParsedType* declared = node->varDecl.declaredTypes;
    if (declared && index < node->varDecl.varCount) {
        return declared + index;
    }
    return &node->varDecl.declaredType;
}





// AST Node Creation Functions
// -----------------------------


// Main blocks
ASTNode *createProgramNode(ASTNode **declarations, size_t declCount);
ASTNode *createBlockNode(ASTNode **statements, size_t statementCount);
ASTNode* createStatementExpressionNode(ASTNode* block);
void astNodeAppendAttributes(ASTNode* node, ASTAttribute** attrs, size_t count);
void astNodeCloneTypeAttributes(ASTNode* node, const ParsedType* type);
void astNodeSetProvenance(ASTNode* node, const Token* tok);


// Preprocesses
ASTNode* createIncludeDirectiveNode(const char* filePath, bool isSystem);
ASTNode* createDefineDirectiveNode(const char* macroName, const char* macroValue);
ASTNode* createConditionalDirectiveNode(const char* symbol, bool isNegated);
ASTNode* createEndifDirectiveNode(void);


// Expressions
ASTNode *createTernaryExprNode(ASTNode *condition, ASTNode *trueExpr, ASTNode *falseExpr);
ASTNode *createBinaryExprNode(const char *op, ASTNode *left, ASTNode *right);
ASTNode* createUnaryExprNode(const char* op, ASTNode* operand, bool isPostfix);
ASTNode* createCommaExprNode(ASTNode** expressions, size_t count);
ASTNode* createCastExpressionNode(ParsedType type, ASTNode* expr);
ASTNode* createCompoundLiteralNode(ParsedType literalType,
                                   struct DesignatedInit** entries,
                                   size_t entryCount);


// Singular types
ASTNode *createBasicTypeNode(const char *name);
ASTNode *createIdentifierNode(const char *name);
ASTNode* createAsmNode(const char* asmText);
ASTNode* createStaticAssertNode(ASTNode* condition, ASTNode* message);
ASTNode *createSizeofNode(ASTNode* target);
ASTNode *createAlignofNode(ASTNode* target);
ASTNode *createNumberLiteralNode(const char *value);
ASTNode *createCharLiteralNode(const char *value);
ASTNode *createStringLiteralNode(const char *value);
ASTNode* createParsedTypeNode(ParsedType type);

// Assignents and Arrays
ASTNode *createAssignmentNode(ASTNode *identifier, ASTNode *value, TokenType op);
ASTNode *createVariableDeclarationNode(ParsedType declaredType, ASTNode **identifiers,
                                       struct DesignatedInit **initializers, size_t varCount);
ASTNode* createTypedefNode(ParsedType baseType, ASTNode* alias);
ASTNode* createEnumDefinitionNode(const char* name, ASTNode** members, 
					ASTNode** values, size_t count);
ASTNode* createStructOrUnionDefinitionNode(ASTNodeType type, const char* name, 
					ASTNode** fields, size_t fieldCount);
ASTNode* createArrayAccessNode(ASTNode* array, ASTNode* index);
ASTNode* createMemberAccessNode(TokenType op, ASTNode* base, const char* fieldName);
ASTNode* createPointerDereferenceNode(ASTNode* operand);

// Tests and loops
ASTNode *createIfStatementNode(ASTNode *condition, ASTNode *thenBody);
ASTNode *createIfElseStatementNode(ASTNode *condition, ASTNode *thenBody, ASTNode *elseBody);
ASTNode *createSequenceNode(ASTNode *left, ASTNode *right);
ASTNode *createWhileLoopNode(ASTNode *condition, ASTNode *body, int isDoWhile);
ASTNode *createForLoopNode(ASTNode *init, ASTNode *condition, ASTNode *increment, ASTNode *body);


// Break points
ASTNode *createReturnNode(ASTNode *expr);
ASTNode *createBreakNode(void);
ASTNode *createContinueNode(void);
ASTNode* createGotoStatementNode(const char* label);
ASTNode* createLabelDeclarationNode(const char* labelName, ASTNode* statement);


// Functions
ASTNode* createFunctionDeclarationNode(ParsedType returnType,
                                       ASTNode* funcName,
                                       ASTNode** paramList,
                                       size_t paramCount,
                                       bool isVariadic);
ASTNode* createFunctionDefinitionNode(ParsedType returnType,
                                      ASTNode* funcName,
                                      ASTNode** paramList,
				      ASTNode* body,
                                      size_t paramCount,
                                      bool isVariadic);
ASTNode *createFunctionCallNode(ASTNode *callee, ASTNode **arguments, size_t argumentCount);
ASTNode* createFunctionPointerDeclarationNode(ParsedType returnType,
                                              ASTNode* name,
                                              ASTNode** params,
                                              size_t paramCount,
                                              ASTNode* initializer);



// Switch/Case
ASTNode *createCaseNode(ASTNode *caseValue, ASTNode **caseBody);
ASTNode *createSwitchNode(ASTNode *condition, ASTNode **caseList);


#endif // AST_NODE_H
