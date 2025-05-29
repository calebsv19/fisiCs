
#include "code_gen.h"

// LLVM Context & Builder (Global for IR generation)
LLVMModuleRef TheModule;
LLVMBuilderRef Builder;

static LLVMBasicBlockRef loopBreakTarget = NULL;
static LLVMBasicBlockRef loopContinueTarget = NULL;


void initializeLLVM(void) {
    TheModule = LLVMModuleCreateWithName("my_compiler_module");
    Builder = LLVMCreateBuilder();
}

// Main Handler method
LLVMValueRef codegen(ASTNode *node) {
    if (!node) {
        fprintf(stderr, "Error: NULL node in codegen\n");
        return NULL;
    }

    switch (node->type) {
        case AST_PROGRAM:
            return codegenProgram(node);
        case AST_BLOCK:
            return codegenBlock(node);
        case AST_BINARY_EXPRESSION:
            return codegenBinaryExpression(node);
        case AST_UNARY_EXPRESSION:
            return codegenUnaryExpression(node);
        case AST_TERNARY_EXPRESSION:
            return codegenTernaryExpression(node);
        case AST_ASSIGNMENT:
            return codegenAssignment(node);
        case AST_VARIABLE_DECLARATION:
            return codegenVariableDeclaration(node);
        case AST_ARRAY_DECLARATION:
            return codegenArrayDeclaration(node);
        case AST_ARRAY_ACCESS:
            return codegenArrayAccess(node);
        case AST_POINTER_DEREFERENCE:
            return codegenPointerDereference(node);
	case AST_POINTER_ACCESS:
            return codegenPointerAccess(node);   
        case AST_DOT_ACCESS:
            return codegenDotAccess(node);       
        case AST_IF_STATEMENT:
            return codegenIfStatement(node);
        case AST_WHILE_LOOP:
            return codegenWhileLoop(node);
        case AST_FOR_LOOP:
            return codegenForLoop(node);
        case AST_FUNCTION_DEFINITION:
            return codegenFunctionDefinition(node);
        case AST_FUNCTION_CALL:
            return codegenFunctionCall(node);
        case AST_RETURN:
            return codegenReturn(node);
        case AST_BREAK:
            return codegenBreak(node);
        case AST_CONTINUE:
            return codegenContinue(node);
        case AST_SWITCH:
            return codegenSwitch(node);
	case AST_CASE:
    	    fprintf(stderr, "Error: 'case' encountered outside of 'switch' at codegen level\n");
    	    return NULL;
        case AST_NUMBER_LITERAL:
            return codegenNumberLiteral(node);
        case AST_CHAR_LITERAL:
            return codegenCharLiteral(node);
        case AST_STRING_LITERAL:
            return codegenStringLiteral(node);
        case AST_IDENTIFIER:
            return codegenIdentifier(node);
        case AST_SIZEOF:
            return codegenSizeof(node);
        default:
            fprintf(stderr, "Error: Unhandled AST node type %d\n", node->type);
            return NULL;
    }
}


LLVMValueRef codegenIdentifier(ASTNode *node) {
    if (node->type != AST_IDENTIFIER) {
        fprintf(stderr, "Error: Invalid node type for codegenIdentifier\n");
        return NULL;
    }

    LLVMValueRef var = LLVMGetNamedGlobal(TheModule, node->valueNode.value);
    if (!var) {
        fprintf(stderr, "Error: Undefined variable %s\n", node->valueNode.value);
        return NULL;
    }

    return LLVMBuildLoad2(Builder, LLVMInt32Type(), var, node->valueNode.value);
}


// Generates LLVM IR for a Number Literal node
LLVMValueRef codegenNumberLiteral(ASTNode *node) {
    if (node->type != AST_NUMBER_LITERAL) {
        fprintf(stderr, "Error: Invalid node type for codegenNumberLiteral\n");
        return NULL;
    }

    int value = atoi(node->valueNode.value);  // Convert string to integer
    return LLVMConstInt(LLVMInt32Type(), value, 0);
}

