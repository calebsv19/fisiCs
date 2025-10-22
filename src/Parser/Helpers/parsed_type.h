#ifndef PARSED_TYPE_H
#define PARSED_TYPE_H

#include <stdlib.h>

#include "Lexer/tokens.h"  // Required for TokenType
#include <stdbool.h>

struct Parser;

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
} ParsedType;


// Utility to convert TokenType to string
const char* getTokenTypeName(TokenType type);
void parsedTypeAddPointerDepth(ParsedType* t, int depth);

// Optional tiny helpers
void parsedTypeSetFunctionPointer(ParsedType* t, size_t nParams, const ParsedType* params);
void parsedTypeFree(ParsedType* t);  // free fpParams if set (call in your global cleanup)

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
