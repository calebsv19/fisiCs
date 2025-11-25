#include "ast_node.h"
#include "Parser/parser.h"
#include "Parser/Helpers/parser_helpers.h"




// Safer strdup (portable even if strdup isn't declared)
static char* xstrdup(const char* s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char* p = (char*)malloc(n);
    if (!p) return NULL;
    memcpy(p, s, n);
    return p;
}

// ast_node.c
static ASTNode* new_node(ASTNodeType tag) {
    ASTNode* n = (ASTNode*)calloc(1, sizeof(ASTNode));  // zero everything
    if (!n) { fprintf(stderr, "OOM ASTNode\n"); return NULL; }
    n->type = tag;
    n->line = 0;
    n->attributes = NULL;
    n->attributeCount = 0;
    return n;
}

static void inherit_line_from_node(ASTNode* target, ASTNode* source) {
    if (!target || target->line) return;
    if (source && source->line) {
        target->line = source->line;
    }
}

static void inherit_line_from_pair(ASTNode* target, ASTNode* first, ASTNode* second) {
    inherit_line_from_node(target, first);
    if (target && target->line) return;
    inherit_line_from_node(target, second);
}

static void inherit_line_from_array(ASTNode* target, ASTNode** nodes, size_t count) {
    if (!target || target->line || !nodes) return;
    for (size_t i = 0; i < count; ++i) {
        inherit_line_from_node(target, nodes[i]);
        if (target->line) break;
    }
}

static void inherit_line_from_designated_inits(ASTNode* target,
                                               DesignatedInit** entries,
                                               size_t count) {
    if (!target || target->line || !entries) return;
    for (size_t i = 0; i < count; ++i) {
        DesignatedInit* entry = entries[i];
        if (!entry) continue;
        inherit_line_from_node(target, entry->expression);
        if (target->line) break;
        inherit_line_from_node(target, entry->indexExpr);
        if (target->line) break;
    }
}



// Creates a Program node (global declarations)
ASTNode *createProgramNode(ASTNode **declarations, size_t declCount) {
    ASTNode *node = new_node(AST_PROGRAM);
    if (!node) return NULL;
    node->block.statementCount = declCount;
    node->block.statements = declarations;   // may be NULL if count==0
    inherit_line_from_array(node, declarations, declCount);
    return node;
}


// Creates a Block (Compound Statement) node
ASTNode *createBlockNode(ASTNode **statements, size_t statementCount) {
    ASTNode *node = new_node(AST_BLOCK);
    if (!node) return NULL;
    node->block.statementCount = statementCount;
    node->block.statements = statements;
    inherit_line_from_array(node, statements, statementCount);
    return node;
}

ASTNode* createStatementExpressionNode(ASTNode* block) {
    ASTNode* node = new_node(AST_STATEMENT_EXPRESSION);
    if (!node) return NULL;
    node->statementExpr.block = block;
    inherit_line_from_node(node, block);
    return node;
}

void astNodeAppendAttributes(ASTNode* node, ASTAttribute** attrs, size_t count) {
    if (!attrs || count == 0) {
        free(attrs);
        return;
    }
    if (!node) {
        astAttributeListDestroy(attrs, count);
        free(attrs);
        return;
    }
    size_t newCount = node->attributeCount + count;
    ASTAttribute** merged = (ASTAttribute**)realloc(node->attributes, newCount * sizeof(ASTAttribute*));
    if (!merged) {
        astAttributeListDestroy(attrs, count);
        free(attrs);
        return;
    }
    memcpy(merged + node->attributeCount, attrs, count * sizeof(ASTAttribute*));
    free(attrs);
    node->attributes = merged;
    node->attributeCount = newCount;
}

void astNodeCloneTypeAttributes(ASTNode* node, const ParsedType* type) {
    if (!node || !type || type->attributeCount == 0 || !type->attributes) {
        return;
    }
    ASTAttribute** clones = astAttributeListClone(type->attributes, type->attributeCount);
    if (!clones) {
        return;
    }
    astNodeAppendAttributes(node, clones, type->attributeCount);
}


// 		MAIN BLOCKS
// -----------------------------------------------------------------
// 		PREPROCESSORS


ASTNode* createIncludeDirectiveNode(const char* filePath, bool isSystem) {
    ASTNode* node = new_node(AST_INCLUDE_DIRECTIVE);
    if (!node) return NULL;

    node->includeDirective.filePath = xstrdup(filePath);
    if (filePath && !node->includeDirective.filePath) {
        free(node);
        fprintf(stderr, "OOM: include filePath\n");
        return NULL;
    }
    node->includeDirective.isSystem = isSystem;
    return node;
}


