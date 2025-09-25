#include "Parser/Helpers/parsed_type.h"

#include <stdlib.h>
#include <string.h>

// Convert TokenType to string for debugging and printing
const char* getTokenTypeName(TokenType type) {
    switch (type) {
        case TOKEN_INT: return "int";
        case TOKEN_FLOAT: return "float";
        case TOKEN_DOUBLE: return "double";
        case TOKEN_CHAR: return "char";
        case TOKEN_BOOL: return "bool";
        case TOKEN_VOID: return "void";
        case TOKEN_SHORT: return "short";
        case TOKEN_LONG: return "long";
        case TOKEN_CONST: return "const";
        case TOKEN_SIGNED: return "signed";
        case TOKEN_UNSIGNED: return "unsigned";
        case TOKEN_VOLATILE: return "volatile";
        case TOKEN_INLINE: return "inline";
        case TOKEN_IDENTIFIER: return "identifier";
        default: return "unknown";
    }
}



void parsedTypeAddPointerDepth(ParsedType* t, int depth) {
    if (!t || depth <= 0) return;
    t->pointerDepth += depth;
}

void parsedTypeSetFunctionPointer(ParsedType* t, size_t nParams, const ParsedType* params) {
    if (!t) return;
    t->isFunctionPointer = true;
    t->fpParamCount = 0;
    t->fpParams = NULL;

    if (nParams == 0) return;

    ParsedType* copy = (ParsedType*)malloc(nParams * sizeof(ParsedType));
    if (!copy) return;

    memcpy(copy, params, nParams * sizeof(ParsedType));
    t->fpParams = copy;
    t->fpParamCount = nParams;
}

void parsedTypeFree(ParsedType* t) {
    if (!t) return;
    if (t->fpParams) {
        free(t->fpParams);
        t->fpParams = NULL;
    }
    t->fpParamCount = 0;
    t->isFunctionPointer = false;
}


