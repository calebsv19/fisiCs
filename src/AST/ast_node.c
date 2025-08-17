#include "ast_node.h"
#include "Parser/parser.h"
#include "Parser/parser_helpers.h"

// Creates a Program node (global declarations)
ASTNode *createProgramNode(ASTNode **declarations, size_t declCount) {
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node) {
        printf("Error: Memory allocation failed for ProgramNode\n");
        return NULL;
    }

    node->type = AST_PROGRAM;
    node->block.statementCount = declCount;
    node->block.statements = declarations ? declarations : NULL; // Ensure safe handling
    return node;   
}

// Creates a Block (Compound Statement) node
ASTNode *createBlockNode(ASTNode **statements, size_t statementCount) {
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node) {
        printf("Error: Memory allocation failed for BlockNode\n");
        return NULL;
    }

    node->type = AST_BLOCK;
    node->block.statementCount = statementCount;
    node->block.statements = statements ? statements : NULL; // Ensure safe handling
    return node;
}

// 		MAIN BLOCKS
// -----------------------------------------------------------------
// 		PREPROCESSORS


ASTNode* createIncludeDirectiveNode(const char* filePath, bool isSystem) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (!node) {
        printf("Error: Memory allocation failed for IncludeDirectiveNode\n");
        return NULL;
    }

    node->type = AST_INCLUDE_DIRECTIVE;
    node->includeDirective.filePath = strdup(filePath);
    node->includeDirective.isSystem = isSystem;
    node->nextStmt = NULL;

    return node;
}

ASTNode* createDefineDirectiveNode(const char* macroName, const char* macroValue) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (!node) {
        printf("Error: Memory allocation failed for DefineDirectiveNode\n");
        return NULL;
    }

    node->type = AST_DEFINE_DIRECTIVE;
    node->defineDirective.macroName = strdup(macroName);
    node->defineDirective.macroValue = macroValue ? strdup(macroValue) : NULL;
    node->nextStmt = NULL;

    return node;
}

ASTNode* createConditionalDirectiveNode(const char* symbol, bool isNegated) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (!node) {
        printf("Error: Memory allocation failed for ConditionalDirectiveNode\n");
        return NULL;
    }

    node->type = AST_CONDITIONAL_DIRECTIVE;
    node->conditionalDirective.symbol = strdup(symbol);
    node->conditionalDirective.isNegated = isNegated;
    node->nextStmt = NULL;

    return node;
}

ASTNode* createEndifDirectiveNode(void) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (!node) {
        printf("Error: Memory allocation failed for EndifDirectiveNode\n");
        return NULL;
    }

    node->type = AST_ENDIF_DIRECTIVE;
    node->nextStmt = NULL;
    // No additional data needed

    return node;
}





//              PREPROCESSORS
// -----------------------------------------------------------------
//   		EXPRESSIONS





// Creates a Unary Expression node
ASTNode *createUnaryExprNode(const char *op, ASTNode *operand, bool isPostfix) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_UNARY_EXPRESSION; 
    node->expr.op = strdup(op);  
    node->expr.left = operand;  // Operand is stored in 'left'
    node->expr.right = NULL;    // NULL indicates a unary operation
    node->expr.isPostfix = isPostfix;
    return node;
}

// Creates a Binary Expression node (operator, left operand, right operand)
ASTNode *createBinaryExprNode(const char *op, ASTNode *left, ASTNode *right) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_BINARY_EXPRESSION;
    node->expr.op = strdup(op);  // Duplicate operator string for safety
    node->expr.left = left;
    node->expr.right = right;
    return node;
}

// Creates a Ternary Expression node (condition ? trueExpr : falseExpr)
ASTNode *createTernaryExprNode(ASTNode *condition, ASTNode *trueExpr, ASTNode *falseExpr) {
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node) {
        printf("Error: Memory allocation failed for TernaryExprNode\n");
        return NULL;
    }

    node->type = AST_TERNARY_EXPRESSION;
    node->ternaryExpr.condition = condition;
    node->ternaryExpr.trueExpr = trueExpr;
    node->ternaryExpr.falseExpr = falseExpr;
    
    return node;
}

// Creates a Comma Expression node from a flat list of expressions
ASTNode* createCommaExprNode(ASTNode** expressions, size_t count) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (!node) {
        printf("Error: Memory allocation failed for CommaExprNode\n");
        return NULL;
    }

    node->type = AST_COMMA_EXPRESSION;
    node->commaExpr.expressions = expressions;
    node->commaExpr.exprCount = count;
    node->nextStmt = NULL;

    return node;
}