ASTNode* createDefineDirectiveNode(const char* macroName, const char* macroValue) {
    ASTNode* node = new_node(AST_DEFINE_DIRECTIVE);
    if (!node) return NULL;

    node->defineDirective.macroName  = xstrdup(macroName);
    node->defineDirective.macroValue = macroValue ? xstrdup(macroValue) : NULL;

    if ((macroName && !node->defineDirective.macroName) ||
        (macroValue && !node->defineDirective.macroValue)) {
        // best-effort cleanup
        free(node->defineDirective.macroName);
        free(node->defineDirective.macroValue);
        free(node);
        fprintf(stderr, "OOM: define macro strings\n");
        return NULL;
    }
    return node;
}


ASTNode* createConditionalDirectiveNode(const char* symbol, bool isNegated) {
    ASTNode* node = new_node(AST_CONDITIONAL_DIRECTIVE);
    if (!node) return NULL;

    node->conditionalDirective.symbol    = xstrdup(symbol);
    node->conditionalDirective.isNegated = isNegated;

    if (symbol && !node->conditionalDirective.symbol) {
        free(node);
        fprintf(stderr, "OOM: conditional symbol\n");
        return NULL;
    }
    return node;
}


ASTNode* createEndifDirectiveNode(void) {
    ASTNode* node = new_node(AST_ENDIF_DIRECTIVE);
    if (!node) return NULL;
    // no payload
    return node;
}




//              PREPROCESSORS
// -----------------------------------------------------------------
//   		EXPRESSIONS





// Creates a Unary Expression node
ASTNode* createUnaryExprNode(const char* op, ASTNode* operand, bool isPostfix) {
    ASTNode* node = new_node(AST_UNARY_EXPRESSION);
    if (!node) return NULL;

    node->expr.op = op ? xstrdup(op) : NULL;
    if (op && !node->expr.op) { free(node); return NULL; }

    node->expr.left     = operand;   // operand goes in 'left'
    node->expr.right    = NULL;      // unary => right is NULL
    node->expr.isPostfix = isPostfix;
    inherit_line_from_node(node, operand);
    return node;
}

// Creates a Binary Expression node (operator, left operand, right operand)
ASTNode* createBinaryExprNode(const char* op, ASTNode* left, ASTNode* right) {
    ASTNode* node = new_node(AST_BINARY_EXPRESSION);
    if (!node) return NULL;

    node->expr.op = op ? xstrdup(op) : NULL;
    if (op && !node->expr.op) { free(node); return NULL; }

    node->expr.left  = left;
    node->expr.right = right;
    inherit_line_from_pair(node, left, right);
    return node;
}


// Creates a Ternary Expression node (condition ? trueExpr : falseExpr)
ASTNode* createTernaryExprNode(ASTNode* condition, ASTNode* trueExpr, ASTNode* falseExpr) {
    ASTNode* node = new_node(AST_TERNARY_EXPRESSION);
    if (!node) return NULL;

    node->ternaryExpr.condition = condition;
    node->ternaryExpr.trueExpr  = trueExpr;
    node->ternaryExpr.falseExpr = falseExpr;
    inherit_line_from_node(node, condition);
    inherit_line_from_node(node, trueExpr);
    inherit_line_from_node(node, falseExpr);
    return node;
}


// Creates a Comma Expression node from a flat list of expressions
ASTNode* createCommaExprNode(ASTNode** expressions, size_t count) {
    ASTNode* node = new_node(AST_COMMA_EXPRESSION);
    if (!node) return NULL;

    node->commaExpr.expressions = expressions; // ownership assumed by AST
    node->commaExpr.exprCount   = count;
    inherit_line_from_array(node, expressions, count);
    return node;
}


ASTNode* createCastExpressionNode(ParsedType castType, ASTNode* expr) {
    ASTNode* node = new_node(AST_CAST_EXPRESSION);
    if (!node) return NULL;

    node->castExpr.castType   = castType;  // POD copy-by-value
    node->castExpr.expression = expr;
    inherit_line_from_node(node, expr);
    return node;
}


