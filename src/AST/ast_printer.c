#include "ast_printer.h"
#include "Parser/Helpers/designated_init.h"
#include <stdio.h>

static void printParsedType_inner(const ParsedType* pt);
static void printNodeAttributes(ASTNode* node, int depth);

static const char* attributeSyntaxName(ASTAttributeSyntax syntax) {
    switch (syntax) {
        case AST_ATTRIBUTE_SYNTAX_GNU: return "GNU";
        case AST_ATTRIBUTE_SYNTAX_CXX: return "CXX";
        case AST_ATTRIBUTE_SYNTAX_DECLSPEC: return "DECLSPEC";
        default: return "UNKNOWN";
    }
}

static void printNodeAttributes(ASTNode* node, int depth) {
    if (!node || node->attributeCount == 0 || !node->attributes) {
        return;
    }
    for (size_t i = 0; i < node->attributeCount; ++i) {
        ASTAttribute* attr = node->attributes[i];
        if (!attr) continue;
        for (int j = 0; j < depth; ++j) {
            printf("  ");
        }
        const char* payload = attr->payload ? attr->payload : "";
        printf("ATTRIBUTE [%s] %s\n", attributeSyntaxName(attr->syntax), payload);
    }
}

void printParsedType(const ParsedType* pt) {
    printParsedType_inner(pt);
    printf("\n");
}

static void printParsedType_inner(const ParsedType* pt) {
    if (!pt) { printf("<null-type>"); return; }

    /* Storage class */
    if (pt->isStatic)   printf("static ");
    if (pt->isExtern)   printf("extern ");
    if (pt->isRegister) printf("register ");
    if (pt->isAuto)     printf("auto ");

    /* Qualifiers / modifiers */
    if (pt->isConst)    printf("const ");
    if (pt->isSigned)   printf("signed ");
    if (pt->isUnsigned) printf("unsigned ");
    if (pt->isShort)    printf("short ");
    if (pt->isLong)     printf("long ");
    if (pt->isVolatile) printf("volatile ");
    if (pt->isRestrict) printf("restrict ");
    if (pt->isInline)   printf("inline ");

    /* Base name: primitive or user-defined (struct/union/name) */
    if (pt->kind == TYPE_PRIMITIVE) {
        printf("%s", getTokenTypeName(pt->primitiveType));
    } else {
        if (pt->tag == TAG_STRUCT) printf("struct ");
        if (pt->tag == TAG_UNION)  printf("union ");
        printf("%s", pt->userTypeName ? pt->userTypeName : "<anon>");
    }

    /* Declarator part */
    if (pt->isFunctionPointer) {
        /* Render as: <base> ( <stars> )(param, ...) */
        printf(" (");
        if (pt->derivationCount > 0) {
            bool sawPointer = false;
            for (size_t i = 0; i < pt->derivationCount; ++i) {
                const TypeDerivation* deriv = parsedTypeGetDerivation(pt, i);
                if (!deriv || deriv->kind != TYPE_DERIVATION_POINTER) {
                    continue;
                }
                sawPointer = true;
                printf("*");
                if (deriv->as.pointer.isConst) printf(" const");
                if (deriv->as.pointer.isVolatile) printf(" volatile");
                if (deriv->as.pointer.isRestrict) printf(" restrict");
            }
            if (!sawPointer) printf("*");
        } else if (pt->pointerDepth > 0) {
            for (int i = 0; i < pt->pointerDepth; i++) printf("*");
        } else {
            /* function pointers should always have at least one star,
               but guard just in case: */
            printf("*");
        }
        printf(")(");
        for (size_t i = 0; i < pt->fpParamCount; i++) {
            if (i) printf(", ");
            printParsedType_inner(&pt->fpParams[i]);
        }
        printf(")");
    } else {
        if (pt->derivationCount > 0) {
            for (size_t i = 0; i < pt->derivationCount; ++i) {
                const TypeDerivation* deriv = parsedTypeGetDerivation(pt, i);
                if (!deriv || deriv->kind != TYPE_DERIVATION_POINTER) {
                    continue;
                }
                printf("*");
                if (deriv->as.pointer.isConst) printf(" const");
                if (deriv->as.pointer.isVolatile) printf(" volatile");
                if (deriv->as.pointer.isRestrict) printf(" restrict");
            }
        } else {
            for (int i = 0; i < pt->pointerDepth; i++) printf("*");
        }
    }

    for (size_t i = 0; i < pt->derivationCount; ++i) {
        const TypeDerivation* deriv = parsedTypeGetDerivation(pt, i);
        if (!deriv) continue;
        switch (deriv->kind) {
            case TYPE_DERIVATION_ARRAY:
                printf(" [");
                if (deriv->as.array.isVLA) {
                    printf("VLA");
                } else if (deriv->as.array.isFlexible) {
                    printf("flex");
                } else if (deriv->as.array.hasConstantSize) {
                    printf("%lld", deriv->as.array.constantSize);
                } else if (deriv->as.array.sizeExpr) {
                    if (deriv->as.array.sizeExpr->type == AST_NUMBER_LITERAL) {
                        printf("%s", deriv->as.array.sizeExpr->valueNode.value
                            ? deriv->as.array.sizeExpr->valueNode.value
                            : "");
                    } else {
                        printBasicAST(deriv->as.array.sizeExpr, 0, true);
                    }
                } else {
                    printf("?");
                }
                printf("]");
                break;
            case TYPE_DERIVATION_FUNCTION: {
                printf(" (");
                for (size_t p = 0; p < deriv->as.function.paramCount; ++p) {
                    if (p) printf(", ");
                    printParsedType_inner(&deriv->as.function.params[p]);
                }
                if (deriv->as.function.isVariadic) {
                    if (deriv->as.function.paramCount > 0) {
                        printf(", ");
                    }
                    printf("...");
                }
                printf(")");
                break;
            }
            case TYPE_DERIVATION_POINTER:
            default:
                break;
        }
    }
}


