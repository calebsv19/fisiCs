#ifndef TYPE_CHECKER_H
#define TYPE_CHECKER_H

#include "Parser/Helpers/parsed_type.h"
#include <stdbool.h>

struct Scope;

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
    TagKind tag;
    const char* userTypeName;
    int pointerDepth;
    bool isLValue;
    const ParsedType* originalType;
} TypeInfo;

typedef enum {
    ASSIGN_OK = 0,
    ASSIGN_INCOMPATIBLE,
    ASSIGN_QUALIFIER_LOSS
} AssignmentCheckResult;

TypeInfo makeInvalidType(void);
TypeInfo makeBoolType(void);
TypeInfo makeIntegerType(unsigned bitWidth, bool isSigned, TokenType primitive);
TypeInfo makeFloatTypeInfo(bool isDouble);
TypeInfo typeInfoFromParsedType(const ParsedType* type, struct Scope* scope);

bool typeInfoIsInteger(const TypeInfo* info);
bool typeInfoIsFloating(const TypeInfo* info);
bool typeInfoIsArithmetic(const TypeInfo* info);
bool typeInfoIsPointerLike(const TypeInfo* info);
TypeInfo integerPromote(TypeInfo info);
TypeInfo usualArithmeticConversion(TypeInfo left, TypeInfo right, bool* ok);
TypeInfo defaultArgumentPromotion(TypeInfo info);
bool typesAreEqual(const TypeInfo* a, const TypeInfo* b);
AssignmentCheckResult canAssignTypes(const TypeInfo* dest, const TypeInfo* src);

#endif // TYPE_CHECKER_H