ASTNode* createCompoundLiteralNode(ParsedType literalType,
                                   struct DesignatedInit** entries,
                                   size_t entryCount) {
    ASTNode* node = new_node(AST_COMPOUND_LITERAL);
    if (!node) return NULL;

    node->compoundLiteral.literalType = literalType; // POD copy
    node->compoundLiteral.entries     = entries;     // may be NULL for "{}"
    node->compoundLiteral.entryCount  = entryCount;  // 0 for "{}"
    inherit_line_from_designated_inits(node, entries, entryCount);
    return node;
}






//              EXPRESSIONS
// -----------------------------------------------------------------
//               BASIC





// Creates a Basic Type node (e.g., "int", "float", etc.)
ASTNode* createBasicTypeNode(const char* name) {
    ASTNode* node = new_node(AST_BASIC_TYPE);
    if (!node) return NULL;

    node->valueNode.value = name ? xstrdup(name) : NULL;
    if (name && !node->valueNode.value) {
        free(node);
        fprintf(stderr, "OOM: BasicType name\n");
        return NULL;
    }
    return node;
}


// Inline asm node (own the text; safer than borrowing)
ASTNode* createAsmNode(const char* asmText) {
    ASTNode* node = new_node(AST_ASM);
    if (!node) return NULL;

    node->asmStmt.asmText = asmText ? xstrdup(asmText) : NULL;
    if (asmText && !node->asmStmt.asmText) {
        free(node);
        fprintf(stderr, "OOM: asm text\n");
        return NULL;
    }
    return node;
}


// sizeof(expr or type) node — you currently hang it off expr.left
ASTNode* createSizeofNode(ASTNode* target) {
    ASTNode* node = new_node(AST_SIZEOF);
    if (!node) return NULL;

    node->expr.left  = target;  // keep your existing convention
    node->expr.right = NULL;
    inherit_line_from_node(node, target);
    return node;
}

// Number literal
ASTNode* createNumberLiteralNode(const char* value) {
    ASTNode* node = new_node(AST_NUMBER_LITERAL);
    if (!node) return NULL;

    node->valueNode.value = value ? xstrdup(value) : NULL;
    if (value && !node->valueNode.value) {
        free(node);
        fprintf(stderr, "OOM: number literal\n");
        return NULL;
    }
    return node;
}


// Character literal
ASTNode* createCharLiteralNode(const char* value) {
    ASTNode* node = new_node(AST_CHAR_LITERAL);
    if (!node) return NULL;

    node->valueNode.value = value ? xstrdup(value) : NULL;
    if (value && !node->valueNode.value) {
        free(node);
        fprintf(stderr, "OOM: char literal\n");
        return NULL;
    }
    return node;
}


// String literal
ASTNode* createStringLiteralNode(const char* value) {
    ASTNode* node = new_node(AST_STRING_LITERAL);
    if (!node) return NULL;

    node->valueNode.value = value ? xstrdup(value) : NULL;
    if (value && !node->valueNode.value) {
        free(node);
        fprintf(stderr, "OOM: string literal\n");
        return NULL;
    }
    return node;
}


// Identifier
ASTNode* createIdentifierNode(const char* name) {
    ASTNode* node = new_node(AST_IDENTIFIER);
    if (!node) return NULL;

    node->valueNode.value = name ? xstrdup(name) : NULL;
    if (name && !node->valueNode.value) {
        free(node);
        fprintf(stderr, "OOM: identifier\n");
        return NULL;
    }
    return node;
}


//              BASIC TYPES
// -----------------------------------------------------------------
//              ASSIGNMENTS





// Assignment & Variable Declaration Nodes

ASTNode* createAssignmentNode(ASTNode* identifier, ASTNode* value, TokenType op) {
    ASTNode* node = new_node(AST_ASSIGNMENT);
    if (!node) return NULL;

    const char* op_str = getOperatorString(op);  // assumed provided elsewhere
    node->assignment.op = op_str ? xstrdup(op_str) : NULL;
    if (op_str && !node->assignment.op) {
        free(node);
        fprintf(stderr, "OOM: assignment operator string\n");
        return NULL;
    }

    node->assignment.target = identifier;
    node->assignment.value  = value;
    if (identifier && identifier->line) {
        node->line = identifier->line;
    }
    inherit_line_from_node(node, value);
    return node;
}


