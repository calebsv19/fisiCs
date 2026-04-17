// SPDX-License-Identifier: Apache-2.0

#ifndef ANALYZE_EXPR_INTERNAL_H
#define ANALYZE_EXPR_INTERNAL_H

#include "analyze_expr.h"
#include "Compiler/compiler_context.h"

void analyzeExpressionEffects(ASTNode* node, Scope* scope);
char* analyzeExprAccessPath(ASTNode* node);

const ParsedType* analyzeExprLookupFieldType(const TypeInfo* base,
                                             const char* fieldName,
                                             Scope* scope);
bool analyzeExprLookupFieldIsBitfield(const TypeInfo* base,
                                      const char* fieldName,
                                      Scope* scope);
const CCTagFieldLayout* analyzeExprLookupFieldLayout(const TypeInfo* base,
                                                     const char* fieldName,
                                                     Scope* scope);

TypeInfo analyzeFunctionCallExpression(ASTNode* node, Scope* scope);
TypeInfo analyzePointerDereferenceExpression(ASTNode* node, Scope* scope);
TypeInfo analyzeMemberAccessExpression(ASTNode* node, Scope* scope);
TypeInfo analyzeArrayAccessExpression(ASTNode* node, Scope* scope);
TypeInfo analyzeCompoundLiteralExpression(ASTNode* node, Scope* scope);
TypeInfo analyzeStatementExpressionValue(ASTNode* node, Scope* scope);
TypeInfo analyzeTernaryExpression(ASTNode* node, Scope* scope);

void reportOperandError(ASTNode* node, const char* expectation, const char* op);
void reportNodeError(ASTNode* node, const char* message, const char* hint);
bool typeInfoIsKnown(const TypeInfo* info);
bool isNullPointerConstant(ASTNode* expr, Scope* scope);
void restoreBaseCategory(TypeInfo* info);
bool isExpressionNodeType(ASTNodeType type);
bool parsedTypeTopLevelConst(const ParsedType* type);
bool parsedTypePrependPointer(ParsedType* t);
void typeInfoAdoptParsedType(TypeInfo* info, ParsedType* ownedParsed);

#endif // ANALYZE_EXPR_INTERNAL_H