LLVMValueRef codegenCharLiteral(ASTNode *node) {
    if (node->type != AST_CHAR_LITERAL) {
        fprintf(stderr, "Error: Invalid node type for codegenCharLiteral\n");
        return NULL;
    }

    char value = node->valueNode.value[0];  // Take first char
    return LLVMConstInt(LLVMInt8Type(), value, 0);
}

LLVMValueRef codegenStringLiteral(ASTNode *node) {
    if (node->type != AST_STRING_LITERAL) {
        fprintf(stderr, "Error: Invalid node type for codegenStringLiteral\n");
        return NULL;
    }

    return LLVMBuildGlobalStringPtr(Builder, node->valueNode.value, "str");
}

LLVMValueRef codegenTernaryExpression(ASTNode *node) {
    if (node->type != AST_TERNARY_EXPRESSION) {
        fprintf(stderr, "Error: Invalid node type for codegenTernaryExpression\n");
        return NULL;
    }

    LLVMValueRef condition = codegen(node->ternaryExpr.condition);
    if (!condition) {
        fprintf(stderr, "Error: Failed to generate condition for ternary expression\n");
        return NULL;
    }

    // Convert condition to boolean (LLVM requires i1)
    condition = LLVMBuildICmp(Builder, LLVMIntNE, condition, LLVMConstInt(LLVMInt32Type(), 0, 0), 
"ternaryCond");

    // Get parent function
    LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(Builder));

    // Create basic blocks
    LLVMBasicBlockRef trueBB = LLVMAppendBasicBlock(func, "ternaryTrue");
    LLVMBasicBlockRef falseBB = LLVMAppendBasicBlock(func, "ternaryFalse");
    LLVMBasicBlockRef mergeBB = LLVMAppendBasicBlock(func, "ternaryMerge");

    // Conditional branch
    LLVMBuildCondBr(Builder, condition, trueBB, falseBB);

    // Generate true expression block
    LLVMPositionBuilderAtEnd(Builder, trueBB);
    LLVMValueRef trueValue = codegen(node->ternaryExpr.trueExpr);
    LLVMBuildBr(Builder, mergeBB);

    // Generate false expression block
    LLVMPositionBuilderAtEnd(Builder, falseBB);
    LLVMValueRef falseValue = codegen(node->ternaryExpr.falseExpr);
    LLVMBuildBr(Builder, mergeBB);

    // Merge block
    LLVMPositionBuilderAtEnd(Builder, mergeBB);
    LLVMValueRef phi = LLVMBuildPhi(Builder, LLVMInt32Type(), "ternaryResult");
    LLVMAddIncoming(phi, &trueValue, &trueBB, 1);
    LLVMAddIncoming(phi, &falseValue, &falseBB, 1);

    return phi;
}

LLVMValueRef codegenBinaryExpression(ASTNode *node) {
    if (node->type != AST_BINARY_EXPRESSION) {
        fprintf(stderr, "Error: Invalid node type for codegenBinaryExpression\n");
        return NULL;
    }

    LLVMValueRef L = codegen(node->expr.left);
    LLVMValueRef R = codegen(node->expr.right);
    if (!L || !R) {
        fprintf(stderr, "Error: Failed to generate operands for binary expression\n");
        return NULL;
    }

    // Identify the operator and generate corresponding LLVM IR
    if (strcmp(node->expr.op, "+") == 0) {
        return LLVMBuildAdd(Builder, L, R, "addtmp");
    } else if (strcmp(node->expr.op, "-") == 0) {
        return LLVMBuildSub(Builder, L, R, "subtmp");
    } else if (strcmp(node->expr.op, "*") == 0) {
        return LLVMBuildMul(Builder, L, R, "multmp");
    } else if (strcmp(node->expr.op, "/") == 0) {
        return LLVMBuildSDiv(Builder, L, R, "divtmp");
    } else if (strcmp(node->expr.op, "==") == 0) {
        return LLVMBuildICmp(Builder, LLVMIntEQ, L, R, "eqtmp");
    } else if (strcmp(node->expr.op, "!=") == 0) {
        return LLVMBuildICmp(Builder, LLVMIntNE, L, R, "netmp");
    } else if (strcmp(node->expr.op, "<") == 0) {
        return LLVMBuildICmp(Builder, LLVMIntSLT, L, R, "lttmp");
    } else if (strcmp(node->expr.op, "<=") == 0) {
        return LLVMBuildICmp(Builder, LLVMIntSLE, L, R, "letmp");
    } else if (strcmp(node->expr.op, ">") == 0) {
        return LLVMBuildICmp(Builder, LLVMIntSGT, L, R, "gttmp");
    } else if (strcmp(node->expr.op, ">=") == 0) {
        return LLVMBuildICmp(Builder, LLVMIntSGE, L, R, "getmp");
    }

    fprintf(stderr, "Error: Unsupported binary operator %s\n", node->expr.op);
    return NULL;
}