ASTNode* createVariableDeclarationNode(ParsedType declaredType,
                                       ASTNode** identifiers,
                                       struct DesignatedInit** initializers,
                                       size_t varCount) {
    ASTNode* node = new_node(AST_VARIABLE_DECLARATION);
    if (!node) return NULL;

    node->varDecl.declaredType = declaredType;   // POD copy-by-value
    node->varDecl.declaredTypes = NULL;
    node->varDecl.varNames     = identifiers;    // arrays owned by AST builder
    node->varDecl.initializers = initializers;   // may be NULL entries
    node->varDecl.varCount     = varCount;
    if (varCount > 0 && identifiers && identifiers[0]) {
        node->line = identifiers[0]->line;
    }
    inherit_line_from_designated_inits(node, initializers, varCount);
    return node;
}


ASTNode* createTypedefNode(ParsedType baseType, ASTNode* alias) {
    ASTNode* node = new_node(AST_TYPEDEF);
    if (!node) return NULL;

    node->typedefStmt.baseType = baseType;  // POD copy
    node->typedefStmt.alias    = alias;     // identifier node (already duped)
    if (alias) node->line = alias->line;
    return node;
}


ASTNode* createEnumDefinitionNode(const char* name,
                                  ASTNode** members,
                                  ASTNode** values,
                                  size_t count) {
    ASTNode* node = new_node(AST_ENUM_DEFINITION);
    if (!node) return NULL;

    node->enumDef.enumName    = createIdentifierNode(name);  // owns its own string
    node->enumDef.members     = members;  // parallel arrays, same length
    node->enumDef.values      = values;   // may contain NULLs for implicit values
    node->enumDef.memberCount = count;
    return node;
}


ASTNode* createStructOrUnionDefinitionNode(ASTNodeType type,
                                           const char* name,
                                           ASTNode** fields,
                                           size_t fieldCount) {
    ASTNode* node = new_node(type); // type must be AST_STRUCT_DEFINITION or AST_UNION_DEFINITION
    if (!node) return NULL;

    node->structDef.structName = createIdentifierNode(name); // owns name
    node->structDef.fields     = fields;     // array of field decl nodes
    node->structDef.fieldCount = fieldCount;
    return node;
}




// Array declaration:  T name[size] = { ... }
ASTNode* createArrayDeclarationNode(ParsedType declaredType,
                                    ASTNode* name, ASTNode* size,
                                    struct DesignatedInit** initValues,
                                    size_t valueCount) {
    ASTNode* node = new_node(AST_ARRAY_DECLARATION);
    if (!node) return NULL;

    node->arrayDecl.declaredType = declaredType;   // POD copy
    node->arrayDecl.varName      = name;
    node->arrayDecl.arraySize    = size;           // may be NULL for unsized
    node->arrayDecl.initializers = initValues;     // may be NULL
    node->arrayDecl.valueCount   = valueCount;     // 0 ok
    node->arrayDecl.isVLA        = false;
    inherit_line_from_node(node, name);
    inherit_line_from_node(node, size);
    inherit_line_from_designated_inits(node, initValues, valueCount);
    return node;
}


 // Link a chain of array sizes (e.g., [][][]), stored via nextStmt
ASTNode* chainArraySizes(ASTNode* outer, ASTNode* inner) {
    if (!outer) return inner;
    ASTNode* cur = outer;
    while (cur->nextStmt) cur = cur->nextStmt;
    cur->nextStmt = inner;   // inner can itself be a chain; last's nextStmt stays NULL
    return outer;
}


// Array access: arr[index]
ASTNode* createArrayAccessNode(ASTNode* array, ASTNode* index) {
    ASTNode* node = new_node(AST_ARRAY_ACCESS);
    if (!node) return NULL;

    node->arrayAccess.array = array;
    node->arrayAccess.index = index;
    inherit_line_from_pair(node, array, index);
    return node;
}


// Member access: base.field  OR  base->field
ASTNode* createMemberAccessNode(TokenType op, ASTNode* base, const char* fieldName) {
    ASTNode* node = new_node((op == TOKEN_ARROW) ? AST_POINTER_ACCESS : AST_DOT_ACCESS);
    if (!node) return NULL;

    node->memberAccess.base  = base;
    node->memberAccess.field = fieldName ? xstrdup(fieldName) : NULL;
    if (fieldName && !node->memberAccess.field) {
        free(node); fprintf(stderr, "OOM: member field\n"); return NULL;
    }
    inherit_line_from_node(node, base);
    return node;
}


// *operand
ASTNode* createPointerDereferenceNode(ASTNode* operand) {
    ASTNode* node = new_node(AST_POINTER_DEREFERENCE);
    if (!node) return NULL;

    node->pointerDeref.pointer = operand;
    inherit_line_from_node(node, operand);
    return node;
}