void printDesignatedInit(DesignatedInit* init, int depth) {
    for (int i = 0; i < depth; i++) printf("  ");
    
    // Print field or index designation
    if (init->fieldName) {
        printf(".%s = ", init->fieldName);
    } else if (init->indexExpr) {
        printf("[");
        printBasicAST(init->indexExpr, 0, true);
        printf("] = ");
    } else {
        printf("= ");
    }
     
    // Print the expression or compound value
    if (init->expression) {
        if (init->expression->type == AST_COMPOUND_LITERAL) {
            printf("\n");
            for (size_t i = 0; i < init->expression->compoundLiteral.entryCount; i++) {
                printDesignatedInit(init->expression->compoundLiteral.entries[i], depth + 1);
            }
        } else {
            bool simple =
                init->expression->type == AST_NUMBER_LITERAL ||
                init->expression->type == AST_IDENTIFIER ||
                init->expression->type == AST_STRING_LITERAL ||
                init->expression->type == AST_CHAR_LITERAL;
                
            printBasicAST(init->expression, simple ? 0 : depth + 1, !simple);
            if (simple) printf("\n");
        }
    } else {
        printf("NULL\n");
    }
}

void printBasicAST(struct ASTNode* node, int depth, bool inlineMode) {
    if (!node) return;
        
    if (!inlineMode) {
        for (int i = 0; i < depth; i++) printf("    ");
    } 
        
    switch (node->type) {
        case AST_NUMBER_LITERAL:
            printf("NUMBER_LITERAL %s", node->valueNode.value);
            break;
        case AST_STRING_LITERAL: {
            const char* payload = NULL;
            ast_literal_encoding(node->valueNode.value, &payload);
            printf("STRING_LITERAL \"%s\"", payload ? payload : node->valueNode.value);
            break;
        }
        case AST_CHAR_LITERAL: {
            const char* payload = NULL;
            ast_literal_encoding(node->valueNode.value, &payload);
            printf("CHAR_LITERAL '%s'", payload ? payload : node->valueNode.value);
            break;
        }
        case AST_IDENTIFIER:
            printf("IDENTIFIER %s", node->valueNode.value);
            break;
        case AST_UNARY_EXPRESSION:
            if (inlineMode) {
                const char* op = node->expr.op ? node->expr.op : "";
                if (node->expr.isPostfix) {
                    printBasicAST(node->expr.left, depth, true);
                    printf("%s", op);
                } else {
                    printf("%s", op);
                    printBasicAST(node->expr.left, depth, true);
                }
                break;
            }
            printAST(node, depth);
            return;
        case AST_COMMA_EXPRESSION:
            printf("COMMA EXPRESSION");
            if (!inlineMode) printf("\n");
            for (size_t i = 0; i < node->commaExpr.exprCount; i++) {
                printBasicAST(node->commaExpr.expressions[i], depth + 1, false);
            }
            return;
        default:
            // Fall back to full tree printing
            if (inlineMode) {
                printf("(Unrecognized for inline: type %d)", node->type);
            } else {
                printAST(node, depth);
            }
            return;
    }

    if (!inlineMode) {
        printf("\n");
    }
}







