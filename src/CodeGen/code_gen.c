#include "code_gen.h"
#include "codegen_internal.h"
#include "codegen_types.h"
#include "codegen_type_cache.h"
#include "Parser/Helpers/designated_init.h"
#include "Parser/Helpers/parsed_type.h"
#include "Syntax/semantic_model.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct NamedValue {
    char* name;
    LLVMValueRef value;
    LLVMTypeRef type;
    bool isGlobal;
} NamedValue;

typedef struct CGScope {
    NamedValue* entries;
    size_t count;
    size_t capacity;
    struct CGScope* parent;
} CGScope;

typedef struct LoopTarget {
    LLVMBasicBlockRef breakBB;
    LLVMBasicBlockRef continueBB;
} LoopTarget;

typedef struct NamedType {
    char* name;
    LLVMTypeRef type;
} NamedType;

typedef struct StructFieldInfo {
    char* name;
    unsigned index;
    LLVMTypeRef type;
} StructFieldInfo;

typedef struct StructInfo {
    char* name;
    StructFieldInfo* fields;
    size_t fieldCount;
    bool isUnion;
    LLVMTypeRef llvmType;
} StructInfo;

struct CodegenContext {
    LLVMContextRef llvmContext;
    LLVMModuleRef module;
    LLVMBuilderRef builder;

    CGScope* currentScope;

    LoopTarget* loopStack;
    size_t loopStackSize;
    size_t loopStackCapacity;

    NamedType* namedTypes;
    size_t namedTypeCount;
    size_t namedTypeCapacity;

    StructInfo* structInfos;
    size_t structInfoCount;
    size_t structInfoCapacity;

    const SemanticModel* semanticModel;
    CGTypeCache* typeCache;
};

LLVMContextRef cg_context_get_llvm_context(CodegenContext* ctx);
LLVMModuleRef cg_context_get_module(CodegenContext* ctx);
LLVMBuilderRef cg_context_get_builder(CodegenContext* ctx);
LLVMTypeRef cg_context_lookup_named_type(CodegenContext* ctx, const char* name);
void cg_context_cache_named_type(CodegenContext* ctx, const char* name, LLVMTypeRef type);

static CGScope* cg_scope_create(CGScope* parent);
static void cg_scope_destroy(CGScope* scope);
static CGScope* cg_scope_push(CodegenContext* ctx);
static void cg_scope_pop(CodegenContext* ctx);
static void cg_scope_insert(CGScope* scope, const char* name, LLVMValueRef value, LLVMTypeRef type, bool isGlobal);
static NamedValue* cg_scope_lookup(CGScope* scope, const char* name);

static void cg_loop_push(CodegenContext* ctx, LLVMBasicBlockRef breakBB, LLVMBasicBlockRef continueBB);
static void cg_loop_pop(CodegenContext* ctx);
static LoopTarget cg_loop_peek(const CodegenContext* ctx);

static LLVMTypeRef* collectParamTypes(CodegenContext* ctx, size_t paramCount, ASTNode** params);
static LLVMValueRef ensureFunction(CodegenContext* ctx, const char* name, const ParsedType* returnType, size_t paramCount, LLVMTypeRef* paramTypes);
static void declareFunctionPrototype(CodegenContext* ctx, ASTNode* node);
static void declareGlobalVariable(CodegenContext* ctx, ASTNode* node);
static void predeclareGlobals(CodegenContext* ctx, ASTNode* program);
static void declareGlobalVariableSymbol(CodegenContext* ctx, const Symbol* sym);
static void declareFunctionSymbol(CodegenContext* ctx, const Symbol* sym);
static void predeclareGlobalSymbolCallback(const Symbol* sym, void* userData);
static void recordStructInfo(CodegenContext* ctx, const char* name, bool isUnion, StructFieldInfo* fields, size_t fieldCount, LLVMTypeRef llvmType);
static const StructInfo* lookupStructInfo(CodegenContext* ctx, const char* name, LLVMTypeRef llvmType);
static bool codegenLValue(CodegenContext* ctx, ASTNode* target, LLVMValueRef* outPtr, LLVMTypeRef* outType);
static LLVMValueRef buildStructFieldPointer(CodegenContext* ctx,
                                            LLVMValueRef basePtr,
                                            LLVMTypeRef aggregateTypeHint,
                                            const char* structName,
                                            const char* fieldName,
                                            LLVMTypeRef* outFieldType);
static LLVMValueRef buildArrayElementPointer(CodegenContext* ctx, LLVMValueRef arrayPtr, LLVMValueRef index, LLVMTypeRef elementType);
static LLVMTypeRef ensureStructLLVMTypeByName(CodegenContext* ctx, const char* name, bool isUnionHint);
static void declareStructSymbol(CodegenContext* ctx, const Symbol* sym);

static LLVMValueRef codegenNode(CodegenContext* ctx, ASTNode* node);
static LLVMValueRef codegenProgram(CodegenContext* ctx, ASTNode* node);
static LLVMValueRef codegenBlock(CodegenContext* ctx, ASTNode* node);
static LLVMValueRef codegenBinaryExpression(CodegenContext* ctx, ASTNode* node);
static LLVMValueRef codegenUnaryExpression(CodegenContext* ctx, ASTNode* node);
static LLVMValueRef codegenTernaryExpression(CodegenContext* ctx, ASTNode* node);
static LLVMValueRef codegenAssignment(CodegenContext* ctx, ASTNode* node);
static LLVMValueRef codegenVariableDeclaration(CodegenContext* ctx, ASTNode* node);
static LLVMValueRef codegenArrayDeclaration(CodegenContext* ctx, ASTNode* node);
static LLVMValueRef codegenArrayAccess(CodegenContext* ctx, ASTNode* node);
static LLVMValueRef codegenPointerAccess(CodegenContext* ctx, ASTNode* node);
static LLVMValueRef codegenDotAccess(CodegenContext* ctx, ASTNode* node);
static LLVMValueRef codegenPointerDereference(CodegenContext* ctx, ASTNode* node);
static LLVMValueRef codegenIfStatement(CodegenContext* ctx, ASTNode* node);
static LLVMValueRef codegenWhileLoop(CodegenContext* ctx, ASTNode* node);
static LLVMValueRef codegenForLoop(CodegenContext* ctx, ASTNode* node);
static LLVMValueRef codegenFunctionDefinition(CodegenContext* ctx, ASTNode* node);
static LLVMValueRef codegenFunctionCall(CodegenContext* ctx, ASTNode* node);
static LLVMValueRef codegenTypedef(CodegenContext* ctx, ASTNode* node);
static LLVMValueRef codegenEnumDefinition(CodegenContext* ctx, ASTNode* node);
static LLVMValueRef codegenCastExpression(CodegenContext* ctx, ASTNode* node);
static LLVMValueRef codegenCompoundLiteral(CodegenContext* ctx, ASTNode* node);
static LLVMValueRef codegenReturn(CodegenContext* ctx, ASTNode* node);
static LLVMValueRef codegenBreak(CodegenContext* ctx, ASTNode* node);
static LLVMValueRef codegenContinue(CodegenContext* ctx, ASTNode* node);
static LLVMValueRef codegenNumberLiteral(CodegenContext* ctx, ASTNode* node);
static LLVMValueRef codegenCharLiteral(CodegenContext* ctx, ASTNode* node);
static LLVMValueRef codegenStringLiteral(CodegenContext* ctx, ASTNode* node);
static LLVMValueRef codegenIdentifier(CodegenContext* ctx, ASTNode* node);
static LLVMValueRef codegenSizeof(CodegenContext* ctx, ASTNode* node);
static LLVMValueRef codegenSwitch(CodegenContext* ctx, ASTNode* node);
static void codegenCase(CodegenContext* ctx, ASTNode* node, LLVMValueRef switchInst, LLVMBasicBlockRef switchEnd);
static LLVMTypeRef codegenStructDefinition(CodegenContext* ctx, ASTNode* node);
static LLVMValueRef codegenStructFieldAccess(CodegenContext* ctx, ASTNode* node);
static LLVMValueRef codegenHeapAllocation(CodegenContext* ctx, ASTNode* node);
CodegenContext* codegen_context_create(const char* moduleName, const SemanticModel* semanticModel) {
    CodegenContext* ctx = (CodegenContext*)calloc(1, sizeof(CodegenContext));
    if (!ctx) {
        return NULL;
    }

    ctx->llvmContext = LLVMContextCreate();
    if (!ctx->llvmContext) {
        free(ctx);
        return NULL;
    }

    const char* name = moduleName ? moduleName : "compiler_module";
    ctx->module = LLVMModuleCreateWithNameInContext(name, ctx->llvmContext);
    if (!ctx->module) {
        LLVMContextDispose(ctx->llvmContext);
        free(ctx);
        return NULL;
    }

    ctx->builder = LLVMCreateBuilderInContext(ctx->llvmContext);
    if (!ctx->builder) {
        LLVMDisposeModule(ctx->module);
        LLVMContextDispose(ctx->llvmContext);
        free(ctx);
        return NULL;
    }

    ctx->semanticModel = semanticModel;
    if (semanticModel) {
        ctx->typeCache = cg_type_cache_create(semanticModel);
        if (!ctx->typeCache) {
            LLVMDisposeBuilder(ctx->builder);
            LLVMDisposeModule(ctx->module);
            LLVMContextDispose(ctx->llvmContext);
            free(ctx);
            return NULL;
        }
    }

    return ctx;
}

void codegen_context_destroy(CodegenContext* ctx) {
    if (!ctx) return;

    while (ctx->currentScope) {
        cg_scope_pop(ctx);
    }

    free(ctx->loopStack);
    ctx->loopStack = NULL;
    ctx->loopStackSize = 0;
    ctx->loopStackCapacity = 0;

    if (ctx->namedTypes) {
        for (size_t i = 0; i < ctx->namedTypeCount; ++i) {
            free(ctx->namedTypes[i].name);
        }
        free(ctx->namedTypes);
        ctx->namedTypes = NULL;
        ctx->namedTypeCount = 0;
        ctx->namedTypeCapacity = 0;
    }

    if (ctx->structInfos) {
        for (size_t i = 0; i < ctx->structInfoCount; ++i) {
            free(ctx->structInfos[i].name);
            for (size_t j = 0; j < ctx->structInfos[i].fieldCount; ++j) {
                free(ctx->structInfos[i].fields[j].name);
            }
            free(ctx->structInfos[i].fields);
        }
        free(ctx->structInfos);
        ctx->structInfos = NULL;
        ctx->structInfoCount = 0;
        ctx->structInfoCapacity = 0;
    }

    ctx->semanticModel = NULL;
    if (ctx->typeCache) {
        cg_type_cache_destroy(ctx->typeCache);
        ctx->typeCache = NULL;
    }

    if (ctx->builder) {
        LLVMDisposeBuilder(ctx->builder);
        ctx->builder = NULL;
    }

    if (ctx->module) {
        LLVMDisposeModule(ctx->module);
        ctx->module = NULL;
    }

    if (ctx->llvmContext) {
        LLVMContextDispose(ctx->llvmContext);
        ctx->llvmContext = NULL;
    }

    free(ctx);
}

