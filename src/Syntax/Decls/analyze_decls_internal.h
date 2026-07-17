// SPDX-License-Identifier: Apache-2.0

#ifndef ANALYZE_DECLS_INTERNAL_H
#define ANALYZE_DECLS_INTERNAL_H

#include "analyze_decls.h"
#include "symbol_table.h"
#include "syntax_errors.h"
#include "scope.h"
#include "Compiler/compiler_context.h"
#include "Parser/Helpers/designated_init.h"
#include "const_eval.h"
#include "analyze_expr.h"
#include "type_checker.h"
#include "literal_utils.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ASTNode* varDeclPrimaryNameNode(ASTNode* node);
SourceRange varDeclBestSpellingRange(ASTNode* node);
SourceRange varDeclBestMacroCallSite(ASTNode* node);
SourceRange varDeclBestMacroDefinition(ASTNode* node);

const ParsedType* resolveTypedefBase(Scope* scope, const ParsedType* type, int depth);
void canonicalizeParsedTypeInScope(ParsedType* type, Scope* scope);
void freezeAggregateAliasesAtDeclaration(ParsedType* type, Scope* scope, int depth);
bool parsedTypesStructurallyCompatibleInScope(const ParsedType* a,
                                             const ParsedType* b,
                                             Scope* scope);
bool typedefChainContainsName(Scope* scope,
                              const ParsedType* type,
                              const char* name,
                              int depth);
bool parsedTypeIsFlexibleArray(const ParsedType* type);
uint64_t fingerprintStructLike(const ASTNode* node);
uint64_t fingerprintEnumDefinition(const ASTNode* node);
ParsedType makeEnumValueParsedType(void);
bool tryEvaluateArrayLength(ASTNode* sizeExpr, Scope* scope, size_t* outLen);
const ParsedType* resolveTypedefInScope(const ParsedType* type, Scope* scope);
void evaluateArrayDerivations(ParsedType* type, Scope* scope);
bool enumExprHasOverflowingIntegerLiteral(ASTNode* expr, Scope* scope);
bool enumValueFitsIntRange(long long value, Scope* scope);

void resetFunctionSignature(Symbol* sym);
bool parsedTypeIsPlainVoid(const ParsedType* type);
bool parsedTypeIsDirectFunction(const ParsedType* type);
bool validateFunctionParameters(ASTNode** params,
                                size_t paramCount,
                                bool isVariadic,
                                Scope* scope,
                                int line,
                                const char* funcName);
void assignFunctionSignature(Symbol* sym,
                             ASTNode** params,
                             size_t paramCount,
                             bool isVariadic,
                             bool hasPrototype,
                             Scope* scope);
void assignFunctionSignatureFromParsedType(Symbol* sym,
                                           const ParsedType* functionType,
                                           Scope* scope);
bool functionSignaturesCompatible(const FunctionSignature* lhs,
                                  const FunctionSignature* rhs,
                                  Scope* scope);
void mergeCompatibleFunctionSignatures(FunctionSignature* accumulated,
                                       FunctionSignature* incoming);
void mergeCompatibleParsedTypeDetailsInScope(ParsedType* accumulated,
                                             const ParsedType* incoming,
                                             Scope* scope);
void freeFunctionSignatureParameters(FunctionSignature* signature);
const char* safeIdentifierName(ASTNode* node);
bool scopeIsFileScope(Scope* scope);
void applyInteropAttributes(Symbol* sym, ASTNode* node, Scope* scope, bool allowWarn);
StorageClass deduceStorageClass(const ParsedType* type);
SymbolLinkage deduceLinkage(const ParsedType* type, bool fileScope);
bool validateStorageUsage(const ParsedType* type,
                          bool fileScope,
                          bool isFunction,
                          bool isTypedef,
                          int line,
                          const char* nameHint);
bool validateRestrictUsage(const ParsedType* type,
                           Scope* scope,
                           int line,
                           const char* nameHint);
bool validatePrimitiveSpecifierUsage(const ParsedType* type,
                                     int line,
                                     const char* nameHint);
void analyzeDesignatedInitializer(DesignatedInit* init, Scope* scope);

void reportErrorAtAstNode(ASTNode* node,
                          int fallbackLine,
                          const char* message,
                          const char* hint);
void validateScalarCompoundLiteral(ASTNode* compound,
                                   ASTNode* context,
                                   const char* name);
void validateAggregateDesignatedFieldsRecursive(const ParsedType* aggregateType,
                                                const TypeInfo* aggregateInfo,
                                                ASTNode* compoundExpr,
                                                const char* variableName,
                                                Scope* scope,
                                                int fallbackLine,
                                                int depth);
void maybeRecordConstIntegerValue(Symbol* sym,
                                  const ParsedType* varType,
                                  DesignatedInit* init,
                                  Scope* scope);
void validateVariableInitializer(ParsedType* type,
                                 DesignatedInit* init,
                                 ASTNode* nameNode,
                                 Scope* scope,
                                 bool staticStorage);
void validateVariableArrayInitializer(ParsedType* type,
                                      DesignatedInit* init,
                                      ASTNode* nameNode,
                                      Scope* scope);
void validateArrayInitializerEntries(ParsedType* type,
                                     const char* arrayName,
                                     DesignatedInit** values,
                                     size_t valueCount,
                                     Scope* scope,
                                     ASTNode* contextNode,
                                     long long* outInferredLength);
void validateBitField(ASTNode* field, ParsedType* fieldType, Scope* scope);

#endif // ANALYZE_DECLS_INTERNAL_H
