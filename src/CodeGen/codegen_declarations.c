#include "codegen_private.h"

#include "codegen_types.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

LLVMTypeRef* collectParamTypes(CodegenContext* ctx, size_t paramCount, ASTNode** params) {
    if (paramCount == 0) {
        return NULL;
    }

    fprintf(stderr, "collectParamTypes count=%zu\n", paramCount);

    LLVMTypeRef* paramTypes = (LLVMTypeRef*)calloc(paramCount, sizeof(LLVMTypeRef));
    if (!paramTypes) {
        return NULL;
    }

    for (size_t i = 0; i < paramCount; ++i) {
        ASTNode* param = params ? params[i] : NULL;
        const ParsedType* type = NULL;
        if (param && param->type == AST_VARIABLE_DECLARATION) {
            type = astVarDeclTypeAt(param, 0);
        }
        if (!type) {
            paramTypes[i] = LLVMInt32TypeInContext(ctx->llvmContext);
        } else {
            paramTypes[i] = cg_type_from_parsed(ctx, type);
            if (!paramTypes[i] || LLVMGetTypeKind(paramTypes[i]) == LLVMVoidTypeKind) {
                paramTypes[i] = LLVMInt32TypeInContext(ctx->llvmContext);
            }
        }
    }

    return paramTypes;
}

LLVMValueRef ensureFunction(CodegenContext* ctx,
                            const char* name,
                            const ParsedType* returnType,
                            size_t paramCount,
                            LLVMTypeRef* paramTypes) {
    if (!ctx || !name) return NULL;

    LLVMTypeRef returnLLVM = returnType ? cg_type_from_parsed(ctx, returnType) : LLVMVoidTypeInContext(ctx->llvmContext);
    if (!returnLLVM || LLVMGetTypeKind(returnLLVM) == LLVMVoidTypeKind) {
        returnLLVM = LLVMVoidTypeInContext(ctx->llvmContext);
    }

    LLVMTypeRef fnType = LLVMFunctionType(returnLLVM, paramTypes, (unsigned)paramCount, 0);
    LLVMValueRef existing = LLVMGetNamedFunction(ctx->module, name);
    if (existing) {
        return existing;
    }
    LLVMValueRef fn = LLVMAddFunction(ctx->module, name, fnType);
    if (ctx->verifyFunctions) {
        char* error = NULL;
        LLVMVerifyFunction(fn, LLVMAbortProcessAction);
        if (error) {
            LLVMDisposeMessage(error);
        }
    }
    return fn;
}

void declareFunctionPrototype(CodegenContext* ctx, ASTNode* node) {
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

void declareGlobalVariableSymbol(CodegenContext* ctx, const Symbol* sym) {
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

    cg_scope_insert(ctx->currentScope, sym->name, existing, varType, true, false, NULL, &sym->type);
}

void declareFunctionSymbol(CodegenContext* ctx, const Symbol* sym) {
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
            break;
        default:
            break;
    }
}

void declareGlobalVariable(CodegenContext* ctx, ASTNode* node) {
    if (!ctx || !node || node->type != AST_VARIABLE_DECLARATION) return;

    for (size_t i = 0; i < node->varDecl.varCount; ++i) {
        ASTNode* varNameNode = node->varDecl.varNames[i];
        if (!varNameNode || varNameNode->type != AST_IDENTIFIER) continue;
        const char* name = varNameNode->valueNode.value;
        if (!name) continue;
        const ParsedType* parsedType = astVarDeclTypeAt(node, i);
        LLVMTypeRef varType = cg_type_from_parsed(ctx, parsedType);
        if (!varType || LLVMGetTypeKind(varType) == LLVMVoidTypeKind) {
            varType = LLVMInt32TypeInContext(ctx->llvmContext);
        }

        LLVMValueRef existing = LLVMGetNamedGlobal(ctx->module, name);
        if (existing) {
            LLVMTypeRef existingType = LLVMGetElementType(LLVMTypeOf(existing));
            if (!existingType || LLVMGetTypeKind(existingType) == LLVMVoidTypeKind) {
                existingType = varType;
            }
            cg_scope_insert(ctx->currentScope,
                            name,
                            existing,
                            existingType,
                            true,
                            false,
                            NULL,
                            parsedType ? parsedType : &node->varDecl.declaredType);
            continue;
        }

        LLVMValueRef global = LLVMAddGlobal(ctx->module, varType, name);
        LLVMSetInitializer(global, LLVMConstNull(varType));
        cg_scope_insert(ctx->currentScope,
                        name,
                        global,
                        varType,
                        true,
                        false,
                        NULL,
                        parsedType ? parsedType : &node->varDecl.declaredType);
    }
}

void predeclareGlobals(CodegenContext* ctx, ASTNode* program) {
    if (!ctx) return;

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