LLVMModuleRef codegen_get_module(const CodegenContext* ctx) {
    return ctx ? ctx->module : NULL;
}

LLVMValueRef codegen_generate(CodegenContext* ctx, ASTNode* node) {
    if (!ctx || !node) return NULL;

    cg_scope_push(ctx);
    if (node->type == AST_PROGRAM) {
        predeclareGlobals(ctx, node);
    }
    LLVMValueRef result = codegenNode(ctx, node);
    cg_scope_pop(ctx);
    return result;
}

static LLVMValueRef codegenNode(CodegenContext* ctx, ASTNode* node) {
    if (!node) {
        fprintf(stderr, "Error: NULL node in codegen\n");
        return NULL;
    }

    fprintf(stderr, "[CG] Enter node type %d\n", node->type);

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
        case AST_ARRAY_DECLARATION:
            return codegenArrayDeclaration(ctx, node);
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

static LLVMValueRef codegenIdentifier(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_IDENTIFIER) {
        fprintf(stderr, "Error: Invalid node type for codegenIdentifier\n");
        return NULL;
    }

    NamedValue* entry = cg_scope_lookup(ctx->currentScope, node->valueNode.value);
    if (entry) {
        LLVMTypeRef loadType = entry->type;
        if (!loadType || LLVMGetTypeKind(loadType) == LLVMVoidTypeKind) {
            loadType = LLVMGetElementType(LLVMTypeOf(entry->value));
        }
        if (!loadType || LLVMGetTypeKind(loadType) == LLVMVoidTypeKind) {
            loadType = LLVMInt32TypeInContext(ctx->llvmContext);
        }
        return LLVMBuildLoad2(ctx->builder, loadType, entry->value, node->valueNode.value);
    }

    LLVMValueRef global = LLVMGetNamedGlobal(ctx->module, node->valueNode.value);
    if (!global) {
        fprintf(stderr, "Error: Undefined variable %s\n", node->valueNode.value);
        return NULL;
    }

    LLVMTypeRef loadType = LLVMGetElementType(LLVMTypeOf(global));
    if (!loadType || LLVMGetTypeKind(loadType) == LLVMVoidTypeKind) {
        loadType = LLVMInt32TypeInContext(ctx->llvmContext);
    }
    cg_scope_insert(ctx->currentScope,
                    node->valueNode.value,
                    global,
                    loadType,
                    true);
    return LLVMBuildLoad2(ctx->builder, loadType, global, node->valueNode.value);
}

static LLVMValueRef codegenNumberLiteral(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_NUMBER_LITERAL) {
        fprintf(stderr, "Error: Invalid node type for codegenNumberLiteral\n");
        return NULL;
    }

    int value = atoi(node->valueNode.value);
    return LLVMConstInt(LLVMInt32TypeInContext(ctx->llvmContext), value, 0);
}

static LLVMValueRef codegenCharLiteral(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_CHAR_LITERAL) {
        fprintf(stderr, "Error: Invalid node type for codegenCharLiteral\n");
        return NULL;
    }

    char value = node->valueNode.value[0];
    return LLVMConstInt(LLVMInt8TypeInContext(ctx->llvmContext), value, 0);
}

static LLVMValueRef codegenStringLiteral(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_STRING_LITERAL) {
        fprintf(stderr, "Error: Invalid node type for codegenStringLiteral\n");
        return NULL;
    }

    return LLVMBuildGlobalStringPtr(ctx->builder, node->valueNode.value, "str");
}

static LLVMValueRef codegenCompoundLiteral(CodegenContext* ctx, ASTNode* node) {
    (void)ctx;
    if (!node || node->type != AST_COMPOUND_LITERAL) {
        fprintf(stderr, "Error: Invalid node type for codegenCompoundLiteral\n");
        return NULL;
    }

    /* TODO: emit aggregate constants for compound literals. */
    return NULL;
}

static LLVMValueRef codegenTernaryExpression(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_TERNARY_EXPRESSION) {
        fprintf(stderr, "Error: Invalid node type for codegenTernaryExpression\n");
        return NULL;
    }

    LLVMValueRef condition = codegenNode(ctx, node->ternaryExpr.condition);
    if (!condition) {
        fprintf(stderr, "Error: Failed to generate condition for ternary expression\n");
        return NULL;
    }

    condition = LLVMBuildICmp(ctx->builder, LLVMIntNE, condition,
                              LLVMConstInt(LLVMInt32TypeInContext(ctx->llvmContext), 0, 0),
                              "ternaryCond");

    LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(ctx->builder));

    LLVMBasicBlockRef trueBB = LLVMAppendBasicBlock(func, "ternaryTrue");
    LLVMBasicBlockRef falseBB = LLVMAppendBasicBlock(func, "ternaryFalse");
    LLVMBasicBlockRef mergeBB = LLVMAppendBasicBlock(func, "ternaryMerge");

    LLVMBuildCondBr(ctx->builder, condition, trueBB, falseBB);

    LLVMPositionBuilderAtEnd(ctx->builder, trueBB);
    LLVMValueRef trueValue = codegenNode(ctx, node->ternaryExpr.trueExpr);
    LLVMBuildBr(ctx->builder, mergeBB);

    LLVMPositionBuilderAtEnd(ctx->builder, falseBB);
    LLVMValueRef falseValue = codegenNode(ctx, node->ternaryExpr.falseExpr);
    LLVMBuildBr(ctx->builder, mergeBB);

    LLVMPositionBuilderAtEnd(ctx->builder, mergeBB);
    LLVMValueRef phi = LLVMBuildPhi(ctx->builder, LLVMInt32TypeInContext(ctx->llvmContext), "ternaryResult");
    LLVMAddIncoming(phi, &trueValue, &trueBB, 1);
    LLVMAddIncoming(phi, &falseValue, &falseBB, 1);

    return phi;
}

static LLVMValueRef codegenBinaryExpression(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_BINARY_EXPRESSION) {
        fprintf(stderr, "Error: Invalid node type for codegenBinaryExpression\n");
        return NULL;
    }

    LLVMValueRef L = codegenNode(ctx, node->expr.left);
    LLVMValueRef R = codegenNode(ctx, node->expr.right);
    if (!L || !R) {
        fprintf(stderr, "Error: Failed to generate operands for binary expression\n");
        return NULL;
    }

    if (strcmp(node->expr.op, "+") == 0) {
        return LLVMBuildAdd(ctx->builder, L, R, "addtmp");
    } else if (strcmp(node->expr.op, "-") == 0) {
        return LLVMBuildSub(ctx->builder, L, R, "subtmp");
    } else if (strcmp(node->expr.op, "*") == 0) {
        return LLVMBuildMul(ctx->builder, L, R, "multmp");
    } else if (strcmp(node->expr.op, "/") == 0) {
        return LLVMBuildSDiv(ctx->builder, L, R, "divtmp");
    } else if (strcmp(node->expr.op, "==") == 0) {
        return LLVMBuildICmp(ctx->builder, LLVMIntEQ, L, R, "eqtmp");
    } else if (strcmp(node->expr.op, "!=") == 0) {
        return LLVMBuildICmp(ctx->builder, LLVMIntNE, L, R, "netmp");
    } else if (strcmp(node->expr.op, "<") == 0) {
        return LLVMBuildICmp(ctx->builder, LLVMIntSLT, L, R, "lttmp");
    } else if (strcmp(node->expr.op, "<=") == 0) {
        return LLVMBuildICmp(ctx->builder, LLVMIntSLE, L, R, "letmp");
    } else if (strcmp(node->expr.op, ">") == 0) {
        return LLVMBuildICmp(ctx->builder, LLVMIntSGT, L, R, "gttmp");
    } else if (strcmp(node->expr.op, ">=") == 0) {
        return LLVMBuildICmp(ctx->builder, LLVMIntSGE, L, R, "getmp");
    }

    fprintf(stderr, "Error: Unsupported binary operator %s\n", node->expr.op);
    return NULL;
}

static LLVMValueRef codegenUnaryExpression(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_UNARY_EXPRESSION) {
        fprintf(stderr, "Error: Invalid node type for codegenUnaryExpression\n");
        return NULL;
    }

    if (node->expr.op && (strcmp(node->expr.op, "++") == 0 || strcmp(node->expr.op, "--") == 0)) {
        bool isIncrement = (node->expr.op[0] == '+');
        bool isPostfix = node->expr.isPostfix;

        LLVMValueRef targetPtr = NULL;
        LLVMTypeRef targetType = NULL;
        if (!codegenLValue(ctx, node->expr.left, &targetPtr, &targetType)) {
            fprintf(stderr, "Error: ++/-- operand must be assignable\n");
            return NULL;
        }

        if (!targetType || LLVMGetTypeKind(targetType) != LLVMIntegerTypeKind) {
            targetType = LLVMInt32TypeInContext(ctx->llvmContext);
        }

        LLVMValueRef current = LLVMBuildLoad2(ctx->builder, targetType, targetPtr, "unary.load");
        LLVMValueRef one = LLVMConstInt(targetType, 1, 0);
        LLVMValueRef updated = isIncrement
            ? LLVMBuildAdd(ctx->builder, current, one, "unary.inc")
            : LLVMBuildSub(ctx->builder, current, one, "unary.dec");

        LLVMBuildStore(ctx->builder, updated, targetPtr);
        return isPostfix ? current : updated;
    }

    LLVMValueRef operand = codegenNode(ctx, node->expr.left);
    if (!operand) {
        fprintf(stderr, "Error: Failed to generate operand for unary expression\n");
        return NULL;
    }

    if (strcmp(node->expr.op, "-") == 0) {
        return LLVMBuildNeg(ctx->builder, operand, "negtmp");
    } else if (strcmp(node->expr.op, "!") == 0) {
        return LLVMBuildNot(ctx->builder, operand, "nottmp");
    } else if (strcmp(node->expr.op, "&") == 0) {
        fprintf(stderr, "Error: Address-of (&) operator requires variables, not implemented yet.\n");
        return NULL;
    } else if (strcmp(node->expr.op, "*") == 0) {
        return LLVMBuildLoad2(ctx->builder, LLVMInt32TypeInContext(ctx->llvmContext), operand, "loadtmp");
    }

    fprintf(stderr, "Error: Unsupported unary operator %s\n", node->expr.op);
    return NULL;
}

