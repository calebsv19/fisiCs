#ifndef PARSED_TYPE_H
#define PARSED_TYPE_H

#include "../Lexer/tokens.h"  // Required for TokenType
#include <stdbool.h>


// Enum for distinguishing between primitive and user-defined types
typedef enum {
    TYPE_INVALID,
    TYPE_PRIMITIVE,
    TYPE_USER_DEFINED
} TypeKind;

// Track if it's struct/union or just a typedef name
typedef enum { TAG_NONE, TAG_STRUCT, TAG_UNION } TagKind;

// Struct representing a parsed type
typedef struct {
    TypeKind kind;
    TagKind tag;

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


#endif // PARSED_TYPE_H

