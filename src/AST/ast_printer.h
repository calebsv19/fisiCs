// SPDX-License-Identifier: Apache-2.0

#ifndef AST_PRINTER_H
#define AST_PRINTER_H

#include "AST/ast_node.h"
#include "Parser/Helpers/parsed_type.h"
#include "Parser/Helpers/designated_init.h"
#include <stdbool.h>

void printAST(ASTNode* node, int depth);
void printParsedType(const ParsedType* pt);
void printDesignatedInit(struct DesignatedInit* init, int depth);
void printBasicAST(ASTNode* node, int depth, bool newline);

#endif // AST_PRINTER_H

