#ifndef CODEGEN_PRIVATE_H
#define CODEGEN_PRIVATE_H

#include "codegen_internal.h"

#include "AST/ast_node.h"
#include "Parser/Helpers/designated_init.h"
#include "Parser/Helpers/parsed_type.h"
#include "Syntax/semantic_model.h"

#include <llvm-c/Core.h>

#include <stdbool.h>
#include <stddef.h>

#include "Utils/logging.h"

#ifdef CODEGEN_DEBUG
#define CG_DEBUG(...) LOG_DEBUG("codegen", __VA_ARGS__)
#else
#define CG_DEBUG(...) ((void)0)
#endif

#ifdef CODEGEN_TRACE
#define CG_TRACE(...) LOG_TRACE("codegen", __VA_ARGS__)
#else
#define CG_TRACE(...) ((void)0)
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NamedValue {
    char* name;
    LLVMValueRef value;
    LLVMTypeRef type;
    LLVMTypeRef elementType;
    const ParsedType* parsedType;
    bool isGlobal;
    bool addressOnly;
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
    ParsedType parsedType;
} StructFieldInfo;

typedef struct StructInfo {
    char* name;
    StructFieldInfo* fields;
    size_t fieldCount;
    bool isUnion;
    LLVMTypeRef llvmType;
} StructInfo;

typedef struct LabelBinding {
    char* name;
    LLVMBasicBlockRef block;
    bool defined;
} LabelBinding;

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

    LabelBinding* labels;
    size_t labelCount;
    size_t labelCapacity;

    const SemanticModel* semanticModel;
    CGTypeCache* typeCache;
    const ParsedType* currentFunctionReturnType;
    bool verifyFunctions;
};

typedef enum {
    CG_VALUE_UNKNOWN = 0,
    CG_VALUE_BOOL,
    CG_VALUE_SIGNED_INT,
    CG_VALUE_UNSIGNED_INT,
    CG_VALUE_FLOAT,
    CG_VALUE_POINTER,
    CG_VALUE_AGGREGATE
} CGValueCategory;

CGScope* cg_scope_push(CodegenContext* ctx);
void cg_scope_pop(CodegenContext* ctx);
void cg_scope_insert(CGScope* scope,
                     const char* name,
                     LLVMValueRef value,
                     LLVMTypeRef type,
                     bool isGlobal,
                     bool addressOnly,
                     LLVMTypeRef elementType,
                     const ParsedType* parsedType);
NamedValue* cg_scope_lookup(CGScope* scope, const char* name);

void cg_loop_push(CodegenContext* ctx, LLVMBasicBlockRef breakBB, LLVMBasicBlockRef continueBB);
void cg_loop_pop(CodegenContext* ctx);
LoopTarget cg_loop_peek(const CodegenContext* ctx);
LoopTarget cg_loop_peek_for_continue(const CodegenContext* ctx);

LabelBinding* cg_lookup_label(CodegenContext* ctx, const char* name);
LabelBinding* cg_ensure_label(CodegenContext* ctx, const char* name);
LLVMBasicBlockRef cg_label_ensure_block(CodegenContext* ctx, LabelBinding* binding, LLVMValueRef func);

LLVMTypeRef cg_get_intptr_type(CodegenContext* ctx);
LLVMValueRef cg_widen_bool_to_int(CodegenContext* ctx, LLVMValueRef value, const char* nameHint);
LLVMValueRef cg_build_pointer_offset(CodegenContext* ctx,
                                     LLVMValueRef basePtr,
                                     LLVMValueRef offsetValue,
                                     const ParsedType* pointerParsed,
                                     const ParsedType* offsetParsed,
                                     bool isSubtract);
LLVMValueRef cg_build_pointer_difference(CodegenContext* ctx,
                                         LLVMValueRef lhsPtr,
                                         LLVMValueRef rhsPtr,
                                         const ParsedType* lhsParsed);
const ParsedType* cg_resolve_expression_type(CodegenContext* ctx, ASTNode* node);
bool cg_expression_is_unsigned(CodegenContext* ctx, ASTNode* node);
LLVMIntPredicate cg_select_int_predicate(const char* op, bool preferUnsigned);

LLVMValueRef cg_build_truthy(CodegenContext* ctx,
                             LLVMValueRef value,
                             const ParsedType* parsedType,
                             const char* nameHint);