static LLVMValueRef codegenCastExpression(CodegenContext* ctx, ASTNode* node) {
    if (!ctx || !node || node->type != AST_CAST_EXPRESSION) {
        fprintf(stderr, "Error: Invalid node type for codegenCastExpression\n");
        return NULL;
    }

    LLVMValueRef value = codegenNode(ctx, node->castExpr.expression);
    if (!value) {
        return NULL;
    }

    LLVMTypeRef target = cg_type_from_parsed(ctx, &node->castExpr.castType);
    if (!target || LLVMGetTypeKind(target) == LLVMVoidTypeKind) {
        return value; /* no conversion performed yet */
    }

    LLVMTypeRef valueType = LLVMTypeOf(value);
    if (valueType == target) {
        return value;
    }

    if (LLVMGetTypeKind(target) == LLVMPointerTypeKind && LLVMGetTypeKind(valueType) == LLVMPointerTypeKind) {
        return LLVMBuildBitCast(ctx->builder, value, target, "ptrcast");
    }

    if (LLVMGetTypeKind(target) == LLVMIntegerTypeKind && LLVMGetTypeKind(valueType) == LLVMIntegerTypeKind) {
        unsigned valueBits = LLVMGetIntTypeWidth(valueType);
        unsigned targetBits = LLVMGetIntTypeWidth(target);
        if (valueBits == targetBits) {
            return LLVMBuildBitCast(ctx->builder, value, target, "intcast");
        } else if (valueBits < targetBits) {
            return LLVMBuildSExt(ctx->builder, value, target, "extcast");
        } else {
            return LLVMBuildTrunc(ctx->builder, value, target, "truncast");
        }
    }

    /* Fallback: rely on bitcast even if the types differ (TODO: semantic-driven casts). */
    return LLVMBuildBitCast(ctx->builder, value, target, "casttmp");
}

static LLVMValueRef codegenVariableDeclaration(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_VARIABLE_DECLARATION) {
        fprintf(stderr, "Error: Invalid node type for codegenVariableDeclaration\n");
        return NULL;
    }

    fprintf(stderr, "[CG] Var decl count=%zu\n", node->varDecl.varCount);
    if (!ctx->currentScope || ctx->currentScope->parent == NULL) {
        return NULL;
    }

    LLVMTypeRef varType = cg_type_from_parsed(ctx, &node->varDecl.declaredType);
    if (!varType || LLVMGetTypeKind(varType) == LLVMVoidTypeKind) {
        varType = LLVMInt32TypeInContext(cg_context_get_llvm_context(ctx));
    }
    for (size_t i = 0; i < node->varDecl.varCount; i++) {
        ASTNode* varNameNode = node->varDecl.varNames[i];
        LLVMValueRef allocaInst = LLVMBuildAlloca(ctx->builder, varType, varNameNode->valueNode.value);
        cg_scope_insert(ctx->currentScope,
                        varNameNode->valueNode.value,
                        allocaInst,
                        varType,
                        false);

        DesignatedInit* init = node->varDecl.initializers ? node->varDecl.initializers[i] : NULL;
        if (init && init->expression) {
            LLVMValueRef initValue = codegenNode(ctx, init->expression);
            if (initValue) {
                LLVMBuildStore(ctx->builder, initValue, allocaInst);
            }
        }
    }

    return NULL;
}

static LLVMValueRef codegenAssignment(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_ASSIGNMENT) {
        fprintf(stderr, "Error: Invalid node type for codegenAssignment\n");
        return NULL;
    }

    fprintf(stderr, "[CG] Assignment node\n");
    ASTNode* target = node->assignment.target;
    LLVMValueRef targetPtr = NULL;
    LLVMTypeRef targetType = NULL;
    if (!codegenLValue(ctx, target, &targetPtr, &targetType)) {
        fprintf(stderr, "Error: Unsupported assignment target type %d\n", target ? target->type : -1);
        return NULL;
    }

    LLVMValueRef value = codegenNode(ctx, node->assignment.value);
    if (!value) {
        fprintf(stderr, "Error: Assignment value failed to generate\n");
        return NULL;
    }

    const char* op = node->assignment.op ? node->assignment.op : "=";
    LLVMTypeRef storeType = targetType;
    if (!storeType || LLVMGetTypeKind(storeType) == LLVMVoidTypeKind) {
        storeType = LLVMGetElementType(LLVMTypeOf(targetPtr));
    }
    if (!storeType || LLVMGetTypeKind(storeType) == LLVMVoidTypeKind) {
        storeType = LLVMInt32TypeInContext(ctx->llvmContext);
    }

    LLVMValueRef storedValue = value;

    if (op && strcmp(op, "=") != 0) {
        LLVMValueRef current = LLVMBuildLoad2(ctx->builder, storeType, targetPtr, "compound.load");
        if (!current) {
            fprintf(stderr, "Error: Failed to load target for compound assignment\n");
            return NULL;
        }

        if (strcmp(op, "+=") == 0) {
            storedValue = LLVMBuildAdd(ctx->builder, current, value, "compound.add");
        } else if (strcmp(op, "-=") == 0) {
            storedValue = LLVMBuildSub(ctx->builder, current, value, "compound.sub");
        } else if (strcmp(op, "*=") == 0) {
            storedValue = LLVMBuildMul(ctx->builder, current, value, "compound.mul");
        } else if (strcmp(op, "/=") == 0) {
            storedValue = LLVMBuildSDiv(ctx->builder, current, value, "compound.div");
        } else if (strcmp(op, "%=") == 0) {
            storedValue = LLVMBuildSRem(ctx->builder, current, value, "compound.rem");
        } else {
            fprintf(stderr, "Error: Unsupported compound assignment operator %s\n", op);
            return NULL;
        }
    }

    return LLVMBuildStore(ctx->builder, storedValue, targetPtr);
}

static LLVMValueRef codegenFunctionDefinition(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_FUNCTION_DEFINITION) {
        fprintf(stderr, "Error: Invalid node type for codegenFunctionDefinition\n");
        return NULL;
    }

    const char* fnName = (node->functionDef.funcName && node->functionDef.funcName->type == AST_IDENTIFIER)
        ? node->functionDef.funcName->valueNode.value
        : "<anon>";
    fprintf(stderr, "[CG] Function definition %s paramCount=%zu\n", fnName, node->functionDef.paramCount);
    LLVMTypeRef returnType = cg_type_from_parsed(ctx, &node->functionDef.returnType);
    if (!returnType) {
        returnType = LLVMVoidTypeInContext(ctx->llvmContext);
    }

    size_t paramCount = node->functionDef.paramCount;
    LLVMTypeRef* paramTypes = NULL;
    if (paramCount > 0) {
        paramTypes = (LLVMTypeRef*)malloc(sizeof(LLVMTypeRef) * paramCount);
        if (!paramTypes) return NULL;
        for (size_t i = 0; i < paramCount; i++) {
            LLVMTypeRef inferred = NULL;
            ASTNode* paramDecl = node->functionDef.parameters[i];
            if (paramDecl && paramDecl->type == AST_VARIABLE_DECLARATION) {
                inferred = cg_type_from_parsed(ctx, &paramDecl->varDecl.declaredType);
            }
            if (!inferred || LLVMGetTypeKind(inferred) == LLVMVoidTypeKind) {
                inferred = LLVMInt32TypeInContext(ctx->llvmContext);
            }
            paramTypes[i] = inferred;
        }
    }

    LLVMTypeRef funcType = LLVMFunctionType(returnType, paramTypes, paramCount, 0);
    LLVMValueRef function = LLVMAddFunction(ctx->module,
                                            node->functionDef.funcName->valueNode.value,
                                            funcType);

    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(function, "entry");
    LLVMPositionBuilderAtEnd(ctx->builder, entry);

    cg_scope_push(ctx);

    for (size_t i = 0; i < paramCount; i++) {
        ASTNode* paramDecl = node->functionDef.parameters[i];
        if (!paramDecl || paramDecl->type != AST_VARIABLE_DECLARATION) continue;
        for (size_t j = 0; j < paramDecl->varDecl.varCount; j++) {
            ASTNode* paramName = paramDecl->varDecl.varNames[j];
            LLVMTypeRef paramType = paramTypes ? paramTypes[i] : LLVMInt32TypeInContext(ctx->llvmContext);
            LLVMValueRef allocaInst = LLVMBuildAlloca(ctx->builder,
                                                      paramType,
                                                      paramName->valueNode.value);
            LLVMBuildStore(ctx->builder, LLVMGetParam(function, (unsigned)i), allocaInst);
            cg_scope_insert(ctx->currentScope,
                            paramName->valueNode.value,
                            allocaInst,
                            paramType,
                            false);
        }
    }

    codegenNode(ctx, node->functionDef.body);

    cg_scope_pop(ctx);
    free(paramTypes);
    return function;
}

static LLVMValueRef codegenFunctionCall(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_FUNCTION_CALL) {
        fprintf(stderr, "Error: Invalid node type for codegenFunctionCall\n");
        return NULL;
    }

    if (!node->functionCall.callee || node->functionCall.callee->type != AST_IDENTIFIER) {
        fprintf(stderr, "Error: Unsupported callee in function call\n");
        return NULL;
    }

    LLVMValueRef function = LLVMGetNamedFunction(ctx->module,
                                                 node->functionCall.callee->valueNode.value);
    if (!function) {
        fprintf(stderr, "Error: Undefined function %s\n",
                node->functionCall.callee->valueNode.value);
        return NULL;
    }

    char* rawFnType = LLVMPrintTypeToString(LLVMTypeOf(function));
    fprintf(stderr, "[CG] Function call raw type: %s\n", rawFnType ? rawFnType : "<null>");
    if (rawFnType) LLVMDisposeMessage(rawFnType);

    LLVMValueRef* args = NULL;
    if (node->functionCall.argumentCount > 0) {
        args = (LLVMValueRef*)malloc(sizeof(LLVMValueRef) * node->functionCall.argumentCount);
        if (!args) return NULL;
        for (size_t i = 0; i < node->functionCall.argumentCount; i++) {
            args[i] = codegenNode(ctx, node->functionCall.arguments[i]);
        }
    }

    LLVMTypeRef calleeType = NULL;
    const SemanticModel* model = cg_context_get_semantic_model(ctx);
    if (model) {
        const char* name = node->functionCall.callee->valueNode.value;
        const Symbol* sym = semanticModelLookupGlobal(model, name);
        if (sym && sym->kind == SYMBOL_FUNCTION) {
            LLVMTypeRef returnType = cg_type_from_parsed(ctx, &sym->type);
            if (!returnType || LLVMGetTypeKind(returnType) == LLVMVoidTypeKind) {
                returnType = LLVMVoidTypeInContext(ctx->llvmContext);
            }

            size_t paramCount = 0;
            ASTNode** params = NULL;
            if (sym->definition) {
                if (sym->definition->type == AST_FUNCTION_DEFINITION) {
                    paramCount = sym->definition->functionDef.paramCount;
                    params = sym->definition->functionDef.parameters;
                } else if (sym->definition->type == AST_FUNCTION_DECLARATION) {
                    paramCount = sym->definition->functionDecl.paramCount;
                    params = sym->definition->functionDecl.parameters;
                }
            }

            LLVMTypeRef* paramTypes = collectParamTypes(ctx, paramCount, params);
            calleeType = LLVMFunctionType(returnType, paramTypes, (unsigned)paramCount, 0);
            free(paramTypes);
        }
    }

    if (!calleeType) {
        calleeType = LLVMTypeOf(function);
        if (LLVMGetTypeKind(calleeType) == LLVMPointerTypeKind) {
            LLVMTypeRef element = LLVMGetElementType(calleeType);
            if (element) {
                calleeType = element;
            }
        }
    }

    char* resolvedFnType = LLVMPrintTypeToString(calleeType);
    fprintf(stderr, "[CG] Function call resolved type: %s\n", resolvedFnType ? resolvedFnType : "<null>");
    if (resolvedFnType) LLVMDisposeMessage(resolvedFnType);

    LLVMValueRef call = LLVMBuildCall2(ctx->builder,
                                       calleeType,
                                       function,
                                       args,
                                       node->functionCall.argumentCount,
                                       "calltmp");
    free(args);
    return call;
}

