
#ifndef _PARSER_PREPROC_H
#define _PARSER_PREPROC_H

#include "parser.h"
#include "Parser/Helpers/parsed_type.h"

#include "AST/ast_node.h"

#include "Lexer/tokens.h"
#include "Lexer/lexer.h"

// Dispatcher
ASTNode* handlePreprocessorDirectives(Parser* parser);

// Individual directives
ASTNode* parseIncludeDirective(Parser* parser);
ASTNode* parseDefineDirective(Parser* parser);
ASTNode* parseConditionalDirective(Parser* parser);
ASTNode* parseEndifDirective(Parser* parser);

#endif // _PARSER_PREPROC_H