LLVMTypeRef cg_element_type_from_pointer(CodegenContext* ctx,
                                         const ParsedType* pointerParsed,
                                         LLVMTypeRef pointerLLVM);
LLVMValueRef cg_cast_value(CodegenContext* ctx,
                           LLVMValueRef value,
                           LLVMTypeRef targetType,
                           const ParsedType* fromParsed,
                           const ParsedType* toParsed,
                           const char* nameHint);

bool cg_store_initializer_expression(CodegenContext* ctx,
                                     LLVMValueRef destPtr,
                                     LLVMTypeRef destType,
                                     const ParsedType* destParsed,
                                     ASTNode* expr);
bool cg_store_designated_entries(CodegenContext* ctx,
                                 LLVMValueRef destPtr,
                                 LLVMTypeRef destType,
                                 const ParsedType* destParsed,
                                 DesignatedInit** entries,
                                 size_t entryCount);
bool cg_store_compound_literal_into_ptr(CodegenContext* ctx,
                                        LLVMValueRef destPtr,
                                        LLVMTypeRef destType,
                                        const ParsedType* destParsed,
                                        ASTNode* literalNode);

bool codegenLValue(CodegenContext* ctx,
                   ASTNode* target,
                   LLVMValueRef* outPtr,
                   LLVMTypeRef* outType,
                   const ParsedType** outParsedType);
LLVMValueRef buildStructFieldPointer(CodegenContext* ctx,
                                     LLVMValueRef basePtr,
                                     LLVMTypeRef aggregateTypeHint,
                                     const char* structName,
                                     const char* fieldName,
                                     LLVMTypeRef* outFieldType,
                                     const ParsedType** outFieldParsedType);
LLVMValueRef buildArrayElementPointer(CodegenContext* ctx,
                                      LLVMValueRef arrayPtr,
                                      LLVMValueRef index,
                                      LLVMTypeRef aggregateTypeHint,
                                      LLVMTypeRef elementTypeHint,
                                      LLVMTypeRef* outElementType);
LLVMTypeRef ensureStructLLVMTypeByName(CodegenContext* ctx, const char* name, bool isUnionHint);
void declareStructSymbol(CodegenContext* ctx, const Symbol* sym);

LLVMTypeRef* collectParamTypes(CodegenContext* ctx, size_t paramCount, ASTNode** params);
LLVMValueRef ensureFunction(CodegenContext* ctx,
                            const char* name,
                            const ParsedType* returnType,
                            size_t paramCount,
                            LLVMTypeRef* paramTypes);
void declareFunctionPrototype(CodegenContext* ctx, ASTNode* node);
void declareGlobalVariable(CodegenContext* ctx, ASTNode* node);
void predeclareGlobals(CodegenContext* ctx, ASTNode* program);
void declareGlobalVariableSymbol(CodegenContext* ctx, const Symbol* sym);
void declareFunctionSymbol(CodegenContext* ctx, const Symbol* sym);

LLVMValueRef codegenNode(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenProgram(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenBlock(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenBinaryExpression(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenUnaryExpression(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenTernaryExpression(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenAssignment(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenVariableDeclaration(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenArrayDeclaration(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenArrayAccess(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenPointerAccess(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenDotAccess(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenPointerDereference(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenIfStatement(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenWhileLoop(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenForLoop(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenLogicalAndOr(CodegenContext* ctx, LLVMValueRef lhsValue, ASTNode* rhsNode, bool isAnd);
LLVMValueRef codegenFunctionDefinition(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenFunctionCall(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenTypedef(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenEnumDefinition(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenCastExpression(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenCompoundLiteral(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenReturn(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenBreak(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenContinue(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenNumberLiteral(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenCharLiteral(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenStringLiteral(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenIdentifier(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenSizeof(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenStatementExpression(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenSwitch(CodegenContext* ctx, ASTNode* node);
LLVMTypeRef codegenStructDefinition(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenStructFieldAccess(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenHeapAllocation(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenLabel(CodegenContext* ctx, ASTNode* node);
LLVMValueRef codegenGoto(CodegenContext* ctx, ASTNode* node);

#ifdef __cplusplus
}
#endif

#endif /* CODEGEN_PRIVATE_H */