static LLVMValueRef codegenTypedef(CodegenContext* ctx, ASTNode* node) {
    if (!ctx || !node || node->type != AST_TYPEDEF) {
        fprintf(stderr, "Error: Invalid node type for codegenTypedef\n");
        return NULL;
    }

    fprintf(stderr, "[CG] Typedef encountered\n");
    const char* aliasName = (node->typedefStmt.alias && node->typedefStmt.alias->type == AST_IDENTIFIER)
        ? node->typedefStmt.alias->valueNode.value
        : NULL;
    if (!aliasName || aliasName[0] == '\0') {
        return NULL;
    }

    if (cg_context_get_type_cache(ctx)) {
        return NULL; /* semantic model already contributed this typedef */
    }

    LLVMTypeRef aliased = cg_type_from_parsed(ctx, &node->typedefStmt.baseType);
    if (!aliased || LLVMGetTypeKind(aliased) == LLVMVoidTypeKind) {
        aliased = LLVMInt32TypeInContext(ctx->llvmContext);
    }
    cg_context_cache_named_type(ctx, aliasName, aliased);
    return NULL;
}

static LLVMValueRef codegenEnumDefinition(CodegenContext* ctx, ASTNode* node) {
    if (!ctx || !node || node->type != AST_ENUM_DEFINITION) {
        fprintf(stderr, "Error: Invalid node type for codegenEnumDefinition\n");
        return NULL;
    }
    (void)ctx;
    /* Currently enums only contribute constants during semantics; IR emission is a no-op. */
    return NULL;
}

static LLVMValueRef codegenIfStatement(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_IF_STATEMENT) {
        fprintf(stderr, "Error: Invalid node type for codegenIfStatement\n");
        return NULL;
    }

    LLVMValueRef cond = codegenNode(ctx, node->ifStmt.condition);
    if (!cond) {
        fprintf(stderr, "Error: Failed to generate condition for if statement\n");
        return NULL;
    }

    cond = LLVMBuildICmp(ctx->builder, LLVMIntNE, cond,
                         LLVMConstInt(LLVMInt32TypeInContext(ctx->llvmContext), 0, 0),
                         "ifcond");

    LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(ctx->builder));
    LLVMBasicBlockRef thenBB = LLVMAppendBasicBlock(func, "then");
    LLVMBasicBlockRef elseBB = node->ifStmt.elseBranch ? LLVMAppendBasicBlock(func, "else") : NULL;
    LLVMBasicBlockRef mergeBB = LLVMAppendBasicBlock(func, "ifcont");

    LLVMBuildCondBr(ctx->builder, cond, thenBB, elseBB ? elseBB : mergeBB);

    LLVMPositionBuilderAtEnd(ctx->builder, thenBB);
    codegenNode(ctx, node->ifStmt.thenBranch);
    if (!LLVMGetInsertBlock(ctx->builder) || !LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(ctx->builder))) {
        LLVMBuildBr(ctx->builder, mergeBB);
    }

    if (elseBB) {
        LLVMPositionBuilderAtEnd(ctx->builder, elseBB);
        codegenNode(ctx, node->ifStmt.elseBranch);
        if (!LLVMGetInsertBlock(ctx->builder) || !LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(ctx->builder))) {
            LLVMBuildBr(ctx->builder, mergeBB);
        }
    }

    LLVMPositionBuilderAtEnd(ctx->builder, mergeBB);
    return NULL;
}

static LLVMValueRef codegenWhileLoop(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_WHILE_LOOP) {
        fprintf(stderr, "Error: Invalid node type for codegenWhileLoop\n");
        return NULL;
    }

    LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(ctx->builder));
    LLVMBasicBlockRef condBB = LLVMAppendBasicBlock(func, "loopcond");
    LLVMBasicBlockRef bodyBB = LLVMAppendBasicBlock(func, "loopbody");
    LLVMBasicBlockRef afterBB = LLVMAppendBasicBlock(func, "loopend");

    if (!node->whileLoop.isDoWhile) {
        LLVMBuildBr(ctx->builder, condBB);
    }

    LLVMPositionBuilderAtEnd(ctx->builder, condBB);
    LLVMValueRef cond = codegenNode(ctx, node->whileLoop.condition);
    cond = LLVMBuildICmp(ctx->builder, LLVMIntNE, cond,
                         LLVMConstInt(LLVMInt32TypeInContext(ctx->llvmContext), 0, 0),
                         "loopcond");
    LLVMBuildCondBr(ctx->builder, cond, bodyBB, afterBB);

    LLVMPositionBuilderAtEnd(ctx->builder, bodyBB);
    cg_loop_push(ctx, afterBB, condBB);
    codegenNode(ctx, node->whileLoop.body);
    cg_loop_pop(ctx);
    LLVMBuildBr(ctx->builder, condBB);

    LLVMPositionBuilderAtEnd(ctx->builder, afterBB);
    return NULL;
}

static LLVMValueRef codegenForLoop(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_FOR_LOOP) {
        fprintf(stderr, "Error: Invalid node type for codegenForLoop\n");
        return NULL;
    }

    LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(ctx->builder));
    LLVMBasicBlockRef condBB = LLVMAppendBasicBlock(func, "forcond");
    LLVMBasicBlockRef bodyBB = LLVMAppendBasicBlock(func, "forbody");
    LLVMBasicBlockRef incBB = LLVMAppendBasicBlock(func, "forinc");
    LLVMBasicBlockRef afterBB = LLVMAppendBasicBlock(func, "forend");

    if (node->forLoop.initializer) {
        codegenNode(ctx, node->forLoop.initializer);
    }

    LLVMBuildBr(ctx->builder, condBB);

    LLVMPositionBuilderAtEnd(ctx->builder, condBB);
    LLVMValueRef cond = node->forLoop.condition ? codegenNode(ctx, node->forLoop.condition)
                                                : LLVMConstInt(LLVMInt32TypeInContext(ctx->llvmContext), 1, 0);
    cond = LLVMBuildICmp(ctx->builder, LLVMIntNE, cond,
                         LLVMConstInt(LLVMInt32TypeInContext(ctx->llvmContext), 0, 0),
                         "forcond");
    LLVMBuildCondBr(ctx->builder, cond, bodyBB, afterBB);

    LLVMPositionBuilderAtEnd(ctx->builder, bodyBB);
    cg_loop_push(ctx, afterBB, incBB);
    codegenNode(ctx, node->forLoop.body);
    cg_loop_pop(ctx);
    LLVMBuildBr(ctx->builder, incBB);

    LLVMPositionBuilderAtEnd(ctx->builder, incBB);
    if (node->forLoop.increment) {
        codegenNode(ctx, node->forLoop.increment);
    }
    LLVMBuildBr(ctx->builder, condBB);

    LLVMPositionBuilderAtEnd(ctx->builder, afterBB);
    return NULL;
}

static LLVMValueRef codegenReturn(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_RETURN) {
        fprintf(stderr, "Error: Invalid node type for codegenReturn\n");
        return NULL;
    }

    fprintf(stderr, "[CG] Return begin\n");
    if (node->returnStmt.returnValue) {
        LLVMValueRef value = codegenNode(ctx, node->returnStmt.returnValue);
        fprintf(stderr, "[CG] Return emitting with value\n");
        return LLVMBuildRet(ctx->builder, value);
    }
    fprintf(stderr, "[CG] Return emitting void\n");
    return LLVMBuildRetVoid(ctx->builder);
}

static LLVMValueRef codegenBreak(CodegenContext* ctx, ASTNode* node) {
    (void)node;
    LoopTarget target = cg_loop_peek(ctx);
    if (!target.breakBB) {
        fprintf(stderr, "Error: 'break' used outside of loop\n");
        return NULL;
    }
    return LLVMBuildBr(ctx->builder, target.breakBB);
}

static LLVMValueRef codegenContinue(CodegenContext* ctx, ASTNode* node) {
    (void)node;
    LoopTarget target = cg_loop_peek(ctx);
    if (!target.continueBB) {
        fprintf(stderr, "Error: 'continue' used outside of loop\n");
        return NULL;
    }
    return LLVMBuildBr(ctx->builder, target.continueBB);
}

static LLVMValueRef codegenSwitch(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_SWITCH) {
        fprintf(stderr, "Error: Invalid node type for codegenSwitch\n");
        return NULL;
    }

    LLVMValueRef condition = codegenNode(ctx, node->switchStmt.condition);
    if (!condition) {
        fprintf(stderr, "Error: Failed to generate switch condition\n");
        return NULL;
    }

    LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(ctx->builder));
    LLVMBasicBlockRef switchEnd = LLVMAppendBasicBlock(func, "switchEnd");

    LLVMValueRef switchInst = LLVMBuildSwitch(ctx->builder, condition, switchEnd,
                                              node->switchStmt.caseListSize);

    for (size_t i = 0; i < node->switchStmt.caseListSize; i++) {
        codegenCase(ctx, node->switchStmt.caseList[i], switchInst, switchEnd);
    }

    LLVMPositionBuilderAtEnd(ctx->builder, switchEnd);
    return NULL;
}

static void codegenCase(CodegenContext* ctx, ASTNode* node, LLVMValueRef switchInst,
                        LLVMBasicBlockRef switchEnd) {
    if (!node || node->type != AST_CASE) {
        fprintf(stderr, "Error: Invalid node type for codegenCase\n");
        return;
    }

    LLVMValueRef caseValue = codegenNode(ctx, node->caseStmt.caseValue);
    if (!caseValue) {
        fprintf(stderr, "Error: Failed to generate case value\n");
        return;
    }

    LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(ctx->builder));
    LLVMBasicBlockRef caseBB = LLVMAppendBasicBlock(func, "case");

    LLVMAddCase(switchInst, caseValue, caseBB);

    LLVMPositionBuilderAtEnd(ctx->builder, caseBB);
    for (size_t i = 0; i < node->caseStmt.caseBodySize; i++) {
        codegenNode(ctx, node->caseStmt.caseBody[i]);
    }

    if (!LLVMGetInsertBlock(ctx->builder) || !LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(ctx->builder))) {
        LLVMBuildBr(ctx->builder, switchEnd);
    }
}