LLVMValueRef codegenUnaryExpression(ASTNode *node) {
    if (node->type != AST_UNARY_EXPRESSION) {
        fprintf(stderr, "Error: Invalid node type for codegenUnaryExpression\n");
        return NULL;
    }

    LLVMValueRef Operand = codegen(node->expr.left);
    if (!Operand) {
        fprintf(stderr, "Error: Failed to generate operand for unary expression\n");
        return NULL;
    }

    // Identify the operator and generate corresponding LLVM IR
    if (strcmp(node->expr.op, "-") == 0) {
        return LLVMBuildNeg(Builder, Operand, "negtmp");
    } else if (strcmp(node->expr.op, "!") == 0) {
        return LLVMBuildNot(Builder, Operand, "nottmp");
    } else if (strcmp(node->expr.op, "&") == 0) {
        fprintf(stderr, "Error: Address-of (&) operator requires variables, not implemented yet.\n");
        return NULL;
    } else if (strcmp(node->expr.op, "*") == 0) {
        return LLVMBuildLoad2(Builder, LLVMInt32Type(), Operand, "loadtmp");
    }

    fprintf(stderr, "Error: Unsupported unary operator %s\n", node->expr.op);
    return NULL;
}

LLVMValueRef codegenVariableDeclaration(ASTNode *node) {
    if (node->type != AST_VARIABLE_DECLARATION) {
        fprintf(stderr, "Error: Invalid node type for codegenVariableDeclaration\n");
        return NULL;
    }

    LLVMTypeRef varType = LLVMInt32Type();  // Assume int32 for now
    for (size_t i = 0; i < node->varDecl.varCount; i++) {
        ASTNode *varNameNode = node->varDecl.varNames[i];
        struct DesignatedInit* initNode = node->varDecl.initializers ? node->varDecl.initializers[i] : NULL;

        LLVMValueRef variable = LLVMBuildAlloca(Builder, varType, varNameNode->valueNode.value);

	DesignatedInit* init = node->varDecl.initializers[i];
        if (init && init->expression) {
            LLVMValueRef initValue = codegen(init->expression);
            LLVMBuildStore(Builder, initValue, variable);
        }
    }

    return NULL;  // Declarations don’t return a value
}

LLVMValueRef codegenAssignment(ASTNode *node) {
    if (node->type != AST_ASSIGNMENT) {
        fprintf(stderr, "Error: Invalid node type for codegenAssignment\n");
        return NULL;
    }

    LLVMValueRef targetVar = LLVMBuildAlloca(Builder, LLVMInt32Type(), node->assignment.target->valueNode.value);
    LLVMValueRef value = codegen(node->assignment.value);
    
    if (!targetVar || !value) {
        fprintf(stderr, "Error: Assignment failed\n");
        return NULL;
    }

    return LLVMBuildStore(Builder, value, targetVar);
}

