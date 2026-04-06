// SPDX-License-Identifier: Apache-2.0

#ifndef AST_ATTRIBUTE_H
#define AST_ATTRIBUTE_H

#include <stddef.h>
#include <stdbool.h>

typedef enum {
    AST_ATTRIBUTE_SYNTAX_GNU,
    AST_ATTRIBUTE_SYNTAX_CXX,
    AST_ATTRIBUTE_SYNTAX_DECLSPEC
} ASTAttributeSyntax;

typedef struct ASTAttribute {
    ASTAttributeSyntax syntax;
    char* payload;
} ASTAttribute;

ASTAttribute* astAttributeCreate(ASTAttributeSyntax syntax, const char* payload);
ASTAttribute* astAttributeClone(const ASTAttribute* attr);
void astAttributeDestroy(ASTAttribute* attr);

ASTAttribute** astAttributeListClone(ASTAttribute* const* items, size_t count);
void astAttributeListDestroy(ASTAttribute** items, size_t count);
bool astAttributeListAppend(ASTAttribute*** list,
                            size_t* count,
                            ASTAttribute** extras,
                            size_t extraCount);

#endif /* AST_ATTRIBUTE_H */