static LLVMValueRef codegenArrayDeclaration(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_ARRAY_DECLARATION) {
        fprintf(stderr, "Error: Invalid node type for codegenArrayDeclaration\n");
        return NULL;
    }

    LLVMTypeRef elementType = cg_type_from_parsed(ctx, &node->arrayDecl.declaredType);
    if (!elementType || LLVMGetTypeKind(elementType) == LLVMVoidTypeKind) {
        elementType = LLVMInt32TypeInContext(ctx->llvmContext);
    }

    LLVMValueRef computedSize = node->arrayDecl.arraySize ? codegenNode(ctx, node->arrayDecl.arraySize) : NULL;
    if (!computedSize && node->arrayDecl.valueCount == 0) {
        fprintf(stderr, "Error: Array size not specified\n");
        return NULL;
    }

    (void)computedSize;

    LLVMTypeRef arrayType = LLVMArrayType(elementType, node->arrayDecl.valueCount ? (unsigned)node->arrayDecl.valueCount : 1);
    LLVMValueRef array = LLVMBuildAlloca(ctx->builder, arrayType,
                                         node->arrayDecl.varName->valueNode.value);
    cg_scope_insert(ctx->currentScope,
                    node->arrayDecl.varName->valueNode.value,
                    array,
                    LLVMTypeOf(array),
                    false);

    for (size_t i = 0; i < node->arrayDecl.valueCount; i++) {
        LLVMValueRef index = LLVMConstInt(LLVMInt32TypeInContext(ctx->llvmContext), i, 0);
        DesignatedInit* init = node->arrayDecl.initializers ? node->arrayDecl.initializers[i] : NULL;
        if (init && init->expression) {
            LLVMValueRef value = codegenNode(ctx, init->expression);
            if (value) {
                LLVMValueRef elementPtr = LLVMBuildGEP2(ctx->builder, arrayType, array, &index, 1, "elementPtr");
                LLVMBuildStore(ctx->builder, value, elementPtr);
            }
        }
    }

    return array;
}

static LLVMValueRef codegenArrayAccess(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_ARRAY_ACCESS) {
        fprintf(stderr, "Error: Invalid node type for codegenArrayAccess\n");
        return NULL;
    }

    LLVMValueRef arrayPtr = codegenNode(ctx, node->arrayAccess.array);
    LLVMValueRef index = codegenNode(ctx, node->arrayAccess.index);
    if (!arrayPtr || !index) {
        fprintf(stderr, "Error: Array access failed\n");
        return NULL;
    }
    LLVMValueRef elementPtr = buildArrayElementPointer(ctx, arrayPtr, index, NULL);
    if (!elementPtr) {
        fprintf(stderr, "Error: Failed to compute array element pointer\n");
        return NULL;
    }
    LLVMTypeRef elementType = LLVMGetElementType(LLVMTypeOf(elementPtr));
    if (!elementType || LLVMGetTypeKind(elementType) == LLVMVoidTypeKind) {
        elementType = LLVMInt32TypeInContext(ctx->llvmContext);
    }
    return LLVMBuildLoad2(ctx->builder, elementType, elementPtr, "arrayLoad");
}

static LLVMValueRef codegenDotAccess(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_DOT_ACCESS) {
        fprintf(stderr, "Error: Invalid node type for codegenDotAccess\n");
        return NULL;
    }

    fprintf(stderr, "[CG] Dot access begin\n");
    LLVMValueRef basePtr = NULL;
    LLVMTypeRef baseType = NULL;
    if (!codegenLValue(ctx, node->memberAccess.base, &basePtr, &baseType)) {
        fprintf(stderr, "[CG] Dot access fallback to value\n");
        LLVMValueRef baseVal = codegenNode(ctx, node->memberAccess.base);
        if (!baseVal) return NULL;
        if (LLVMGetTypeKind(LLVMTypeOf(baseVal)) == LLVMPointerTypeKind) {
            basePtr = baseVal;
            baseType = LLVMGetElementType(LLVMTypeOf(basePtr));
        } else {
            LLVMValueRef tmpAlloca = LLVMBuildAlloca(ctx->builder, LLVMTypeOf(baseVal), "dot_tmp_ptr");
            LLVMBuildStore(ctx->builder, baseVal, tmpAlloca);
            basePtr = tmpAlloca;
            baseType = LLVMTypeOf(baseVal);
        }
    }

    const char* nameHint = NULL;
    if (baseType && LLVMGetTypeKind(baseType) == LLVMStructTypeKind) {
        nameHint = LLVMGetStructName(baseType);
    }

    LLVMTypeRef fieldType = NULL;
    LLVMValueRef fieldPtr = buildStructFieldPointer(ctx, basePtr, baseType, nameHint, node->memberAccess.field, &fieldType);
    if (!fieldPtr) {
        char* tyStr = LLVMPrintTypeToString(LLVMTypeOf(basePtr));
        fprintf(stderr, "DEBUG: dot access base type %s\n", tyStr ? tyStr : "<null>");
        if (tyStr) { LLVMDisposeMessage(tyStr); }
        fprintf(stderr, "Error: Unknown field '%s'\n", node->memberAccess.field);
        return NULL;
    }
    char* ptrTyStr = LLVMPrintTypeToString(LLVMTypeOf(fieldPtr));
    fprintf(stderr, "[CG] Dot access got field pointer type=%s\n", ptrTyStr ? ptrTyStr : "<null>");
    if (ptrTyStr) LLVMDisposeMessage(ptrTyStr);
    if (!fieldType || LLVMGetTypeKind(fieldType) == LLVMVoidTypeKind) {
        fieldType = LLVMInt32TypeInContext(ctx->llvmContext);
    }
    char* fieldTyStr = LLVMPrintTypeToString(fieldType);
    fprintf(stderr, "[CG] Dot access loading type=%s\n", fieldTyStr ? fieldTyStr : "<null>");
    if (fieldTyStr) LLVMDisposeMessage(fieldTyStr);
    LLVMValueRef result = LLVMBuildLoad2(ctx->builder, fieldType, fieldPtr, "dot_field");
    fprintf(stderr, "[CG] Dot access load complete\n");
    return result;
}

static LLVMValueRef codegenPointerAccess(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_POINTER_ACCESS) {
        fprintf(stderr, "Error: Invalid node type for codegenPointerAccess\n");
        return NULL;
    }

    LLVMValueRef basePtr = codegenNode(ctx, node->memberAccess.base);
    if (!basePtr) return NULL;
    LLVMTypeRef basePtrType = LLVMTypeOf(basePtr);
    LLVMTypeRef aggregateType = NULL;
    const char* nameHint = NULL;
    if (LLVMGetTypeKind(basePtrType) == LLVMPointerTypeKind) {
        aggregateType = LLVMGetElementType(basePtrType);
        if (aggregateType && LLVMGetTypeKind(aggregateType) == LLVMStructTypeKind) {
            nameHint = LLVMGetStructName(aggregateType);
        }
    }
    LLVMTypeRef fieldType = NULL;
    LLVMValueRef fieldPtr = buildStructFieldPointer(ctx, basePtr, aggregateType, nameHint, node->memberAccess.field, &fieldType);
    if (!fieldPtr) {
        fprintf(stderr, "Error: Unknown field '%s'\n", node->memberAccess.field);
        return NULL;
    }
    if (!fieldType || LLVMGetTypeKind(fieldType) == LLVMVoidTypeKind) {
        fieldType = LLVMInt32TypeInContext(ctx->llvmContext);
    }
    return LLVMBuildLoad2(ctx->builder, fieldType, fieldPtr, "ptr_field");
}

static LLVMValueRef codegenPointerDereference(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_POINTER_DEREFERENCE) {
        fprintf(stderr, "Error: Invalid node type for codegenPointerDereference\n");
        return NULL;
    }

    LLVMValueRef pointer = codegenNode(ctx, node->pointerDeref.pointer);
    if (!pointer) {
        fprintf(stderr, "Error: Failed to generate pointer dereference\n");
        return NULL;
    }
    LLVMTypeRef elemType = LLVMGetElementType(LLVMTypeOf(pointer));
    if (!elemType || LLVMGetTypeKind(elemType) == LLVMVoidTypeKind) {
        elemType = LLVMInt32TypeInContext(ctx->llvmContext);
    }
    return LLVMBuildLoad2(ctx->builder, elemType, pointer, "ptrLoad");
}

static LLVMTypeRef codegenStructDefinition(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_STRUCT_DEFINITION && node->type != AST_UNION_DEFINITION) {
        fprintf(stderr, "Error: Invalid node type for codegenStructDefinition\n");
        return NULL;
    }

    fprintf(stderr, "[CG] Struct/Union definition node\n");
    ASTNode* nameNode = node->structDef.structName;
    if (!nameNode || nameNode->type != AST_IDENTIFIER) {
        fprintf(stderr, "Error: Anonymous structs/unions not yet supported in codegen\n");
        return NULL;
    }
    const char* structName = nameNode->valueNode.value;
    if (!structName || structName[0] == '\0') {
        fprintf(stderr, "Error: Struct name missing during codegen\n");
        return NULL;
    }

    bool isUnion = (node->type == AST_UNION_DEFINITION);
    LLVMTypeRef structType = ensureStructLLVMTypeByName(ctx, structName, isUnion);
    if (!structType) {
        structType = cg_context_lookup_named_type(ctx, structName);
    }
    if (!structType) {
        structType = LLVMStructCreateNamed(ctx->llvmContext, structName);
        cg_context_cache_named_type(ctx, structName, structType);
    }

    if (!LLVMIsOpaqueStruct(structType)) {
        return structType;
    }

    size_t totalFields = 0;
    for (size_t i = 0; i < node->structDef.fieldCount; ++i) {
        ASTNode* fieldNode = node->structDef.fields[i];
        if (!fieldNode) continue;
        if (fieldNode->type == AST_VARIABLE_DECLARATION) {
            totalFields += fieldNode->varDecl.varCount;
        }
    }

    if (totalFields == 0) {
        LLVMStructSetBody(structType, NULL, 0, 0);
        recordStructInfo(ctx, structName, isUnion, NULL, 0, structType);
        return structType;
    }

    LLVMTypeRef* fieldTypes = (LLVMTypeRef*)malloc(sizeof(LLVMTypeRef) * totalFields);
    StructFieldInfo* infos = (StructFieldInfo*)calloc(totalFields, sizeof(StructFieldInfo));
    if (!fieldTypes || !infos) {
        free(fieldTypes);
        free(infos);
        return NULL;
    }

    size_t fieldIndex = 0;
    for (size_t i = 0; i < node->structDef.fieldCount; ++i) {
        ASTNode* fieldNode = node->structDef.fields[i];
        if (!fieldNode) continue;
        if (fieldNode->type != AST_VARIABLE_DECLARATION) {
            fprintf(stderr, "Warning: Unsupported struct member node type %d\n", fieldNode->type);
            continue;
        }

        LLVMTypeRef memberType = cg_type_from_parsed(ctx, &fieldNode->varDecl.declaredType);
        if (!memberType || LLVMGetTypeKind(memberType) == LLVMVoidTypeKind) {
            memberType = LLVMInt32TypeInContext(ctx->llvmContext);
        }
        char* memberTypeStr = LLVMPrintTypeToString(memberType);
        fprintf(stderr, "[CG] Struct member type=%s\n", memberTypeStr ? memberTypeStr : "<null>");
        if (memberTypeStr) LLVMDisposeMessage(memberTypeStr);

        for (size_t v = 0; v < fieldNode->varDecl.varCount; ++v) {
            if (fieldIndex >= totalFields) break;
            ASTNode* nameNodeField = fieldNode->varDecl.varNames[v];
            const char* fname = (nameNodeField && nameNodeField->type == AST_IDENTIFIER)
                ? nameNodeField->valueNode.value
                : NULL;

            fieldTypes[fieldIndex] = memberType;
            infos[fieldIndex].name = fname ? strdup(fname) : NULL;
            infos[fieldIndex].index = isUnion ? 0 : (unsigned)fieldIndex;
            infos[fieldIndex].type = memberType;
            fieldIndex++;
        }
    }

    LLVMStructSetBody(structType, fieldTypes, (unsigned)fieldIndex, 0);
    recordStructInfo(ctx, structName, isUnion, infos, fieldIndex, structType);
    free(fieldTypes);
    return structType;
}