LLVMValueRef codegenFunctionDefinition(ASTNode *node) {
    if (node->type != AST_FUNCTION_DEFINITION) {
        fprintf(stderr, "Error: Invalid node type for codegenFunctionDefinition\n");
        return NULL;
    }

    LLVMTypeRef returnType = LLVMInt32Type();  // Assume all functions return int32 for now

    // Create function parameter types
    LLVMTypeRef *paramTypes = malloc(sizeof(LLVMTypeRef) * node->functionDef.paramCount);
    for (size_t i = 0; i < node->functionDef.paramCount; i++) {
        paramTypes[i] = LLVMInt32Type();  // Assume int32 parameters for now
    }

    LLVMTypeRef funcType = LLVMFunctionType(returnType, paramTypes, node->functionDef.paramCount, 0);
    LLVMValueRef function = LLVMAddFunction(TheModule, node->functionDef.funcName->valueNode.value, 
						funcType);

    // Create entry basic block
    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(function, "entry");
    LLVMPositionBuilderAtEnd(Builder, entry);

    // Generate function body
    codegen(node->functionDef.body);

    return function;
}

LLVMValueRef codegenFunctionCall(ASTNode *node) {
    if (node->type != AST_FUNCTION_CALL) {
        fprintf(stderr, "Error: Invalid node type for codegenFunctionCall\n");
        return NULL;
    }

    LLVMValueRef function = LLVMGetNamedFunction(TheModule, node->functionCall.callee->valueNode.value);
    if (!function) {
        fprintf(stderr, "Error: Undefined function %s\n", node->functionCall.callee->valueNode.value);
        return NULL;
    }

    LLVMValueRef *args = malloc(sizeof(LLVMValueRef) * node->functionCall.argumentCount);
    for (size_t i = 0; i < node->functionCall.argumentCount; i++) {
        args[i] = codegen(node->functionCall.arguments[i]);
    }

    return LLVMBuildCall2(Builder, LLVMGetElementType(LLVMTypeOf(function)), function, args, 
node->functionCall.argumentCount, "calltmp");
}

LLVMValueRef codegenIfStatement(ASTNode *node) {
    if (node->type != AST_IF_STATEMENT) {
        fprintf(stderr, "Error: Invalid node type for codegenIfStatement\n");
        return NULL;
    }

    LLVMValueRef cond = codegen(node->ifStmt.condition);
    if (!cond) {
        fprintf(stderr, "Error: Failed to generate condition for if statement\n");
        return NULL;
    }

    // Convert condition to boolean (LLVM expects i1)
    cond = LLVMBuildICmp(Builder, LLVMIntNE, cond, LLVMConstInt(LLVMInt32Type(), 0, 0), "ifcond");

    // Get parent function
    LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(Builder));

    // Create basic blocks
    LLVMBasicBlockRef thenBB = LLVMAppendBasicBlock(func, "then");
    LLVMBasicBlockRef elseBB = node->ifStmt.elseBranch ? LLVMAppendBasicBlock(func, "else") : NULL;
    LLVMBasicBlockRef mergeBB = LLVMAppendBasicBlock(func, "ifcont");

    // Conditional branch
    LLVMBuildCondBr(Builder, cond, thenBB, elseBB ? elseBB : mergeBB);

    // Generate then block
    LLVMPositionBuilderAtEnd(Builder, thenBB);
    codegen(node->ifStmt.thenBranch);
    LLVMBuildBr(Builder, mergeBB);

    // Generate else block (if exists)
    if (elseBB) {
        LLVMPositionBuilderAtEnd(Builder, elseBB);
        codegen(node->ifStmt.elseBranch);
        LLVMBuildBr(Builder, mergeBB);
    }

    // Merge point
    LLVMPositionBuilderAtEnd(Builder, mergeBB);

    return NULL;
}