ASTNode* createCastExpressionNode(ParsedType castType, ASTNode* expr) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (!node) {
        printf("Error: Memory allocation failed for cast expression node.\n");
        return NULL;
    }

    memset(node, 0, sizeof(ASTNode));
    node->type = AST_CAST_EXPRESSION;
    node->castExpr.castType = castType;
    node->castExpr.expression = expr;
    return node;
}





//              EXPRESSIONS
// -----------------------------------------------------------------
//               BASIC





// Creates a Basic Type node (e.g., "int", "float", etc.)
ASTNode *createBasicTypeNode(const char *name) {
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node) {
        printf("Error: Memory allocation failed for BasicTypeNode\n");
        return NULL;
    }
    
    node->type = AST_BASIC_TYPE;
    node->valueNode.value = strdup(name); // Store type name safely

    if (!node->valueNode.value) {
        printf("Error: Memory allocation failed for BasicTypeNode name\n");
        free(node);
        return NULL;
    }

    return node;
}

ASTNode* createAsmNode(char* asmText) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_ASM;
    node->nextStmt = NULL;
    node->asmStmt.asmText = asmText;
    return node;
}


// Literal Nodes
ASTNode* createSizeofNode(ASTNode* target) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (!node) {
        printf("Error: Memory allocation failed for sizeof node.\n");
        return NULL;
    }
    node->type = AST_SIZEOF;
    node->expr.left = target;  // `target` can be a type or variable
    return node;
}

// Creates a Number Literal node
ASTNode *createNumberLiteralNode(const char *value) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_NUMBER_LITERAL;
    node->valueNode.value = strdup(value);
    return node;
}

// Creates a Character Literal node
ASTNode *createCharLiteralNode(const char *value) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_CHAR_LITERAL;
    node->valueNode.value = strdup(value);
    return node;
}

// Creates a String Literal node
ASTNode *createStringLiteralNode(const char *value) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_STRING_LITERAL;
    node->valueNode.value = strdup(value);
    return node;
}

// Creates an Identifier node
ASTNode *createIdentifierNode(const char *name) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_IDENTIFIER;
    node->valueNode.value = strdup(name);
    return node;
}


//              BASIC TYPES
// -----------------------------------------------------------------
//              ASSIGNMENTS





// Assignment & Variable Declaration Nodes
// Creates an Assignment node (identifier, value, and operator)
ASTNode *createAssignmentNode(ASTNode *identifier, ASTNode *value, TokenType op) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_ASSIGNMENT;

    // Store the full operator string
    node->assignment.op = strdup(getOperatorString(op));

    if (!node->assignment.op) {
        printf("Error: Failed to assign operator string\n");
        free(node);
        return NULL;
    }

    node->assignment.target = identifier;
    node->assignment.value = value;
    return node;
}

ASTNode *createVariableDeclarationNode(ParsedType declaredType,
                                       ASTNode **identifiers,
                                       struct DesignatedInit **initializers,
                                       size_t varCount) {
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node) {
        printf("Error: Memory allocation failed for VariableDeclarationNode\n");
        return NULL;
    }

    node->type = AST_VARIABLE_DECLARATION;
    node->varDecl.declaredType = declaredType;
    node->varDecl.varNames = identifiers;
    node->varDecl.initializers = initializers;
    node->varDecl.varCount = varCount;

    return node;
}

ASTNode* createTypedefNode(ParsedType baseType, ASTNode* alias) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_TYPEDEF;
    node->nextStmt = NULL;

    node->typedefStmt.baseType = baseType;
    node->typedefStmt.alias = alias;

    return node;
}

ASTNode* createEnumDefinitionNode(const char* name, ASTNode** members, ASTNode** values, size_t count) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (!node) {
        printf("Error: Memory allocation failed for enum definition.\n");
        return NULL;
    }

    memset(node, 0, sizeof(ASTNode));
    node->type = AST_ENUM_DEFINITION;
    node->nextStmt = NULL;

    node->enumDef.enumName = createIdentifierNode(name);
    node->enumDef.members = members;
    node->enumDef.values = values;
    node->enumDef.memberCount = count;

    return node;
}