static LLVMValueRef codegenStructFieldAccess(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_STRUCT_FIELD_ACCESS) {
        fprintf(stderr, "Error: Invalid node type for codegenStructFieldAccess\n");
        return NULL;
    }

    LLVMValueRef structPtr = NULL;
    LLVMTypeRef baseType = NULL;
    if (!codegenLValue(ctx, node->structFieldAccess.structInstance, &structPtr, &baseType)) {
        structPtr = codegenNode(ctx, node->structFieldAccess.structInstance);
        if (!structPtr) {
            fprintf(stderr, "Error: Failed to generate struct instance\n");
            return NULL;
        }
        baseType = LLVMTypeOf(structPtr);
        if (LLVMGetTypeKind(baseType) == LLVMPointerTypeKind) {
            baseType = LLVMGetElementType(baseType);
        }
    }

    const char* nameHint = NULL;
    if (baseType && LLVMGetTypeKind(baseType) == LLVMStructTypeKind) {
        nameHint = LLVMGetStructName(baseType);
    }

    LLVMTypeRef fieldType = NULL;
    LLVMValueRef fieldPtr = buildStructFieldPointer(ctx, structPtr, baseType, nameHint, node->structFieldAccess.fieldName, &fieldType);
    if (!fieldPtr) {
        fprintf(stderr, "Error: Unknown struct field %s\n", node->structFieldAccess.fieldName);
        return NULL;
    }

    if (!fieldType || LLVMGetTypeKind(fieldType) == LLVMVoidTypeKind) {
        fieldType = LLVMInt32TypeInContext(ctx->llvmContext);
    }

    return LLVMBuildLoad2(ctx->builder, fieldType, fieldPtr, "fieldLoad");
}

static LLVMValueRef codegenHeapAllocation(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_HEAP_ALLOCATION) {
        fprintf(stderr, "Error: Invalid node type for codegenHeapAllocation\n");
        return NULL;
    }

    LLVMTypeRef allocType = LLVMStructType(NULL, 0, 0);
    LLVMValueRef size = LLVMSizeOf(allocType);
    LLVMValueRef mallocFunc = LLVMGetNamedFunction(ctx->module, "malloc");

    if (!mallocFunc) {
        fprintf(stderr, "Error: malloc function not found\n");
        return NULL;
    }

    LLVMValueRef args[] = {size};
    return LLVMBuildCall2(ctx->builder, LLVMTypeOf(mallocFunc), mallocFunc, args, 1, "mallocCall");
}

static LLVMValueRef codegenSizeof(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_SIZEOF) {
        fprintf(stderr, "Error: Invalid node type for codegenSizeof\n");
        return NULL;
    }

    LLVMTypeRef type = LLVMInt32TypeInContext(ctx->llvmContext);
    if (node->expr.left) {
        ASTNode* operand = node->expr.left;
        if (operand->type == AST_PARSED_TYPE) {
            type = cg_type_from_parsed(ctx, &operand->parsedTypeNode.parsed);
        } else {
            LLVMValueRef value = codegenNode(ctx, operand);
            if (value) {
                type = LLVMTypeOf(value);
            }
        }
    }

    return LLVMSizeOf(type);
}

static LLVMValueRef codegenBlock(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_BLOCK) {
        fprintf(stderr, "Error: Invalid node type for codegenBlock\n");
        return NULL;
    }

    fprintf(stderr, "[CG] Block statements=%zu\n", node->block.statementCount);
    cg_scope_push(ctx);
    LLVMValueRef last = NULL;
    for (size_t i = 0; i < node->block.statementCount; i++) {
        last = codegenNode(ctx, node->block.statements[i]);
    }
    cg_scope_pop(ctx);
    return last;
}

static LLVMValueRef codegenProgram(CodegenContext* ctx, ASTNode* node) {
    if (node->type != AST_PROGRAM) {
        fprintf(stderr, "Error: Invalid node type for codegenProgram\n");
        return NULL;
    }

    fprintf(stderr, "[CG] Program statements=%zu\n", node->block.statementCount);
    LLVMValueRef last = NULL;
    for (size_t i = 0; i < node->block.statementCount; i++) {
        last = codegenNode(ctx, node->block.statements[i]);
    }
    return last;
}

static CGScope* cg_scope_create(CGScope* parent) {
    CGScope* scope = (CGScope*)calloc(1, sizeof(CGScope));
    if (!scope) return NULL;
    scope->parent = parent;
    return scope;
}

static void cg_scope_destroy(CGScope* scope) {
    if (!scope) return;
    for (size_t i = 0; i < scope->count; i++) {
        free(scope->entries[i].name);
    }
    free(scope->entries);
    free(scope);
}

static CGScope* cg_scope_push(CodegenContext* ctx) {
    if (!ctx) return NULL;
    CGScope* scope = cg_scope_create(ctx->currentScope);
    if (!scope) return NULL;
    ctx->currentScope = scope;
    return scope;
}

static void cg_scope_pop(CodegenContext* ctx) {
    if (!ctx || !ctx->currentScope) return;
    CGScope* parent = ctx->currentScope->parent;
    cg_scope_destroy(ctx->currentScope);
    ctx->currentScope = parent;
}

static void cg_scope_insert(CGScope* scope, const char* name, LLVMValueRef value, LLVMTypeRef type, bool isGlobal) {
    if (!scope || !name) return;

    for (size_t i = 0; i < scope->count; i++) {
        if (strcmp(scope->entries[i].name, name) == 0) {
            scope->entries[i].value = value;
            scope->entries[i].type = type;
            scope->entries[i].isGlobal = isGlobal;
            return;
        }
    }

    if (scope->count == scope->capacity) {
        size_t newCap = scope->capacity ? scope->capacity * 2 : 4;
        NamedValue* resized = (NamedValue*)realloc(scope->entries, newCap * sizeof(NamedValue));
        if (!resized) return;
        scope->entries = resized;
        scope->capacity = newCap;
    }

    scope->entries[scope->count].name = strdup(name);
    scope->entries[scope->count].value = value;
    scope->entries[scope->count].type = type;
    scope->entries[scope->count].isGlobal = isGlobal;
    scope->count += 1;
}

static NamedValue* cg_scope_lookup(CGScope* scope, const char* name) {
    for (CGScope* it = scope; it; it = it->parent) {
        for (size_t i = 0; i < it->count; i++) {
            if (strcmp(it->entries[i].name, name) == 0) {
                return &it->entries[i];
            }
        }
    }
    return NULL;
}

static void cg_loop_push(CodegenContext* ctx, LLVMBasicBlockRef breakBB, LLVMBasicBlockRef continueBB) {
    if (!ctx) return;
    if (ctx->loopStackSize == ctx->loopStackCapacity) {
        size_t newCap = ctx->loopStackCapacity ? ctx->loopStackCapacity * 2 : 4;
        LoopTarget* resized = (LoopTarget*)realloc(ctx->loopStack, newCap * sizeof(LoopTarget));
        if (!resized) return;
        ctx->loopStack = resized;
        ctx->loopStackCapacity = newCap;
    }

    ctx->loopStack[ctx->loopStackSize++] = (LoopTarget){breakBB, continueBB};
}

static void cg_loop_pop(CodegenContext* ctx) {
    if (!ctx || ctx->loopStackSize == 0) return;
    ctx->loopStackSize -= 1;
}

static LoopTarget cg_loop_peek(const CodegenContext* ctx) {
    LoopTarget empty = {NULL, NULL};
    if (!ctx || ctx->loopStackSize == 0) return empty;
    return ctx->loopStack[ctx->loopStackSize - 1];
}

static LLVMTypeRef* collectParamTypes(CodegenContext* ctx, size_t paramCount, ASTNode** params) {
    if (paramCount == 0 || !params) return NULL;
    LLVMTypeRef* types = (LLVMTypeRef*)malloc(sizeof(LLVMTypeRef) * paramCount);
    if (!types) return NULL;

    LLVMContextRef llvmCtx = cg_context_get_llvm_context(ctx);
    for (size_t i = 0; i < paramCount; ++i) {
        LLVMTypeRef inferred = NULL;
        ASTNode* paramDecl = params[i];
        if (paramDecl && paramDecl->type == AST_VARIABLE_DECLARATION) {
            inferred = cg_type_from_parsed(ctx, &paramDecl->varDecl.declaredType);
        }
        if (!inferred || LLVMGetTypeKind(inferred) == LLVMVoidTypeKind) {
            inferred = LLVMInt32TypeInContext(llvmCtx);
        }
        types[i] = inferred;
    }
    return types;
}

static LLVMValueRef ensureFunction(CodegenContext* ctx,
                                   const char* name,
                                   const ParsedType* returnType,
                                   size_t paramCount,
                                   LLVMTypeRef* paramTypes) {
    if (!ctx || !name) return NULL;

    LLVMValueRef existing = LLVMGetNamedFunction(ctx->module, name);
    if (existing) return existing;

    LLVMTypeRef retType = cg_type_from_parsed(ctx, returnType);
    if (!retType) {
        retType = LLVMVoidTypeInContext(ctx->llvmContext);
    }

    LLVMTypeRef funcType = LLVMFunctionType(retType, paramTypes, (unsigned)paramCount, 0);
    return LLVMAddFunction(ctx->module, name, funcType);
}