LLVMValueRef codegenWhileLoop(ASTNode *node) {
    if (node->type != AST_WHILE_LOOP) {
        fprintf(stderr, "Error: Invalid node type for codegenWhileLoop\n");
        return NULL;
    }

    LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(Builder));

    LLVMBasicBlockRef condBB = LLVMAppendBasicBlock(func, "loopcond");
    LLVMBasicBlockRef bodyBB = LLVMAppendBasicBlock(func, "loopbody");
    LLVMBasicBlockRef afterBB = LLVMAppendBasicBlock(func, "loopend");

    if (!node->whileLoop.isDoWhile) {
        LLVMBuildBr(Builder, condBB);
    }

    // Condition Block
    LLVMPositionBuilderAtEnd(Builder, condBB);
    LLVMValueRef cond = codegen(node->whileLoop.condition);
    cond = LLVMBuildICmp(Builder, LLVMIntNE, cond, LLVMConstInt(LLVMInt32Type(), 0, 0), "loopcond");
    LLVMBuildCondBr(Builder, cond, bodyBB, afterBB);

    // Body Block
    LLVMPositionBuilderAtEnd(Builder, bodyBB);
    codegen(node->whileLoop.body);
    LLVMBuildBr(Builder, condBB);

    // After Block
    LLVMPositionBuilderAtEnd(Builder, afterBB);

    return NULL;
}

LLVMValueRef codegenForLoop(ASTNode *node) {
    if (node->type != AST_FOR_LOOP) {
        fprintf(stderr, "Error: Invalid node type for codegenForLoop\n");
        return NULL;
    }

    LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(Builder));

    LLVMBasicBlockRef condBB = LLVMAppendBasicBlock(func, "forcond");
    LLVMBasicBlockRef bodyBB = LLVMAppendBasicBlock(func, "forbody");
    LLVMBasicBlockRef incBB = LLVMAppendBasicBlock(func, "forinc");  
    LLVMBasicBlockRef afterBB = LLVMAppendBasicBlock(func, "forend");

    // Set loop control targets
    loopBreakTarget = afterBB;
    loopContinueTarget = incBB;  // Continue should jump to increment step

    // Generate initializer
    if (node->forLoop.initializer) {
        codegen(node->forLoop.initializer);
    }

    // Jump to condition check
    LLVMBuildBr(Builder, condBB);

    // Condition Block
    LLVMPositionBuilderAtEnd(Builder, condBB);
    LLVMValueRef cond = codegen(node->forLoop.condition);
    if (!cond) {
        fprintf(stderr, "Error: Null condition in for loop\n");
        return NULL;
    }
    cond = LLVMBuildICmp(Builder, LLVMIntNE, cond, LLVMConstInt(LLVMInt32Type(), 0, 0), "forcond");
    LLVMBuildCondBr(Builder, cond, bodyBB, afterBB);  // Fix: Added condition argument

    // Body Block
    LLVMPositionBuilderAtEnd(Builder, bodyBB);
    codegen(node->forLoop.body);
    LLVMBuildBr(Builder, incBB);  // Jump to increment step

    // Increment Block
    LLVMPositionBuilderAtEnd(Builder, incBB);
    if (node->forLoop.increment) {
        codegen(node->forLoop.increment);
    }
    LLVMBuildBr(Builder, condBB);  // Jump back to condition

    // After Block (Loop exit)
    LLVMPositionBuilderAtEnd(Builder, afterBB);
    loopBreakTarget = NULL;
    loopContinueTarget = NULL;

    return NULL;
}


// Control Statements
LLVMValueRef codegenReturn(ASTNode *node) {
    if (node->type != AST_RETURN) {
        fprintf(stderr, "Error: Invalid node type for codegenReturn\n");
        return NULL;
    }

    if (node->returnStmt.returnValue) {
        LLVMValueRef returnValue = codegen(node->returnStmt.returnValue);
        return LLVMBuildRet(Builder, returnValue);
    } else {
        return LLVMBuildRetVoid(Builder);
    }
}

LLVMValueRef codegenBreak(ASTNode *node) {
    if (node->type != AST_BREAK) {
        fprintf(stderr, "Error: Invalid node type for codegenBreak\n");
        return NULL;
    }

    if (!loopBreakTarget) {
        fprintf(stderr, "Error: 'break' used outside of loop\n");
        return NULL;
    }

    return LLVMBuildBr(Builder, loopBreakTarget);
}

LLVMValueRef codegenContinue(ASTNode *node) {
    if (node->type != AST_CONTINUE) {
        fprintf(stderr, "Error: Invalid node type for codegenContinue\n");
        return NULL;
    }

    if (!loopContinueTarget) {
        fprintf(stderr, "Error: 'continue' used outside of loop\n");
        return NULL;
    }

    return LLVMBuildBr(Builder, loopContinueTarget);
}