// if (cond) thenBody;
ASTNode* createIfStatementNode(ASTNode* condition, ASTNode* thenBody) {
    ASTNode* node = new_node(AST_IF_STATEMENT);
    if (!node) return NULL;

    node->ifStmt.condition  = condition;
    node->ifStmt.thenBranch = thenBody;
    node->ifStmt.elseBranch = NULL;
    inherit_line_from_pair(node, condition, thenBody);
    return node;
}


// if (cond) thenBody; else elseBody;
ASTNode* createIfElseStatementNode(ASTNode* condition, ASTNode* thenBody, ASTNode* elseBody) {
    ASTNode* node = new_node(AST_IF_STATEMENT);
    if (!node) return NULL;

    node->ifStmt.condition  = condition;
    node->ifStmt.thenBranch = thenBody;
    node->ifStmt.elseBranch = elseBody;
    inherit_line_from_node(node, condition);
    inherit_line_from_node(node, thenBody);
    inherit_line_from_node(node, elseBody);
    return node;
}


// while (cond) body;   or   do { body } while (cond);
ASTNode* createWhileLoopNode(ASTNode* condition, ASTNode* body, int isDoWhile) {
    ASTNode* node = new_node(AST_WHILE_LOOP);
    if (!node) return NULL;

    node->whileLoop.condition = condition;
    node->whileLoop.body      = body;
    node->whileLoop.isDoWhile = isDoWhile;  // 0 = while, 1 = do-while
    inherit_line_from_pair(node, condition, body);
    return node;
}


// for (init; cond; inc) body;
ASTNode* createForLoopNode(ASTNode* init, ASTNode* condition, ASTNode* increment, ASTNode* body) {
    ASTNode* node = new_node(AST_FOR_LOOP);
    if (!node) return NULL;

    node->forLoop.initializer = init;       // may be decl or expr (or NULL)
    node->forLoop.condition   = condition;  // may be NULL
    node->forLoop.increment   = increment;  // may be NULL
    node->forLoop.body        = body;       // must be a stmt/block node
    inherit_line_from_node(node, init);
    inherit_line_from_node(node, condition);
    inherit_line_from_node(node, increment);
    inherit_line_from_node(node, body);
    return node;
}




//              TESTS AND LOOPS
// -----------------------------------------------------------------
//              CONTROL




// Flow Control Nodes

ASTNode* createReturnNode(ASTNode* expr) {
    ASTNode* node = new_node(AST_RETURN);
    if (!node) return NULL;
    node->returnStmt.returnValue = expr;  // NULL for 'return;'
    inherit_line_from_node(node, expr);
    return node;
}

ASTNode* createBreakNode(void) {
    ASTNode* node = new_node(AST_BREAK);
    if (!node) return NULL;
    node->loopControl.isBreak    = 1;     // 1 = break
    node->loopControl.loopTarget = NULL;  // filled later if you thread targets
    return node;
}

ASTNode* createContinueNode(void) {
    ASTNode* node = new_node(AST_CONTINUE);
    if (!node) return NULL;
    node->loopControl.isBreak    = 0;     // 0 = continue
    node->loopControl.loopTarget = NULL;
    return node;
}

ASTNode* createGotoStatementNode(const char* label) {
    ASTNode* node = new_node(AST_GOTO_STATEMENT);
    if (!node) return NULL;

    node->gotoStmt.label = label ? xstrdup(label) : NULL;
    if (label && !node->gotoStmt.label) {
        free(node);
        fprintf(stderr, "OOM: goto label\n");
        return NULL;
    }
    return node;
}

ASTNode* createLabelDeclarationNode(const char* labelName, ASTNode* statement) {
    ASTNode* node = new_node(AST_LABEL_DECLARATION);
    if (!node) return NULL;

    node->label.labelName = labelName ? xstrdup(labelName) : NULL;
    if (labelName && !node->label.labelName) {
        free(node);
        fprintf(stderr, "OOM: label name\n");
        return NULL;
    }
    node->label.statement = statement;  // the labeled statement node
    inherit_line_from_node(node, statement);
    return node;
}




//              CONTROL
// -----------------------------------------------------------------
//              FUNCTIONS