static void declareFunctionPrototype(CodegenContext* ctx, ASTNode* node) {
    if (!ctx || !node) return;

    const ParsedType* returnType = NULL;
    const char* name = NULL;
    size_t paramCount = 0;
    ASTNode** params = NULL;

    switch (node->type) {
        case AST_FUNCTION_DEFINITION:
            if (!node->functionDef.funcName || node->functionDef.funcName->type != AST_IDENTIFIER) return;
            name = node->functionDef.funcName->valueNode.value;
            returnType = &node->functionDef.returnType;
            paramCount = node->functionDef.paramCount;
            params = node->functionDef.parameters;
            break;
        case AST_FUNCTION_DECLARATION:
            if (!node->functionDecl.funcName || node->functionDecl.funcName->type != AST_IDENTIFIER) return;
            name = node->functionDecl.funcName->valueNode.value;
            returnType = &node->functionDecl.returnType;
            paramCount = node->functionDecl.paramCount;
            params = node->functionDecl.parameters;
            break;
        default:
            return;
    }

    if (!name) return;

    LLVMTypeRef* paramTypes = collectParamTypes(ctx, paramCount, params);
    LLVMValueRef fn = ensureFunction(ctx, name, returnType, paramCount, paramTypes);
    (void)fn;
    free(paramTypes);
}

static void declareGlobalVariableSymbol(CodegenContext* ctx, const Symbol* sym) {
    if (!ctx || !sym || !sym->name) return;

    LLVMTypeRef varType = cg_type_from_parsed(ctx, &sym->type);
    if (!varType || LLVMGetTypeKind(varType) == LLVMVoidTypeKind) {
        varType = LLVMInt32TypeInContext(ctx->llvmContext);
    }

    LLVMValueRef existing = LLVMGetNamedGlobal(ctx->module, sym->name);
    if (!existing) {
        existing = LLVMAddGlobal(ctx->module, varType, sym->name);
        LLVMSetInitializer(existing, LLVMConstNull(varType));
    }

    cg_scope_insert(ctx->currentScope, sym->name, existing, varType, true);
}

static void declareFunctionSymbol(CodegenContext* ctx, const Symbol* sym) {
    if (!ctx || !sym || !sym->name) return;

    size_t paramCount = 0;
    ASTNode** params = NULL;
    if (sym->definition) {
        if (sym->definition->type == AST_FUNCTION_DEFINITION) {
            paramCount = sym->definition->functionDef.paramCount;
            params = sym->definition->functionDef.parameters;
        } else if (sym->definition->type == AST_FUNCTION_DECLARATION) {
            paramCount = sym->definition->functionDecl.paramCount;
            params = sym->definition->functionDecl.parameters;
        }
    }

    LLVMTypeRef* paramTypes = collectParamTypes(ctx, paramCount, params);
    LLVMValueRef fn = ensureFunction(ctx, sym->name, &sym->type, paramCount, paramTypes);
    (void)fn;
    free(paramTypes);
}

static void predeclareGlobalSymbolCallback(const Symbol* sym, void* userData) {
    CodegenContext* ctx = (CodegenContext*)userData;
    if (!ctx || !sym) return;
    fprintf(stderr, "[CG] Predeclare symbol kind=%d name=%s\n", sym->kind, sym->name ? sym->name : "<anon>");
    switch (sym->kind) {
        case SYMBOL_VARIABLE:
            declareGlobalVariableSymbol(ctx, sym);
            break;
        case SYMBOL_FUNCTION:
            declareFunctionSymbol(ctx, sym);
            break;
        case SYMBOL_STRUCT:
            declareStructSymbol(ctx, sym);
            break;
        case SYMBOL_ENUM:
            /* enums currently do not require IR predeclaration */
            break;
        default:
            break;
    }
}

static void declareGlobalVariable(CodegenContext* ctx, ASTNode* node) {
    if (!ctx || !node || node->type != AST_VARIABLE_DECLARATION) return;

    LLVMTypeRef varType = cg_type_from_parsed(ctx, &node->varDecl.declaredType);
    if (!varType || LLVMGetTypeKind(varType) == LLVMVoidTypeKind) {
        varType = LLVMInt32TypeInContext(ctx->llvmContext);
    }

    for (size_t i = 0; i < node->varDecl.varCount; ++i) {
        ASTNode* varNameNode = node->varDecl.varNames[i];
        if (!varNameNode || varNameNode->type != AST_IDENTIFIER) continue;
        const char* name = varNameNode->valueNode.value;
        if (!name) continue;

        LLVMValueRef existing = LLVMGetNamedGlobal(ctx->module, name);
        if (existing) {
            LLVMTypeRef existingType = LLVMGetElementType(LLVMTypeOf(existing));
            if (!existingType || LLVMGetTypeKind(existingType) == LLVMVoidTypeKind) {
                existingType = varType;
            }
            cg_scope_insert(ctx->currentScope, name, existing, existingType, true);
            continue;
        }

        LLVMValueRef global = LLVMAddGlobal(ctx->module, varType, name);
        LLVMSetInitializer(global, LLVMConstNull(varType));
        cg_scope_insert(ctx->currentScope, name, global, varType, true);
    }
}

static void predeclareGlobals(CodegenContext* ctx, ASTNode* program) {
    if (!ctx) return;

    fprintf(stderr, "[CG] Predeclare globals start\n");
    const SemanticModel* model = cg_context_get_semantic_model(ctx);
    if (model) {
        semanticModelForEachGlobal(model, predeclareGlobalSymbolCallback, ctx);

        if (program && program->type == AST_PROGRAM) {
            for (size_t i = 0; i < program->block.statementCount; ++i) {
                ASTNode* stmt = program->block.statements[i];
                if (!stmt) continue;
                if (stmt->type == AST_STRUCT_DEFINITION || stmt->type == AST_UNION_DEFINITION) {
                    (void)codegenStructDefinition(ctx, stmt);
                }
            }
        }
        return;
    }

    if (!program || program->type != AST_PROGRAM) return;

    for (size_t i = 0; i < program->block.statementCount; ++i) {
        ASTNode* stmt = program->block.statements[i];
        if (!stmt) continue;
        switch (stmt->type) {
            case AST_VARIABLE_DECLARATION:
                declareGlobalVariable(ctx, stmt);
                break;
            case AST_FUNCTION_DECLARATION:
            case AST_FUNCTION_DEFINITION:
                declareFunctionPrototype(ctx, stmt);
                break;
            case AST_STRUCT_DEFINITION:
            case AST_UNION_DEFINITION:
                (void)codegenStructDefinition(ctx, stmt);
                break;
            default:
                break;
        }
    }
}

static LLVMTypeRef ensureStructLLVMTypeByName(CodegenContext* ctx, const char* name, bool isUnionHint) {
    if (!ctx || !name || name[0] == '\0') return NULL;
    ParsedType temp;
    memset(&temp, 0, sizeof(temp));
    temp.kind = isUnionHint ? TYPE_UNION : TYPE_STRUCT;
    temp.userTypeName = (char*)name;
    return cg_type_from_parsed(ctx, &temp);
}

static void declareStructSymbol(CodegenContext* ctx, const Symbol* sym) {
    if (!ctx || !sym) return;
    bool isUnion = false;
    if (sym->definition &&
        (sym->definition->type == AST_STRUCT_DEFINITION || sym->definition->type == AST_UNION_DEFINITION)) {
        isUnion = (sym->definition->type == AST_UNION_DEFINITION);
        codegenStructDefinition(ctx, sym->definition);
    } else if (sym->name) {
        CGTypeCache* cache = cg_context_get_type_cache(ctx);
        if (cache) {
            cg_type_cache_lookup_struct(cache, sym->name, &isUnion);
        }
        ensureStructLLVMTypeByName(ctx, sym->name, isUnion);
    }
}

LLVMContextRef cg_context_get_llvm_context(CodegenContext* ctx) {
    return ctx ? ctx->llvmContext : NULL;
}

LLVMModuleRef cg_context_get_module(CodegenContext* ctx) {
    return ctx ? ctx->module : NULL;
}

LLVMBuilderRef cg_context_get_builder(CodegenContext* ctx) {
    return ctx ? ctx->builder : NULL;
}

const SemanticModel* cg_context_get_semantic_model(CodegenContext* ctx) {
    return ctx ? ctx->semanticModel : NULL;
}

CGTypeCache* cg_context_get_type_cache(CodegenContext* ctx) {
    return ctx ? ctx->typeCache : NULL;
}

LLVMTypeRef cg_context_lookup_named_type(CodegenContext* ctx, const char* name) {
    if (!ctx || !name) return NULL;
    for (size_t i = 0; i < ctx->namedTypeCount; ++i) {
        if (ctx->namedTypes[i].name && strcmp(ctx->namedTypes[i].name, name) == 0) {
            return ctx->namedTypes[i].type;
        }
    }
    return NULL;
}

void cg_context_cache_named_type(CodegenContext* ctx, const char* name, LLVMTypeRef type) {
    if (!ctx || !name || !type) return;

    for (size_t i = 0; i < ctx->namedTypeCount; ++i) {
        if (ctx->namedTypes[i].name && strcmp(ctx->namedTypes[i].name, name) == 0) {
            ctx->namedTypes[i].type = type;
            return;
        }
    }

    if (ctx->namedTypeCount == ctx->namedTypeCapacity) {
        size_t newCap = ctx->namedTypeCapacity ? ctx->namedTypeCapacity * 2 : 4;
        NamedType* resized = (NamedType*)realloc(ctx->namedTypes, newCap * sizeof(NamedType));
        if (!resized) return;
        ctx->namedTypes = resized;
        ctx->namedTypeCapacity = newCap;
    }

    ctx->namedTypes[ctx->namedTypeCount].name = strdup(name);
    ctx->namedTypes[ctx->namedTypeCount].type = type;
    ctx->namedTypeCount += 1;
}

static void recordStructInfo(CodegenContext* ctx,
                             const char* name,
                             bool isUnion,
                             StructFieldInfo* fields,
                             size_t fieldCount,
                             LLVMTypeRef llvmType) {
    if (!ctx || !name) {
        if (fields) free(fields);
        return;
    }

    for (size_t i = 0; i < ctx->structInfoCount; ++i) {
        if (ctx->structInfos[i].name && strcmp(ctx->structInfos[i].name, name) == 0) {
            for (size_t j = 0; j < ctx->structInfos[i].fieldCount; ++j) {
                free(ctx->structInfos[i].fields[j].name);
            }
            free(ctx->structInfos[i].fields);
            ctx->structInfos[i].fields = fields;
            ctx->structInfos[i].fieldCount = fieldCount;
            ctx->structInfos[i].isUnion = isUnion;
            ctx->structInfos[i].llvmType = llvmType;
            return;
        }
    }

    if (ctx->structInfoCount == ctx->structInfoCapacity) {
        size_t newCap = ctx->structInfoCapacity ? ctx->structInfoCapacity * 2 : 4;
        StructInfo* resized = (StructInfo*)realloc(ctx->structInfos, newCap * sizeof(StructInfo));
        if (!resized) {
            for (size_t j = 0; j < fieldCount; ++j) {
                free(fields[j].name);
            }
            free(fields);
            return;
        }
        ctx->structInfos = resized;
        ctx->structInfoCapacity = newCap;
    }

    ctx->structInfos[ctx->structInfoCount].name = strdup(name);
    fprintf(stderr, "DEBUG: recordStructInfo name=%s fieldCount=%zu isUnion=%d\n", name, fieldCount, isUnion);
    ctx->structInfos[ctx->structInfoCount].fields = fields;
    ctx->structInfos[ctx->structInfoCount].fieldCount = fieldCount;
    ctx->structInfos[ctx->structInfoCount].isUnion = isUnion;
    ctx->structInfos[ctx->structInfoCount].llvmType = llvmType;
    ctx->structInfoCount += 1;
}