// Swith and Case
LLVMValueRef codegenSwitch(ASTNode *node) {
    if (node->type != AST_SWITCH) {
        fprintf(stderr, "Error: Invalid node type for codegenSwitch\n");
        return NULL;
    }

    LLVMValueRef condition = codegen(node->switchStmt.condition);
    if (!condition) {
        fprintf(stderr, "Error: Failed to generate switch condition\n");
        return NULL;
    }

    LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(Builder));

    LLVMBasicBlockRef switchEnd = LLVMAppendBasicBlock(func, "switchEnd");

    LLVMValueRef switchInst = LLVMBuildSwitch(Builder, condition, switchEnd, 
					node->switchStmt.caseListSize);

    for (size_t i = 0; i < node->switchStmt.caseListSize; i++) {
        codegenCase(node->switchStmt.caseList[i], switchInst, switchEnd);
    }

    LLVMPositionBuilderAtEnd(Builder, switchEnd);
    
    return NULL;
}

void codegenCase(ASTNode *node, LLVMValueRef switchInst, LLVMBasicBlockRef switchEnd) {
    if (node->type != AST_CASE) {
        fprintf(stderr, "Error: Invalid node type for codegenCase\n");
        return;
    }

    LLVMValueRef caseValue = codegen(node->caseStmt.caseValue);
    if (!caseValue) {
        fprintf(stderr, "Error: Failed to generate case value\n");
        return;
    }

    LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(Builder));
    LLVMBasicBlockRef caseBB = LLVMAppendBasicBlock(func, "case");

    LLVMAddCase(switchInst, caseValue, caseBB);

    LLVMPositionBuilderAtEnd(Builder, caseBB);
    for (size_t i = 0; i < node->caseStmt.caseBodySize; i++) {
        codegen(node->caseStmt.caseBody[i]);
    }

    LLVMBuildBr(Builder, switchEnd);
}


LLVMValueRef codegenArrayDeclaration(ASTNode *node) {
    if (node->type != AST_ARRAY_DECLARATION) {
        fprintf(stderr, "Error: Invalid node type for codegenArrayDeclaration\n");
        return NULL;
    }

    LLVMTypeRef elementType = LLVMInt32Type();  // Assume int for now
    LLVMValueRef arraySize = codegen(node->arrayDecl.arraySize);
    
    if (!arraySize) {
        fprintf(stderr, "Error: Array size not specified\n");
        return NULL;
    }

    LLVMTypeRef arrayType = LLVMArrayType(elementType, node->arrayDecl.valueCount);
    LLVMValueRef array = LLVMBuildAlloca(Builder, arrayType, node->arrayDecl.varName->valueNode.value);

    // Initialize array if values are provided
    for (size_t i = 0; i < node->arrayDecl.valueCount; i++) {
        LLVMValueRef index = LLVMConstInt(LLVMInt32Type(), i, 0);

	DesignatedInit* init = node->arrayDecl.initializers[i];
	if (init && init->expression) {
	    LLVMValueRef value = codegen(init->expression);

	    LLVMValueRef elementPtr = LLVMBuildGEP2(Builder, arrayType, array, &index, 1, "elementPtr");
            LLVMBuildStore(Builder, value, elementPtr);
	}
    }

    return array;
}

LLVMValueRef codegenArrayAccess(ASTNode *node) {
    if (node->type != AST_ARRAY_ACCESS) {
        fprintf(stderr, "Error: Invalid node type for codegenArrayAccess\n");
        return NULL;
    }

    LLVMValueRef arrayPtr = codegen(node->arrayAccess.array);
    LLVMValueRef index = codegen(node->arrayAccess.index);
    
    if (!arrayPtr || !index) {
        fprintf(stderr, "Error: Array access failed\n");
        return NULL;
    }

    LLVMTypeRef elementType = LLVMInt32Type();
    LLVMValueRef elementPtr = LLVMBuildGEP2(Builder, elementType, arrayPtr, &index, 1, "elementPtr");
    
    return LLVMBuildLoad2(Builder, elementType, elementPtr, "arrayLoad");
}