ASTNode* createFunctionDeclarationNode(ParsedType returnType,
                                       ASTNode* funcName,
                                       ASTNode** paramList,
                                       size_t paramCount,
                                       bool isVariadic) {
    ASTNode* node = new_node(AST_FUNCTION_DECLARATION);
    if (!node) return NULL;

    node->functionDecl.returnType = returnType;               // POD copy
    node->functionDecl.funcName   = funcName;
    node->functionDecl.parameters = paramList;
    node->functionDecl.paramCount = paramCount;
    node->functionDecl.isVariadic = isVariadic;
    inherit_line_from_node(node, node->functionDecl.funcName);
    return node;
}


ASTNode* createFunctionDefinitionNode(ParsedType returnType,
                                      ASTNode* funcName,
                                      ASTNode** paramList,
                                      ASTNode* body,
                                      size_t paramCount,
                                      bool isVariadic) {
    ASTNode* node = new_node(AST_FUNCTION_DEFINITION);
    if (!node) return NULL;

    node->functionDef.returnType = returnType;   // POD copy
    node->functionDef.funcName   = funcName;     // should be an AST_IDENTIFIER node
    node->functionDef.parameters = paramList;    // array of param decl nodes (can be NULL if 0)
    node->functionDef.paramCount = paramCount;
    node->functionDef.body       = body;         // block or single statement node
    node->functionDef.isVariadic = isVariadic;
    inherit_line_from_node(node, funcName);
    inherit_line_from_array(node, paramList, paramCount);
    inherit_line_from_node(node, body);
    return node;
}


ASTNode* createFunctionCallNode(ASTNode* callee, ASTNode** arguments, size_t argumentCount) {
    ASTNode* node = new_node(AST_FUNCTION_CALL);
    if (!node) return NULL;

    node->functionCall.callee        = callee;       // identifier / member / pointer call node
    node->functionCall.arguments     = arguments;    // array (not list) is fine
    node->functionCall.argumentCount = argumentCount;
    inherit_line_from_node(node, callee);
    inherit_line_from_array(node, arguments, argumentCount);
    return node;
}


ASTNode* createFunctionPointerDeclarationNode(ParsedType returnType,
                                              ASTNode* name,
                                              ASTNode** params,
                                              size_t paramCount,
                                              ASTNode* initializer) {
    ASTNode* node = new_node(AST_FUNCTION_POINTER);
    if (!node) return NULL;

    node->functionPointer.returnType  = returnType;   // your ParsedType should already encode fp-ness
    node->functionPointer.name        = name;         // identifier node
    node->functionPointer.parameters  = params;       // array of param decl nodes
    node->functionPointer.paramCount  = paramCount;
    node->functionPointer.initializer = initializer;  // optional initializer (e.g., &foo)
    inherit_line_from_node(node, name);
    inherit_line_from_array(node, params, paramCount);
    inherit_line_from_node(node, initializer);
    return node;
}








//              FUNCTIONS
// ----------------------------------------------------------------- 
//              SWITCHES




// Creates a Case node (case value and its associated block)
// NOTE: caseBody is an array (or pointer to first stmt) per your current design.
ASTNode* createCaseNode(ASTNode* caseValue, ASTNode** caseBody) {
    ASTNode* node = new_node(AST_CASE);
    if (!node) return NULL;

    node->caseStmt.caseValue = caseValue; // NULL means 'default'
    node->caseStmt.caseBody  = caseBody;  // may be NULL / empty
    node->caseStmt.nextCase  = NULL;      // explicit, though calloc already zeros
    inherit_line_from_node(node, caseValue);
    if (!node->line && caseBody && caseBody[0]) {
        inherit_line_from_node(node, caseBody[0]);
    }
    return node;
}

// Creates a Switch node (switch expression and case list)
ASTNode* createSwitchNode(ASTNode* condition, ASTNode** caseList) {
    ASTNode* node = new_node(AST_SWITCH);
    if (!node) return NULL;

    node->switchStmt.condition = condition; // expression inside 'switch (...)'
    node->switchStmt.caseList  = caseList;  // array (or head) of case nodes
    inherit_line_from_node(node, condition);
    if (!node->line && caseList && caseList[0]) {
        inherit_line_from_node(node, caseList[0]);
    }
    return node;
}


// A node that wraps an already-parsed type (NOT just a basic-type name).
ASTNode* createParsedTypeNode(ParsedType type) {
    ASTNode* node = new_node(AST_PARSED_TYPE);  
    if (!node) return NULL;

    node->parsedTypeNode.parsed = type; // POD copy-by-value
    return node;
}