void printAST(ASTNode* node, int depth) {
    if (!node) return;
    printNodeAttributes(node, depth);
    // Indentation for tree-like structure
    for (int i = 0; i < depth; i++) {
        printf("  ");
    }
            
    // Print node type
    switch (node->type) {
        case AST_PROGRAM:
            printf("PROGRAM\n");
            for (size_t i = 0; i < node->block.statementCount; i++) {
                printAST(node->block.statements[i], depth + 1);
            }
            break;
            
        case AST_BLOCK:
            printf("BLOCK\n");
            for (size_t i = 0; i < node->block.statementCount; i++) {
                printAST(node->block.statements[i], depth + 1);
            }
            break;

        case AST_STATEMENT_EXPRESSION:
            printf("STATEMENT_EXPRESSION\n");
            if (node->statementExpr.block) {
                printAST(node->statementExpr.block, depth + 1);
            }
            break;
            
        case AST_INCLUDE_DIRECTIVE:
            printf("INCLUDE_DIRECTIVE\n");
            for (int i = 0; i < depth + 1; i++) printf("  ");
            printf("File: %s (%s)\n",
                   node->includeDirective.filePath,
                   node->includeDirective.isSystem ? "system" : "user");
            break;
                    
        case AST_DEFINE_DIRECTIVE:
            printf("DEFINE_DIRECTIVE\n");
            for (int i = 0; i < depth + 1; i++) printf("  ");
            printf("Macro: %s\n", node->defineDirective.macroName);
            if (node->defineDirective.macroValue) {
                for (int i = 0; i < depth + 1; i++) printf("  ");  
                printf("Value: %s\n", node->defineDirective.macroValue);
            }
            break;
                
        case AST_CONDITIONAL_DIRECTIVE:
            printf("CONDITIONAL_DIRECTIVE\n");
                    
            for (int i = 0; i < depth + 1; i++) printf("  ");
            printf("%s %s\n",
                   node->conditionalDirective.isNegated ? "#ifndef" : "#ifdef",
                   node->conditionalDirective.symbol);
            
            if (node->conditionalDirective.body) {
                ASTNode* block = node->conditionalDirective.body;
            
                // If it's a BLOCK node, print each child at depth + 1
                if (block->type == AST_BLOCK) {
                    for (size_t i = 0; i < block->block.statementCount; i++) {
                        printAST(block->block.statements[i], depth + 1);
                    }
                } else {
                    // Print whole body otherwise
                    printAST(block, depth + 1);
                }
            }
            break;

        case AST_ENDIF_DIRECTIVE:
            for (int i = 0; i < depth; i++) printf("  ");
            printf("ENDIF_DIRECTIVE\n");
            break;
            
            
        case AST_UNION_DEFINITION:
            printf("UNION\n");
            
            for (int i = 0; i < depth + 1; i++) printf("    ");
            printf("Name: ");
            printAST(node->structDef.structName, depth + 1);
            
            for (size_t i = 0; i < node->structDef.fieldCount; i++) {
                printAST(node->structDef.fields[i], depth + 1);
            }
            break;
            
            
        case AST_STRUCT_DEFINITION:
            printf("STRUCT\n");
            
            for (int i = 0; i < depth + 1; i++) printf("    ");
            printf("Name: ");
            printAST(node->structDef.structName, depth + 1);
            
            for (size_t i = 0; i < node->structDef.fieldCount; i++) {
                printAST(node->structDef.fields[i], depth + 1);
            }
            break;
            
        case AST_TYPEDEF:
            printf("TYPEDEF\n");
            
            // Print base type
            for (int i = 0; i < depth + 1; i++) printf("    ");
            printf("TYPE: ");
            printParsedType(&node->typedefStmt.baseType);
            
            // Print alias name
            for (int i = 0; i < depth + 1; i++) printf("    ");
            printf("Alias: ");
            printAST(node->typedefStmt.alias, depth + 1);
            break;
            
        case AST_ENUM_DEFINITION:
            printf("ENUM\n");
            
            for (int i = 0; i < depth + 1; i++) printf("    ");
            printf("Name: ");
            printAST(node->enumDef.enumName, depth + 1);
            
            for (size_t i = 0; i < node->enumDef.memberCount; i++) {
                for (int j = 0; j < depth + 1; j++) printf("    "); 
                printf("Member: ");
                printAST(node->enumDef.members[i], depth);
                
                if (node->enumDef.values[i]) {
                    for (int j = 0; j < depth + 2; j++) printf("    ");
                    printf("Value:");
                    printAST(node->enumDef.values[i], depth);
                }
            }
            break;
            
            
        case AST_ASSIGNMENT:
            printf("ASSIGNMENT (%s)\n", node->assignment.op);
            printAST(node->assignment.target, depth + 1);
            printAST(node->assignment.value, depth + 1); 
            break;
            
        case AST_VARIABLE_DECLARATION:
            printf("VAR_DECL\n");
            ParsedType* perTypes = node->varDecl.declaredTypes;
            for (size_t i = 0; i < node->varDecl.varCount; i++) {
                for (int indent = 0; indent < depth + 1; indent++) printf("  ");
                printf("TYPE: ");
                const ParsedType* pt = perTypes ? &perTypes[i] : &node->varDecl.declaredType;
                printParsedType(pt);

                if (node->varDecl.varNames[i]) {
                    printAST(node->varDecl.varNames[i], depth + 1);
                } else {
                    for (int indent = 0; indent < depth + 1; indent++) printf("  ");
                    printf("IDENTIFIER <unnamed>\n");
                }

                if (node->varDecl.bitFieldWidth) {
                    for (int indent = 0; indent < depth + 2; indent++) printf("  ");
                    printf("BITFIELD : ");
                    bool simpleWidth =
                        node->varDecl.bitFieldWidth->type == AST_NUMBER_LITERAL ||
                        node->varDecl.bitFieldWidth->type == AST_IDENTIFIER ||
                        node->varDecl.bitFieldWidth->type == AST_CHAR_LITERAL ||
                        node->varDecl.bitFieldWidth->type == AST_STRING_LITERAL ||
                        node->varDecl.bitFieldWidth->type == AST_UNARY_EXPRESSION;
                    if (simpleWidth) {
                        printBasicAST(node->varDecl.bitFieldWidth, 0, true);
                        printf("\n");
                    } else {
                        printf("\n");
                        printAST(node->varDecl.bitFieldWidth, depth + 3);
                    }
                }

                DesignatedInit* init = node->varDecl.initializers[i];
                if (!init) continue;
                
                if (init->expression && init->expression->type == AST_COMPOUND_LITERAL) {
                    // Compound literal (e.g., .values = { ... })
                    if (init->fieldName) {
                        for (int d = 0; d < depth + 2; d++) printf("  ");
                        printf(".%s = \n", init->fieldName);
                    }
                     
                    for (size_t j = 0; j < init->expression->compoundLiteral.entryCount; j++) {
                        printDesignatedInit(init->expression->compoundLiteral.entries[j],
                                                depth + 3);
                    }
                     
                } else {
                    // Simple assignment (e.g., int x = 5;)
                    for (int d = 0; d < depth + 2; d++) printf("  ");
                    if (init->fieldName) {
                        printf(".%s = ", init->fieldName);
                    } else {
                        printf("= ");
                    }
                    bool simple =
                        init->expression &&
                        (init->expression->type == AST_NUMBER_LITERAL ||
                         init->expression->type == AST_IDENTIFIER ||
                         init->expression->type == AST_STRING_LITERAL ||
                         init->expression->type == AST_CHAR_LITERAL);

                    if (simple) {
                        printBasicAST(init->expression, 0, true);
                        printf("\n");
                    } else {
                        printf("\n");
                        printAST(init->expression, depth + 3);
                    }
                }
            }
            break;

        case AST_ARRAY_ACCESS:
                    printf("ARRAY_ACCESS\n");
            printAST(node->arrayAccess.array, depth + 1);  // Print array being accessed
            printAST(node->arrayAccess.index, depth + 1);  // Print index
            break;
            
        case AST_POINTER_ACCESS:
            printf("POINTER_ACCESS '->'\n");
            printAST(node->memberAccess.base, depth + 1);
            for (int i = 0; i < depth + 1; i++) printf("  ");
            printf("FIELD %s\n", node->memberAccess.field);  
            break;
            
        case AST_DOT_ACCESS:
            printf("DOT_ACCESS '.'\n");
            printAST(node->memberAccess.base, depth + 1);
            for (int i = 0; i < depth + 1; i++) printf("  ");
            printf("FIELD %s\n", node->memberAccess.field);  
            break;
            
        case AST_POINTER_DEREFERENCE:
                printf("POINTER_DEREFERENCE\n");
                printAST(node->pointerDeref.pointer, depth + 1);
                break;
                


        case AST_FUNCTION_DECLARATION:
            printf("FUNCTION DECLARATION\n");
            
            for (int i = 0; i < depth + 1; i++) printf("  ");
            printf("RETURN TYPE: ");
            printParsedType(&node->functionDecl.returnType);
            
            printAST(node->functionDecl.funcName, depth + 1);
            
            if (node->functionDecl.paramCount > 0) {
                for (int i = 0; i < depth + 1; i++) printf("  ");
                printf("PARAMETERS:\n");
                for (size_t i = 0; i < node->functionDecl.paramCount; i++) {
                    printAST(node->functionDecl.parameters[i], depth + 2);  
                }
            }
            if (node->functionDecl.isVariadic) {
                for (int i = 0; i < depth + 1; i++) printf("  ");
                printf("VARIADIC: yes\n");
            }
             
            break;
            
            
        case AST_FUNCTION_DEFINITION:
            printf("FUNCTION DEFINITION\n");
            
            for (int i = 0; i < depth + 1; i++) printf("  ");
            printf("RETURN TYPE: ");
            printParsedType(&node->functionDef.returnType);
            
            printAST(node->functionDef.funcName, depth + 1);
            
            if (node->functionDef.paramCount > 0) {
                for (int i = 0; i < depth + 1; i++) printf("  ");
                printf("PARAMETERS\n");
                for (size_t i = 0; i < node->functionDef.paramCount; i++) {
                    printAST(node->functionDef.parameters[i], depth + 2);  
                }
            }
            if (node->functionDef.isVariadic) {
                for (int i = 0; i < depth + 1; i++) printf("  ");
                printf("VARIADIC: yes\n");
            }
             
            printAST(node->functionDef.body, depth + 1);
            break;
            
        case AST_FUNCTION_CALL:
            printf("FUNCTION CALL\n");
            printAST(node->functionCall.callee, depth + 1);
            
            if (node->functionCall.argumentCount > 0) {
                for (int i = 0; i < depth + 1; i++) printf("  ");
                printf("ARGUMENTS\n");
                for (size_t i = 0; i < node->functionCall.argumentCount; i++) {
                    printAST(node->functionCall.arguments[i], depth + 2);
                }
            }
            break;

        case AST_FUNCTION_POINTER: {
            printf("FUNCTION POINTER DECLARATION\n");
            
            // Return Type
            for (int i = 0; i < depth + 1; i++) printf("  ");
            printf("RETURN TYPE: ");
            printParsedType(&node->functionPointer.returnType);
            
            // Name
            for (int i = 0; i < depth + 1; i++) printf("  ");
            printf("NAME: %s\n", node->functionPointer.name->valueNode.value);
            
            // Parameters
            if (node->functionPointer.paramCount > 0) {
                for (int i = 0; i < depth + 1; i++) printf("  ");
                printf("PARAMETERS:\n");
                for (size_t i = 0; i < node->functionPointer.paramCount; i++) {
                    printAST(node->functionPointer.parameters[i], depth + 2);  
                }
            }
            break;
        }
         
         
         
         
         
        case AST_TERNARY_EXPRESSION:
            printf("TERNARY_EXPRESSION\n");
            
            // Print the condition, true expression, and false expression
            printAST(node->ternaryExpr.condition, depth + 1);
            
            printAST(node->ternaryExpr.trueExpr, depth + 1);
            
            printAST(node->ternaryExpr.falseExpr, depth + 1);
            break;
            
        case AST_BINARY_EXPRESSION:
            printf("BINARY EXPRESSION '%s'\n", node->expr.op);
            printAST(node->expr.left, depth + 1);
            printAST(node->expr.right, depth + 1);
            break;
            
        case AST_UNARY_EXPRESSION:
            printf("UNARY EXPRESSION '%s'%s\n",
                   node->expr.op,
                   node->expr.isPostfix ? " (postfix)" : " (prefix)");
            printAST(node->expr.left, depth + 1);  // Operand is stored in `left`
            break;
            

	case AST_COMMA_EXPRESSION:
	    printf("COMMA_EXPRESSION (%zu expressions)\n", node->commaExpr.exprCount);
	    for (size_t i = 0; i < node->commaExpr.exprCount; i++) {
	        for (int j = 0; j < depth + 1; j++) printf("  ");
	        printf("Expr[%zu]:\n", i);
	        printAST(node->commaExpr.expressions[i], depth + 2);
	    }
	    break;


        case AST_CAST_EXPRESSION:
            printf("CAST_EXPRESSION\n");
            
            // Print type
            for (int i = 0; i < depth + 1; i++) printf("    ");
            printf("CAST TYPE: ");
            printParsedType(&node->castExpr.castType);
            
            // Print the expression being cast
            printAST(node->castExpr.expression, depth + 1);
            break;


	case AST_COMPOUND_LITERAL:
	    printf("COMPOUND_LITERAL\n");
	
	    // Print its type
	    for (int i = 0; i < depth + 1; i++) printf("  ");
	    printf("TYPE: ");
	    printParsedType(&node->compoundLiteral.literalType);
	
	    // Print entries
	    for (size_t i = 0; i < node->compoundLiteral.entryCount; i++) {
	        printDesignatedInit(node->compoundLiteral.entries[i], depth + 1);
	    }
	    break;







        case AST_RETURN:
            printf("RETURN\n");
            printAST(node->returnStmt.returnValue, depth + 1);
            break;
            
        case AST_BREAK:
            printf("BREAK\n");
            break;
            
        case AST_CONTINUE:
            printf("CONTINUE\n");
            break;
            
        case AST_GOTO_STATEMENT:
            printf("GOTO\n");   
            for (int i = 0; i < depth + 1; i++) printf("  ");
            printf("LABEL %s\n", node->gotoStmt.label);
            break;
            
        case AST_LABEL_DECLARATION:
            printf("LABEL: %s\n", node->label.labelName);
            if (node->label.statement) {
                printAST(node->label.statement, depth + 1);
            }
            break;
            
            
            
            
        case AST_IF_STATEMENT:
            printf("IF_STATEMENT\n");
            printAST(node->ifStmt.condition, depth + 1);
            printAST(node->ifStmt.thenBranch, depth + 1);
            
            if (node->ifStmt.elseBranch) {
                if (node->ifStmt.elseBranch->type == AST_IF_STATEMENT) {
                    printf("%*sELSE_IF_STATEMENT\n", depth * 2, "");
                } else {
                    printf("%*sELSE_STATEMENT\n", depth * 2, "");
                }
                printAST(node->ifStmt.elseBranch, depth + 1);
            }
            break;
            
            
        case AST_WHILE_LOOP:
                if (node->whileLoop.isDoWhile) {
                    printf("DO_WHILE_LOOP\n");  
                    printAST(node->whileLoop.body, depth + 1);   // Print body first
                    printAST(node->whileLoop.condition, depth + 1);  // Then condition
                } else {
                    printf("WHILE_LOOP\n");
                     printAST(node->whileLoop.condition, depth + 1);  // Print condition first)
                    printAST(node->whileLoop.body, depth + 1);
                }
                break;

       case AST_FOR_LOOP:
            printf("FOR_LOOP\n");
            printAST(node->forLoop.initializer, depth + 1);
            printAST(node->forLoop.condition, depth + 1);  
            
            ASTNode* inc = node->forLoop.increment;
            while (inc) {
                printAST(inc, depth + 1);
                inc = inc->nextStmt;
            }
             
            printAST(node->forLoop.body, depth + 1);
            break;
            
            
            
            
        case AST_SWITCH:
            printf("SWITCH\n");
            printAST(node->switchStmt.condition, depth + 1);
            
            for (size_t i = 0; i < node->switchStmt.caseListSize; i++) {
                printAST(node->switchStmt.caseList[i], depth + 1);
            }
            break;
            
        case AST_CASE:
                printf("CASE\n");
                
                    // Print the case value (could be a number or a default case)
                if (node->caseStmt.caseValue) {
                    printAST(node->caseStmt.caseValue, depth + 1);
                } else {
                    for (int i = 0; i < depth + 1; i++) printf("  ");
                    printf("DEFAULT\n");
                }
                 
                // Iterate over all statements in the case body
                for (size_t i = 0; i < node->caseStmt.caseBodySize; i++) {
                    printAST(node->caseStmt.caseBody[i], depth + 1);
                }
                 
                // Move to the next case in the switch statement
                if (node->caseStmt.nextCase) {
                    printAST(node->caseStmt.nextCase, depth);
                }
                break;
                
                
                
        case AST_ASM:
            printf("ASM STATEMENT\n");
            for (int i = 0; i < depth + 1; i++) printf("    ");
            printf("CODE: %s\n", node->asmStmt.asmText);
            break;
            
        case AST_SIZEOF:
            printf("SIZEOF\n");
            
            for (int i = 0; i < depth + 1; i++) printf("  ");
            
            if (node->expr.left->type == AST_BASIC_TYPE) {
                ParsedType pt = node->expr.left->parsedTypeNode.parsed;
                
                for (int i = 0; i < depth + 1; i++) printf("  ");
                printParsedType(&pt);
            } else {
                printAST(node->expr.left, depth + 1);
            }
             
            break;
        case AST_ALIGNOF:
            printf("ALIGNOF\n");
            for (int i = 0; i < depth + 1; i++) printf("  ");
            if (node->expr.left->type == AST_BASIC_TYPE) {
                ParsedType pt = node->expr.left->parsedTypeNode.parsed;
                for (int i = 0; i < depth + 1; i++) printf("  ");
                printParsedType(&pt);
            } else {
                printAST(node->expr.left, depth + 1);
            }
            break;


        case AST_NUMBER_LITERAL:
            printf("NUMBER_LITERAL %s\n", node->valueNode.value);
            break;
            
        case AST_CHAR_LITERAL: {
            const char* payload = NULL;
            ast_literal_encoding(node->valueNode.value, &payload);
            printf("CHAR_LITERAL %s\n", payload ? payload : node->valueNode.value);
            break;
        }
            
        case AST_STRING_LITERAL: {
            const char* payload = NULL;
            ast_literal_encoding(node->valueNode.value, &payload);
            printf("STRING_LITERAL %s\n", payload ? payload : node->valueNode.value);
            break;
        }
            
        case AST_IDENTIFIER:
            printf("IDENTIFIER %s\n", node->valueNode.value);
            break;
            
        case AST_BASIC_TYPE:
            printf("BASIC_TYPE %s\n", node->valueNode.value);
            break;

        case AST_PARSED_TYPE: {
	    printf("PARSED_TYPE ");
	    printParsedType(&node->parsedTypeNode.parsed);
	    break;
	}
            
        default:
            printf("UNKNOWN NODE TYPE\n");
            break;
    }
}