LLVMValueRef codegenDotAccess(ASTNode *node) {
    if (node->type != AST_DOT_ACCESS) {
        fprintf(stderr, "Error: Invalid node type for codegenDotAccess\n");
        return NULL;
    }

    // Generate the struct value (e.g., the 'obj' in 'obj.field')
    LLVMValueRef structVal = codegen(node->memberAccess.base);
    if (!structVal) return NULL;

    // Get field index from struct definition (again, you must implement this)
    int fieldIndex = getFieldIndexFromStruct(structVal, node->memberAccess.field);
    if (fieldIndex < 0) {
        fprintf(stderr, "Error: Unknown field '%s'\n", node->memberAccess.field);
        return NULL;
    }

    LLVMValueRef indices[] = {
        LLVMConstInt(LLVMInt32Type(), 0, 0),
        LLVMConstInt(LLVMInt32Type(), fieldIndex, 0)
    };

    LLVMValueRef gep = LLVMBuildGEP2(Builder, LLVMTypeOf(structVal), structVal, indices, 2, 
"dot_field_gep");
    return LLVMBuildLoad2(Builder, LLVMInt32Type(), gep, "dot_field");
}

LLVMValueRef codegenPointerAccess(ASTNode *node) {
    if (node->type != AST_POINTER_ACCESS) {
        fprintf(stderr, "Error: Invalid node type for codegenPointerAccess\n");
        return NULL;
    }

    // Generate base pointer (e.g., the 'ptr' in 'ptr->field')
    LLVMValueRef basePtr = codegen(node->memberAccess.base);
    if (!basePtr) return NULL;

    // Load the struct pointer
    LLVMValueRef loadedPtr = LLVMBuildLoad2(Builder, LLVMTypeOf(basePtr), basePtr, "deref_ptr");

    // Get field index from struct definition (you'll need a real mapping in your symbol table)
    int fieldIndex = getFieldIndexFromStruct(basePtr, node->memberAccess.field); // <- MUST IMPLEMENT THIS

    if (fieldIndex < 0) {
        fprintf(stderr, "Error: Unknown field '%s'\n", node->memberAccess.field);
        return NULL;
    }

    LLVMValueRef indices[] = {
        LLVMConstInt(LLVMInt32Type(), 0, 0),
        LLVMConstInt(LLVMInt32Type(), fieldIndex, 0)
    };

    LLVMValueRef gep = LLVMBuildGEP2(Builder, LLVMTypeOf(loadedPtr), loadedPtr, indices, 2, 
"ptr_field_gep");
    return LLVMBuildLoad2(Builder, LLVMInt32Type(), gep, "ptr_field");
}


LLVMValueRef codegenPointerDereference(ASTNode *node) {
    if (node->type != AST_POINTER_DEREFERENCE) {
        fprintf(stderr, "Error: Invalid node type for codegenPointerDereference\n");
        return NULL;
    }

    LLVMValueRef pointer = codegen(node->pointerDeref.pointer);
    if (!pointer) {
        fprintf(stderr, "Error: Failed to generate pointer dereference\n");
        return NULL;
    }

    return LLVMBuildLoad2(Builder, LLVMInt32Type(), pointer, "ptrLoad");
}

LLVMTypeRef codegenStructDefinition(ASTNode *node) {
    if (node->type != AST_STRUCT_DEFINITION) {
        fprintf(stderr, "Error: Invalid node type for codegenStructDefinition\n");
        return NULL;
    }

    LLVMTypeRef *fieldTypes = malloc(sizeof(LLVMTypeRef) * node->structDef.fieldCount);
    for (size_t i = 0; i < node->structDef.fieldCount; i++) {
        fieldTypes[i] = LLVMInt32Type();  // Assume int for now
    }

    LLVMTypeRef structType = LLVMStructType(fieldTypes, node->structDef.fieldCount, 0);
    LLVMStructSetBody(structType, fieldTypes, node->structDef.fieldCount, 0);

    return structType;
}

