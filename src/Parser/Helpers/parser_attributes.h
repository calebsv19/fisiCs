// SPDX-License-Identifier: Apache-2.0

#ifndef PARSER_ATTRIBUTES_H
#define PARSER_ATTRIBUTES_H

#include "parser.h"
#include "AST/ast_attribute.h"

bool parserLookaheadStartsAttribute(Parser* parser);
ASTAttribute** parserParseAttributeSpecifiers(Parser* parser, size_t* outCount);

#endif /* PARSER_ATTRIBUTES_H */