static const StructInfo* lookupStructInfo(CodegenContext* ctx, const char* name, LLVMTypeRef llvmType) {
    if (!ctx) return NULL;
    for (size_t i = 0; i < ctx->structInfoCount; ++i) {
        if (llvmType && ctx->structInfos[i].llvmType && ctx->structInfos[i].llvmType == llvmType) {
            return &ctx->structInfos[i];
        }
        if (name && ctx->structInfos[i].name && strcmp(ctx->structInfos[i].name, name) == 0) {
            return &ctx->structInfos[i];
        }
    }
    return NULL;
}

static LLVMValueRef buildArrayElementPointer(CodegenContext* ctx, LLVMValueRef arrayPtr, LLVMValueRef index, LLVMTypeRef elementOverride) {
    if (!arrayPtr || !index) return NULL;
    LLVMTypeRef ptrType = LLVMTypeOf(arrayPtr);
    if (LLVMGetTypeKind(ptrType) != LLVMPointerTypeKind) {
        return NULL;
    }

    LLVMTypeRef baseType = LLVMGetElementType(ptrType);
    if (LLVMGetTypeKind(baseType) == LLVMArrayTypeKind) {
        LLVMValueRef zero = LLVMConstInt(LLVMInt32TypeInContext(ctx->llvmContext), 0, 0);
        LLVMValueRef indices[2] = { zero, index };
        return LLVMBuildGEP2(ctx->builder, baseType, arrayPtr, indices, 2, "arrayElemPtr");
    }

    LLVMTypeRef elementType = elementOverride ? elementOverride : baseType;
    LLVMValueRef indices[1] = { index };
    return LLVMBuildGEP2(ctx->builder, elementType, arrayPtr, indices, 1, "arrayElemPtr");
}

static LLVMValueRef buildStructFieldPointer(CodegenContext* ctx,
                                            LLVMValueRef basePtr,
                                            LLVMTypeRef aggregateTypeHint,
                                            const char* structNameHint,
                                            const char* fieldName,
                                            LLVMTypeRef* outFieldType) {
    if (!basePtr || !fieldName) return NULL;

    fprintf(stderr, "[CG] buildStructFieldPointer struct=%s field=%s\n",
            structNameHint ? structNameHint : (basePtr ? "<unknown>" : "<null>"), fieldName);

    if (outFieldType) {
        *outFieldType = NULL;
    }

    LLVMTypeRef ptrType = LLVMTypeOf(basePtr);
    if (LLVMGetTypeKind(ptrType) != LLVMPointerTypeKind) {
        return NULL;
    }

    LLVMTypeRef aggregateType = aggregateTypeHint;
    if (!aggregateType) {
        aggregateType = LLVMGetElementType(ptrType);
    }
    if (LLVMGetTypeKind(aggregateType) != LLVMStructTypeKind) {
        return NULL;
    }

    const char* structName = structNameHint ? structNameHint : LLVMGetStructName(aggregateType);
    unsigned fieldIndex = 0;
    LLVMTypeRef fieldLLVMType = NULL;
    bool found = false;

    CGTypeCache* cache = cg_context_get_type_cache(ctx);
    bool isUnion = false;
    if (cache && structName) {
        CGStructLLVMInfo* info = cg_type_cache_get_struct_info(cache, structName);
        if (info) {
            isUnion = info->isUnion;
            ensureStructLLVMTypeByName(ctx, structName, isUnion);
            for (size_t i = 0; i < info->fieldCount; ++i) {
                if (info->fields[i].name && strcmp(info->fields[i].name, fieldName) == 0) {
                    fieldIndex = info->isUnion ? 0 : info->fields[i].index;
                    fieldLLVMType = cg_type_from_parsed(ctx, &info->fields[i].parsedType);
                    found = true;
                    break;
                }
            }
        }
    }

    if (!found) {
        const StructInfo* legacy = lookupStructInfo(ctx, structName, aggregateType);
        if (legacy) {
            for (size_t i = 0; i < legacy->fieldCount; ++i) {
                if (legacy->fields[i].name && strcmp(legacy->fields[i].name, fieldName) == 0) {
                    fieldIndex = legacy->isUnion ? 0 : legacy->fields[i].index;
                    fieldLLVMType = legacy->fields[i].type;
                    found = true;
                    break;
                }
            }
        }
    }

    if (!found) {
        fprintf(stderr,
                "DEBUG: struct lookup failed for %s.%s (structInfos=%zu)\n",
                structName ? structName : "<anon>",
                fieldName,
                ctx->structInfoCount);
        return NULL;
    }

    if (!fieldLLVMType || LLVMGetTypeKind(fieldLLVMType) == LLVMVoidTypeKind) {
        fieldLLVMType = LLVMInt32TypeInContext(ctx->llvmContext);
    }

    if (outFieldType) {
        *outFieldType = fieldLLVMType;
    }

    return LLVMBuildStructGEP2(ctx->builder, aggregateType, basePtr, fieldIndex, "fieldPtr");
}

static bool codegenLValue(CodegenContext* ctx, ASTNode* target, LLVMValueRef* outPtr, LLVMTypeRef* outType) {
    if (!ctx || !target || !outPtr || !outType) return false;

    switch (target->type) {
        case AST_IDENTIFIER: {
            fprintf(stderr, "[CG] LValue identifier %s\n", target->valueNode.value);
            const char* name = target->valueNode.value;
            NamedValue* entry = cg_scope_lookup(ctx->currentScope, name);
            if (!entry) {
                const SemanticModel* model = cg_context_get_semantic_model(ctx);
                if (model) {
                    const Symbol* sym = semanticModelLookupGlobal(model, name);
                    if (sym) {
                        if (sym->kind == SYMBOL_VARIABLE) {
                            declareGlobalVariableSymbol(ctx, sym);
                        } else if (sym->kind == SYMBOL_FUNCTION) {
                            declareFunctionSymbol(ctx, sym);
                        }
                        entry = cg_scope_lookup(ctx->currentScope, name);
                    }
                }
            }
            if (!entry) {
                fprintf(stderr, "Error: assignment target '%s' not declared\n", name ? name : "<anonymous>");
                return false;
            }
            *outPtr = entry->value;
            *outType = entry->type ? entry->type : LLVMGetElementType(LLVMTypeOf(entry->value));
            return true;
        }

        case AST_ARRAY_ACCESS: {
            fprintf(stderr, "[CG] LValue array access\n");
            LLVMValueRef arrayPtr = codegenNode(ctx, target->arrayAccess.array);
            LLVMValueRef index = codegenNode(ctx, target->arrayAccess.index);
            LLVMValueRef elementPtr = buildArrayElementPointer(ctx, arrayPtr, index, NULL);
            if (!elementPtr) return false;
            *outPtr = elementPtr;
            *outType = LLVMGetElementType(LLVMTypeOf(elementPtr));
            return true;
        }

        case AST_POINTER_DEREFERENCE: {
            fprintf(stderr, "[CG] LValue pointer deref\n");
            LLVMValueRef pointerValue = codegenNode(ctx, target->pointerDeref.pointer);
            if (!pointerValue) return false;
            *outPtr = pointerValue;
            *outType = LLVMGetElementType(LLVMTypeOf(pointerValue));
            return true;
        }

        case AST_DOT_ACCESS: {
            fprintf(stderr, "[CG] LValue dot access\n");
            LLVMValueRef basePtr = NULL;
            LLVMTypeRef baseType = NULL;
            if (!codegenLValue(ctx, target->memberAccess.base, &basePtr, &baseType)) {
                LLVMValueRef baseVal = codegenNode(ctx, target->memberAccess.base);
                if (!baseVal) return false;
                if (LLVMGetTypeKind(LLVMTypeOf(baseVal)) == LLVMPointerTypeKind) {
                    basePtr = baseVal;
                    baseType = LLVMGetElementType(LLVMTypeOf(baseVal));
                } else {
                    LLVMValueRef tmpAlloca = LLVMBuildAlloca(ctx->builder, LLVMTypeOf(baseVal), "dot_tmp_lhs");
                    LLVMBuildStore(ctx->builder, baseVal, tmpAlloca);
                    basePtr = tmpAlloca;
                    baseType = LLVMTypeOf(baseVal);
                }
            }

            const char* nameHint = NULL;
            if (baseType && LLVMGetTypeKind(baseType) == LLVMStructTypeKind) {
                nameHint = LLVMGetStructName(baseType);
            }

            LLVMTypeRef fieldType = NULL;
            LLVMValueRef fieldPtr = buildStructFieldPointer(ctx, basePtr, baseType, nameHint, target->memberAccess.field, &fieldType);
            if (!fieldPtr) return false;
            *outPtr = fieldPtr;
            if (!fieldType || LLVMGetTypeKind(fieldType) == LLVMVoidTypeKind) {
                fieldType = LLVMInt32TypeInContext(ctx->llvmContext);
            }
            *outType = fieldType;
            return true;
        }

        case AST_POINTER_ACCESS: {
            fprintf(stderr, "[CG] LValue pointer access\n");
            LLVMValueRef basePtr = codegenNode(ctx, target->memberAccess.base);
            if (!basePtr) return false;
            LLVMTypeRef basePtrType = LLVMTypeOf(basePtr);
            LLVMTypeRef aggregateType = NULL;
            const char* nameHint = NULL;
            if (LLVMGetTypeKind(basePtrType) == LLVMPointerTypeKind) {
                aggregateType = LLVMGetElementType(basePtrType);
                if (aggregateType && LLVMGetTypeKind(aggregateType) == LLVMStructTypeKind) {
                    nameHint = LLVMGetStructName(aggregateType);
                }
            }
            LLVMTypeRef fieldType = NULL;
            LLVMValueRef fieldPtr = buildStructFieldPointer(ctx, basePtr, aggregateType, nameHint, target->memberAccess.field, &fieldType);
            if (!fieldPtr) return false;
            *outPtr = fieldPtr;
            if (!fieldType || LLVMGetTypeKind(fieldType) == LLVMVoidTypeKind) {
                fieldType = LLVMInt32TypeInContext(ctx->llvmContext);
            }
            *outType = fieldType;
            return true;
        }

        default:
            break;
    }

    return false;
}