LLVMValueRef codegenStructFieldAccess(ASTNode *node) {
    if (node->type != AST_STRUCT_FIELD_ACCESS) {
        fprintf(stderr, "Error: Invalid node type for codegenStructFieldAccess\n");
        return NULL;
    }

    LLVMValueRef structPtr = codegen(node->structFieldAccess.structInstance);
    if (!structPtr) {
        fprintf(stderr, "Error: Failed to generate struct instance\n");
        return NULL;
    }

    int fieldIndex = -1;  // Assume we have a way to determine field index
    if (strcmp(node->structFieldAccess.fieldName, "x") == 0) fieldIndex = 0;
    if (strcmp(node->structFieldAccess.fieldName, "y") == 0) fieldIndex = 1;

    if (fieldIndex == -1) {
        fprintf(stderr, "Error: Unknown struct field %s\n", node->structFieldAccess.fieldName);
        return NULL;
    }

    LLVMValueRef indices[] = {LLVMConstInt(LLVMInt32Type(), 0, 0), LLVMConstInt(LLVMInt32Type(), fieldIndex, 0)};
    LLVMValueRef fieldPtr = LLVMBuildGEP2(Builder, LLVMStructType(NULL, 0, 0), structPtr, indices, 2, "fieldPtr");

    return LLVMBuildLoad2(Builder, LLVMInt32Type(), fieldPtr, "fieldLoad");
}

LLVMValueRef codegenHeapAllocation(ASTNode *node) {
    if (node->type != AST_HEAP_ALLOCATION) {
        fprintf(stderr, "Error: Invalid node type for codegenHeapAllocation\n");
        return NULL;
    }

    LLVMTypeRef allocType = LLVMStructType(NULL, 0, 0);  // Assume struct type for now
    LLVMValueRef size = LLVMSizeOf(allocType);
    LLVMValueRef mallocFunc = LLVMGetNamedFunction(TheModule, "malloc");

    if (!mallocFunc) {
        fprintf(stderr, "Error: malloc function not found\n");
        return NULL;
    }

    LLVMValueRef args[] = {size};
    return LLVMBuildCall2(Builder, LLVMTypeOf(mallocFunc), mallocFunc, args, 1, "mallocCall");
}

LLVMValueRef codegenSizeof(ASTNode *node) {
    if (node->type != AST_SIZEOF) {
        fprintf(stderr, "Error: Invalid node type for codegenSizeof\n");
        return NULL;
    }

    LLVMTypeRef type = LLVMInt32Type();  // Default to int32
    if (node->expr.left) {
        LLVMValueRef value = codegen(node->expr.left);
        type = LLVMTypeOf(value);
    }

    return LLVMSizeOf(type);
}

LLVMValueRef codegenBlock(ASTNode *node) {
    if (node->type != AST_BLOCK) {
        fprintf(stderr, "Error: Invalid node type for codegenBlock\n");
        return NULL;
    }

    LLVMValueRef lastValue = NULL;
    for (size_t i = 0; i < node->block.statementCount; i++) {
        lastValue = codegen(node->block.statements[i]);
    }

    return lastValue;  // Returns the last computed value in the block
}

LLVMValueRef codegenProgram(ASTNode *node) {
    if (node->type != AST_PROGRAM) {
        fprintf(stderr, "Error: Invalid node type for codegenProgram\n");
        return NULL;
    }

    for (size_t i = 0; i < node->block.statementCount; i++) {
        codegen(node->block.statements[i]);
    }

    return NULL;
}


// Helper methods
int getFieldIndexFromStruct(LLVMValueRef structVal, const char* fieldName) {
    // This function should:
    // 1. Identify the struct type from `structVal`
    // 2. Look up the fieldName in the struct's field order
    // 3. Return the correct index

    // TEMP MOCKUP — replace with actual logic based on your symbol table
    if (strcmp(fieldName, "x") == 0) return 0;
    if (strcmp(fieldName, "y") == 0) return 1;
    return -1;
}

