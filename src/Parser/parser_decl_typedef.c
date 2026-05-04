// SPDX-License-Identifier: Apache-2.0

#include "parser_decl.h"

#include "Parser/Helpers/parser_attributes.h"
#include "Parser/Helpers/parser_helpers.h"

#include <stdio.h>
#include <stdlib.h>

ASTNode* parseTypedef(Parser* parser) {
    if (parser->currentToken.type != TOKEN_TYPEDEF) {
        printParseError("Expected 'typedef'", parser);
        return NULL;
    }

    advance(parser); // consume 'typedef'

    // Parse the base type (handles 'struct/union/enum' as needed)
    ParsedType baseType = parseType(parser);
    ASTNode** typedefNodes = NULL;
    size_t typedefCount = 0;
    size_t typedefCapacity = 0;

    while (true) {
        ParsedType declaratorBase = parsedTypeClone(&baseType);
        ParsedDeclarator decl;
        if (!parserParseDeclarator(parser, &declaratorBase, false, true, false, &decl)) {
            parsedTypeFree(&declaratorBase);
            const char* dbgFail = getenv("DEBUG_TYPEDEF_FAIL");
            if (dbgFail) {
                fprintf(stderr,
                        "[typedef] declarator parse failed at line %d token=%s\n",
                        parser->currentToken.line,
                        parser->currentToken.value ? parser->currentToken.value : "<null>");
            }
            if (typedefCount == 0 &&
                parser->currentToken.type == TOKEN_SEMICOLON &&
                parser->tokenBuffer && parser->cursor > 0) {
                const Token* prev = token_buffer_peek(parser->tokenBuffer, parser->cursor - 1);
                if (prev && prev->type == TOKEN_IDENTIFIER) {
                    ASTNode* alias = createIdentifierNode(prev->value);
                    if (alias) {
                        alias->line = prev->line;
                        astNodeSetProvenance(alias, prev);
                    }
                    advance(parser); // consume ';'
                    if (parser->ctx && prev->value && prev->value[0]) {
                        cc_add_typedef(parser->ctx, prev->value);
                    }
                    ASTNode* node = createTypedefNode(baseType, alias);
                    return node;
                }
            }
            printParseError("Invalid typedef declarator", parser);
            parsedTypeFree(&baseType);
            free(typedefNodes);
            return NULL;
        }

        if (typedefCount >= typedefCapacity) {
            size_t newCapacity = typedefCapacity ? (typedefCapacity * 2) : 2;
            ASTNode** resized = (ASTNode**)realloc(typedefNodes, newCapacity * sizeof(*resized));
            if (!resized) {
                parsedTypeFree(&baseType);
                parsedTypeFree(&decl.type);
                free(typedefNodes);
                fprintf(stderr, "OOM: typedef declarator list\n");
                return NULL;
            }
            typedefNodes = resized;
            typedefCapacity = newCapacity;
        }

        ASTNode* node = createTypedefNode(decl.type, decl.identifier);
        if (!node) {
            parsedTypeFree(&baseType);
            parsedTypeFree(&decl.type);
            free(typedefNodes);
            fprintf(stderr, "OOM: typedef node\n");
            return NULL;
        }
        typedefNodes[typedefCount++] = node;

        if (parser->currentToken.type != TOKEN_COMMA) {
            break;
        }
        advance(parser); // consume ','
    }
    parsedTypeFree(&baseType);

    size_t typedefAttrCount = 0;
    ASTAttribute** typedefAttrs = parserParseAttributeSpecifiers(parser, &typedefAttrCount);

    if (parser->currentToken.type != TOKEN_SEMICOLON) {
        printParseError("Expected ';' after typedef", parser);
        astAttributeListDestroy(typedefAttrs, typedefAttrCount);
        free(typedefAttrs);
        free(typedefNodes);
        return NULL;
    }
    advance(parser); // consume ';'

    for (size_t i = 0; i < typedefCount; ++i) {
        ASTNode* node = typedefNodes[i];
        ASTNode* alias = node ? node->typedefStmt.alias : NULL;
        const char* aliasName = (alias && alias->valueNode.value) ? alias->valueNode.value : NULL;
        if (parser->ctx && aliasName && aliasName[0]) {
            cc_add_typedef(parser->ctx, aliasName);
        }
        if (!node) {
            continue;
        }
        astNodeCloneTypeAttributes(node, &node->typedefStmt.baseType);
        if (typedefAttrCount == 0) {
            continue;
        }
        ASTAttribute** attrCopy = astAttributeListClone(typedefAttrs, typedefAttrCount);
        astNodeAppendAttributes(node, attrCopy, typedefAttrCount);
    }
    astAttributeListDestroy(typedefAttrs, typedefAttrCount);
    free(typedefAttrs);

    if (typedefCount == 1) {
        ASTNode* single = typedefNodes[0];
        free(typedefNodes);
        return single;
    }

    return createProgramNode(typedefNodes, typedefCount);
}
