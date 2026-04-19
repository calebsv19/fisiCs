// SPDX-License-Identifier: Apache-2.0

#ifndef TYPE_CHECKER_H
#define TYPE_CHECKER_H

#include "Parser/Helpers/parsed_type.h"
#include <stdbool.h>

struct Scope;
struct CCTagFieldLayout;
struct Symbol;

typedef enum {
    TYPEINFO_INVALID = 0,
    TYPEINFO_INTEGER,
    TYPEINFO_FLOAT,
    TYPEINFO_POINTER,
    TYPEINFO_ARRAY,
    TYPEINFO_FUNCTION,
    TYPEINFO_STRUCT,
    TYPEINFO_UNION,
    TYPEINFO_ENUM,
    TYPEINFO_VOID
} TypeInfoCategory;

typedef struct {
    bool isConst;
    bool isVolatile;
    bool isRestrict;
} PointerQualifier;

#define TYPEINFO_MAX_POINTER_DEPTH 32

typedef enum {
    FLOAT_KIND_FLOAT = 0,
    FLOAT_KIND_DOUBLE,
    FLOAT_KIND_LONG_DOUBLE
} FloatKind;

typedef struct TypeInfo {
    TypeInfoCategory category;
    TokenType primitive;
    unsigned bitWidth;
    bool isSigned;
    bool isConst;
    bool isVolatile;
    bool isRestrict;
    bool isArray;
    bool isFunction;
    bool isVLA;
    bool isComplex;
    bool isImaginary;
    TagKind tag;
    const char* userTypeName;
    int pointerDepth;
    PointerQualifier pointerLevels[TYPEINFO_MAX_POINTER_DEPTH];
    bool isLValue;
    bool isComplete;
    const ParsedType* originalType;
    bool isBitfield;
    const struct CCTagFieldLayout* bitfieldLayout;
} TypeInfo;

typedef enum {
    ASSIGN_OK = 0,
    ASSIGN_INCOMPATIBLE,
    ASSIGN_QUALIFIER_LOSS
} AssignmentCheckResult;

TypeInfo makeInvalidType(void);
TypeInfo makeBoolType(void);
TypeInfo makeIntegerType(unsigned bitWidth, bool isSigned, TokenType primitive);
TypeInfo makeFloatTypeInfo(FloatKind kind, bool isComplex, struct Scope* scope);
TypeInfo typeInfoFromParsedType(const ParsedType* type, struct Scope* scope);
TypeInfo typeInfoFromSymbolCached(struct Symbol* sym, struct Scope* scope);
void invalidateSymbolTypeInfoCache(struct Symbol* sym);
void primeSymbolTypeInfoCache(struct Symbol* sym, struct Scope* scope);

bool typeInfoIsInteger(const TypeInfo* info);
bool typeInfoIsFloating(const TypeInfo* info);
bool typeInfoIsArithmetic(const TypeInfo* info);
bool typeInfoIsPointerLike(const TypeInfo* info);
TypeInfo integerPromote(TypeInfo info);
TypeInfo usualArithmeticConversion(TypeInfo left, TypeInfo right, bool* ok);
TypeInfo defaultArgumentPromotion(TypeInfo info);
bool typesAreEqual(const TypeInfo* a, const TypeInfo* b);
AssignmentCheckResult canAssignTypesInScope(const TypeInfo* dest, const TypeInfo* src, struct Scope* scope);
AssignmentCheckResult canAssignTypes(const TypeInfo* dest, const TypeInfo* src);
void typeInfoDropPointerLevel(TypeInfo* info);
void typeInfoPrependPointerLevel(TypeInfo* info, PointerQualifier q);

#endif // TYPE_CHECKER_H