ASTNode* createStructOrUnionDefinitionNode(ASTNodeType type, const char* name, 
						ASTNode** fields, size_t fieldCount) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (!node) {
        printf("Error: Memory allocation failed for %s definition.\n", type == AST_STRUCT_DEFINITION ? 
			"struct" : "union");
        return NULL;
    }

    memset(node, 0, sizeof(ASTNode));
    node->type = type;

    node->structDef.structName = createIdentifierNode(name);
    node->structDef.fields = fields;
    node->structDef.fieldCount = fieldCount;

    return node;
}




ASTNode *createArrayDeclarationNode(ParsedType declaredType,
                                    ASTNode *name, ASTNode *size,
                                    struct DesignatedInit **initValues, size_t valueCount) {
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node) {
        printf("Error: Memory allocation failed for array declaration.\n");
        return NULL;
    }

    node->type = AST_ARRAY_DECLARATION;
    node->arrayDecl.declaredType = declaredType;
    node->arrayDecl.varName = name;
    node->arrayDecl.arraySize = size;
    node->arrayDecl.initializers = initValues;
    node->arrayDecl.valueCount = valueCount;

    return node;
}



ASTNode* chainArraySizes(ASTNode* outer, ASTNode* inner) {
    if (!outer) return inner;
    ASTNode* current = outer;
    while (current->nextStmt) {
        current = current->nextStmt;
    }
    current->nextStmt = inner;
    return outer;
}




ASTNode* createArrayAccessNode(ASTNode* array, ASTNode* index) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (!node) {
        printf("Error: Memory allocation failed for array access.\n");
        return NULL;
    }

    node->type = AST_ARRAY_ACCESS;
    node->arrayAccess.array = array;
    node->arrayAccess.index = index;
    
    return node;
}



ASTNode* createMemberAccessNode(TokenType op, ASTNode* base, const char* fieldName) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = (op == TOKEN_ARROW) ? AST_POINTER_ACCESS : AST_DOT_ACCESS;
    node->memberAccess.base = base;
    node->memberAccess.field = strdup(fieldName);
    return node;
}


ASTNode* createPointerDereferenceNode(ASTNode* operand) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (!node) {
        printf("Error: Memory allocation failed for pointer dereference.\n");
        return NULL;
    }

    node->type = AST_POINTER_DEREFERENCE;
    node->pointerDeref.pointer = operand;
    
    return node;
}



// Conditional & Sequence Nodes
// Creates an If Statement node (condition and then-body)
ASTNode *createIfStatementNode(ASTNode *condition, ASTNode *thenBody) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_IF_STATEMENT;
    node->ifStmt.condition = condition;
    node->ifStmt.thenBranch = thenBody;
    node->ifStmt.elseBranch = NULL; // No else branch
    return node;
}



// Creates an If-Else Statement node (condition, then-body, and else-body)
ASTNode *createIfElseStatementNode(ASTNode *condition, ASTNode *thenBody, ASTNode *elseBody) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_IF_STATEMENT;
    node->ifStmt.condition = condition;
    node->ifStmt.thenBranch = thenBody;
    node->ifStmt.elseBranch = elseBody; // Directly store else branch
    return node;
}



// Loop Nodes
// Creates a While Loop node (condition and body)
ASTNode *createWhileLoopNode(ASTNode *condition, ASTNode *body, int isDoWhile) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_WHILE_LOOP;
    node->whileLoop.condition = condition;
    node->whileLoop.body = body;
    node->whileLoop.isDoWhile = isDoWhile; // 0 = while, 1 = do-while
    return node;
}

// Creates a For Loop node (initializer, condition, increment, and body)
ASTNode *createForLoopNode(ASTNode *init, ASTNode *condition, ASTNode *increment, ASTNode *body) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_FOR_LOOP;
    node->forLoop.initializer = init;
    node->forLoop.condition = condition;
    node->forLoop.increment = increment;
    node->forLoop.body = body;
    return node;
}




//              TESTS AND LOOPS
// -----------------------------------------------------------------
//              CONTROL




// Flow Control Nodes
// Creates a Return node with an optional return value
ASTNode *createReturnNode(ASTNode *expr) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_RETURN;
    node->returnStmt.returnValue = expr;  // NULL if void return
    return node;
}

// Creates a Break node
ASTNode *createBreakNode(void) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_BREAK;
    node->loopControl.isBreak = 1;  // 1 = Break
    node->loopControl.loopTarget = NULL; // May be set later for nested loops
    return node;
}

