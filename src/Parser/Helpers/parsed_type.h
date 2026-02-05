#ifndef PARSED_TYPE_H
#define PARSED_TYPE_H

#include "Lexer/tokens.h"  // Required for TokenType
#include <stdbool.h>
#include <stddef.h>
#include "AST/ast_attribute.h"

struct Parser;
struct ASTNode;

typedef enum {
    TYPE_DERIVATION_POINTER,
    TYPE_DERIVATION_ARRAY,
    TYPE_DERIVATION_FUNCTION
} TypeDerivationKind;

typedef struct ParsedArrayInfo {
    struct ASTNode* sizeExpr;
    bool isVLA;
    bool hasConstantSize;
    long long constantSize;
    bool isFlexible;
    bool hasStatic;
    bool qualConst;
    bool qualVolatile;
    bool qualRestrict;
} ParsedArrayInfo;

struct ParsedType;

typedef struct ParsedFunctionSignature {
    bool isVariadic;
    size_t paramCount;
    struct ParsedType* params;
} ParsedFunctionSignature;

typedef struct TypeDerivation {
    TypeDerivationKind kind;
    union {
        ParsedArrayInfo array;
        ParsedFunctionSignature function;
        struct {
            bool isConst;
            bool isVolatile;
            bool isRestrict;
        } pointer;
    } as;
} TypeDerivation;

// Enum for distinguishing between primitive and user-defined types
typedef enum {
    TYPE_INVALID,
    TYPE_PRIMITIVE,
    TYPE_STRUCT,
    TYPE_UNION,
    TYPE_ENUM,
    TYPE_NAMED,    
} TypeKind;

typedef enum {
    TYPECTX_Strict,          // casts, sizeof(type), compound literals
    TYPECTX_Declaration      // variable/parameter/field declarations
} TypeContext;


// Track if it's struct/union or just a typedef name
typedef enum { TAG_NONE, TAG_STRUCT, TAG_UNION, TAG_ENUM } TagKind;

// Struct representing a parsed type
typedef struct ParsedType {
    TypeKind kind;
    TagKind tag;

    // Optional inline aggregate definition captured in a declaration specifier
    struct ASTNode* inlineStructOrUnionDef;
    struct ASTNode* inlineEnumDef;

    bool isFunctionPointer;
    size_t fpParamCount;
    struct ParsedType* fpParams;  // array of parameter types (own it)

    // If primitive (e.g. int, float), use this
    TokenType primitiveType;

    // If user-defined (e.g. ASTNode), use this
    char* userTypeName;

    // Modifiers
    bool isConst;
    bool isSigned;
    bool isUnsigned;
    bool isShort;
    bool isLong;
    bool isComplex;
    bool isImaginary;

    // Qualifiers (C99)
    bool isVolatile;   //  volatile int x;
    bool isRestrict;   //  restrict char* p;
    bool isInline;     //  inline void foo();

    // Storage Specifiers
    bool isStatic;
    bool isExtern;
    bool isRegister;
    bool isAuto;

    int pointerDepth;
    bool isVLA;
    bool directlyDeclaresFunction;
    bool isVariadicFunction;

    TypeDerivation* derivations;
    size_t derivationCount;

    ASTAttribute** attributes;
    size_t attributeCount;

    /* Optional explicit alignment from _Alignas/alignas */
    size_t alignOverride;
    bool hasAlignOverride;

    /* Array parameter metadata (C99) */
    bool hasParamArrayInfo;
    ParsedArrayInfo paramArrayInfo;
} ParsedType;


// Utility to convert TokenType to string
const char* getTokenTypeName(TokenType type);
void parsedTypeAddPointerDepth(ParsedType* t, int depth);

// Optional tiny helpers
void parsedTypeSetFunctionPointer(ParsedType* t, size_t nParams, const ParsedType* params);
void parsedTypeFree(ParsedType* t);  // free fpParams if set (call in your global cleanup)

bool parsedTypeAppendPointer(ParsedType* t);
bool parsedTypeAppendArray(ParsedType* t, struct ASTNode* sizeExpr, bool isVLA);
bool parsedTypeAppendFunction(ParsedType* t, const ParsedType* params, size_t paramCount, bool isVariadic);
void parsedTypeResetDerivations(ParsedType* t);
ParsedType parsedTypeClone(const ParsedType* src);
void parsedTypeAdoptAttributes(ParsedType* t, ASTAttribute** attrs, size_t count);
bool parsedTypeIsDirectArray(const ParsedType* t);
bool parsedTypeAdjustArrayParameter(ParsedType* t);
ParsedType parsedTypeArrayElementType(const ParsedType* t);
ParsedType parsedTypePointerTargetType(const ParsedType* t);
ParsedType parsedTypeFunctionReturnType(const ParsedType* t);
void parsedTypeNormalizeFunctionPointer(ParsedType* t);
bool parsedTypesStructurallyEqual(const ParsedType* a, const ParsedType* b);
const TypeDerivation* parsedTypeGetDerivation(const ParsedType* t, size_t index);
const TypeDerivation* parsedTypeGetArrayDerivation(const ParsedType* t, size_t dimensionIndex);
TypeDerivation* parsedTypeGetMutableArrayDerivation(ParsedType* t, size_t dimensionIndex);
size_t parsedTypeCountDerivationsOfKind(const ParsedType* t, TypeDerivationKind kind);
bool parsedTypeHasVLA(const ParsedType* t);

ParsedType parseType(struct Parser* parser);
ParsedType parseTypeCtx(struct Parser* parser, TypeContext ctx);

/* Default initializer for ParsedType 
static inline ParsedType parsedTypeDefault(void) {
    ParsedType t;
    memset(&t, 0, sizeof(ParsedType));
    t.kind = TYPE_INVALID;
    t.tag  = TAG_NONE;
    return t;
}
*/

#endif // PARSED_TYPE_H