// Creates a Continue node
ASTNode *createContinueNode(void) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_CONTINUE;
    node->loopControl.isBreak = 0;  // 0 = Continue
    node->loopControl.loopTarget = NULL; // May be set later for nested loops
    return node;
}

ASTNode* createGotoStatementNode(const char* label) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_GOTO_STATEMENT;
    node->nextStmt = NULL;

    node->gotoStmt.label = strdup(label); // Duplicate string to store safely

    return node;
}

ASTNode* createLabelDeclarationNode(const char* labelName, ASTNode* statement) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (!node) {
        printf("Error: Memory allocation failed for label declaration node.\n");
        return NULL;
    }

    memset(node, 0, sizeof(ASTNode));
    node->type = AST_LABEL_DECLARATION;
    node->nextStmt = NULL;

    node->label.labelName = strdup(labelName);
    node->label.statement = statement;

    return node;
}





//              CONTROL
// -----------------------------------------------------------------
//              FUNCTIONS




ASTNode* createFunctionDeclarationNode(ParsedType returnType, const char* name) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (!node) {
        printf("Error: Memory allocation failed for function declaration.\n");
        return NULL;
    }

    node->type = AST_FUNCTION_DECLARATION;
    node->functionDecl.returnType = returnType;

    // Store name as an identifier node (for consistency with func defs)
    ASTNode* nameNode = malloc(sizeof(ASTNode));
    if (!nameNode) {
        printf("Error: Memory allocation failed for function name node.\n");
        free(node);
        return NULL;
    }

    nameNode->type = AST_IDENTIFIER;
    nameNode->valueNode.value = strdup(name);
    node->functionDecl.funcName = nameNode;

    node->functionDecl.parameters = NULL;
    node->functionDecl.paramCount = 0;

    return node;
}



// Function Nodes
ASTNode* createFunctionDefinitionNode(ParsedType returnType, ASTNode* funcName,
                                      ASTNode** paramList, ASTNode* body, size_t paramCount) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (!node) {
        printf("Error: Memory allocation failed for function definition.\n");
        return NULL;
    }

    node->type = AST_FUNCTION_DEFINITION;
    node->functionDef.returnType = returnType;
    node->functionDef.funcName = funcName;
    node->functionDef.parameters = paramList;
    node->functionDef.paramCount = paramCount;
    node->functionDef.body = body;

    return node;
}


// Creates a Function Call node (callee and arguments)
ASTNode *createFunctionCallNode(ASTNode *callee, ASTNode **arguments, size_t argumentCount) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_FUNCTION_CALL;
    node->functionCall.callee = callee;
    node->functionCall.arguments = arguments; // Argument list (linked list)
    node->functionCall.argumentCount = argumentCount;
    return node;
}

ASTNode* createFunctionPointerDeclarationNode(ParsedType returnType,
                                              ASTNode* name,
                                              ASTNode** params,
                                              size_t paramCount,
                                              ASTNode* initializer) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = AST_FUNCTION_POINTER;
    node->nextStmt = NULL;

    node->functionPointer.returnType = returnType;   // includes isFunctionPointer, fpParams
    node->functionPointer.name = name;
    node->functionPointer.parameters = params;
    node->functionPointer.paramCount = paramCount;
    node->functionPointer.initializer = initializer; // NEW

    return node;
}






//              FUNCTIONS
// ----------------------------------------------------------------- 
//              SWITCHES




// Creates a Case node (case value and its associated block)
ASTNode *createCaseNode(ASTNode *caseValue, ASTNode **caseBody) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_CASE;
    node->caseStmt.caseValue = caseValue; // NULL for default case
    node->caseStmt.caseBody = caseBody;   // Block of statements inside the case
    node->caseStmt.nextCase = NULL;       // Initialized to NULL, linked later
    return node;
}

// Creates a Switch node (switch expression and case list)
ASTNode *createSwitchNode(ASTNode *condition, ASTNode **caseList) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_SWITCH;
    node->switchStmt.condition = condition; // Expression inside switch
    node->switchStmt.caseList = caseList;   // Array (linked list) of case nodes
    return node;
}


ASTNode* createParsedTypeNode(ParsedType type) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (!node) {
        printf("Error: Memory allocation failed for parsed type node.\n");
        return NULL;
    }

    memset(node, 0, sizeof(ASTNode));
    node->type = AST_BASIC_TYPE;
    node->parsedTypeNode.parsed = type;
    
    return node;
}

