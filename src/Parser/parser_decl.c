// SPDX-License-Identifier: Apache-2.0

#include "parser_decl.h"
#include "Parser/Helpers/parser_helpers.h"
#include "Parser/Helpers/parser_lookahead.h"
#include "parser_func.h"
#include "parser_array.h"
#include "Parser/Helpers/parsed_type.h"
#include "Parser/Expr/parser_expr.h"
#include "Parser/Expr/parser_expr_pratt.h"
#include "Parser/Helpers/parser_attributes.h"

#include "Compiler/compiler_context.h" // make sure this is visible via include path

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool parser_allows_block_pointers(Parser* parser) {
    return parser && parser->ctx && cc_extensions_enabled(parser->ctx);
}

static void skip_gnu_attribute_specifier(Parser* parser) {
    if (!parser || parser->currentToken.type != TOKEN_IDENTIFIER || !parser->currentToken.value) {
        return;
    }
    if (strcmp(parser->currentToken.value, "__attribute__") != 0) {
        return;
    }
    advance(parser); // consume __attribute__
    if (parser->currentToken.type != TOKEN_LPAREN) {
        return;
    }
    int depth = 0;
    while (parser->currentToken.type != TOKEN_EOF) {
        if (parser->currentToken.type == TOKEN_LPAREN) {
            depth++;
        } else if (parser->currentToken.type == TOKEN_RPAREN) {
            depth--;
            if (depth <= 0) {
                advance(parser);
                break;
            }
        }
        advance(parser);
    }
}

ASTNode* handleTypeOrFunctionDeclaration(Parser* parser) {
    PARSER_DEBUG_PRINTF("DEBUG: Entered handleTypeOrFunctionDeclaration()\n");

    if (parser->currentToken.type == TOKEN_TYPEDEF) {
        return parseTypedef(parser);
    }

    // Parse the leading type (handles storage/specifiers)
    ParsedType parsedType = parseType(parser);
    if (parsedType.kind == TYPE_INVALID) {
        printParseError("Invalid type in declaration", parser);
        return NULL;
    }
    size_t typeAttrCount = 0;
    ASTAttribute** typeAttrs = parserParseAttributeSpecifiers(parser, &typeAttrCount);
    parsedTypeAdoptAttributes(&parsedType, typeAttrs, typeAttrCount);

    if (parser->currentToken.type == TOKEN_SEMICOLON &&
        parsedType.tag != TAG_NONE) {
        advance(parser); // consume ';' on tag-only declaration
        if (parser->ctx && parsedType.userTypeName) {
            CCTagKind tagKind = CC_TAG_STRUCT;
            if (parsedType.tag == TAG_UNION) tagKind = CC_TAG_UNION;
            else if (parsedType.tag == TAG_ENUM) tagKind = CC_TAG_ENUM;
            cc_add_tag(parser->ctx, tagKind, parsedType.userTypeName);
        }
        if (parsedType.inlineStructOrUnionDef) {
            return parsedType.inlineStructOrUnionDef;
        }
        if (parsedType.inlineEnumDef) {
            return parsedType.inlineEnumDef;
        }
        return NULL;
    }

    bool isFunctionDeclarator = false;
    Parser probeParser = cloneParserWithFreshLexer(parser);
    ParsedDeclarator probeDecl;
    if (parserParseDeclarator(&probeParser,
                              &parsedType,
                              true,
                              true,
                              false,
                              &probeDecl)) {
        // Only treat declarators that spell a direct function type (first
        // derivation == function) as function declarations/definitions. A
        // pointer-to-function should be parsed as a variable.
        isFunctionDeclarator = probeDecl.declaresFunction;
        parserDeclaratorDestroy(&probeDecl);
    }
    freeParserClone(&probeParser);

    if (isFunctionDeclarator) {
        PARSER_DEBUG_PRINTF("DEBUG: Detected function declaration or definition\n");
        return parseFunctionDefinition(parser, parsedType);
    }

    PARSER_DEBUG_PRINTF("DEBUG: Detected variable declaration\n");
    size_t varCount = 0;
    return parseVariableDeclaration(parser, parsedType, &varCount);
}


ASTNode* parseDeclaration(Parser* parser, ParsedType declaredType, size_t* varCount) {
    PARSER_DEBUG_PRINTF("DEBUG: Entering parseDeclaration() at line %d with token '%s'\n",
           parser->currentToken.line, parser->currentToken.value);
    return parseVariableDeclaration(parser, declaredType, varCount);
}

ASTNode* parseVariableDeclaration(Parser* parser, ParsedType declaredType, size_t* outVarCount) {
    size_t varCapacity = 4;
    size_t varCount = 0;   
    
    ASTNode** varNames = malloc(varCapacity * sizeof(ASTNode*));
    struct DesignatedInit** initializers = malloc(varCapacity * sizeof(DesignatedInit*));
    ParsedType* perTypes = malloc(varCapacity * sizeof(ParsedType));
    ASTAttribute** trailingAttrs = NULL;
    size_t trailingAttrCount = 0;
    
    if (!varNames || !initializers || !perTypes) {
        printf("Error: Memory allocation failed for variable declaration.\n");
        return NULL;
    }
     
    while (1) {
        ParsedDeclarator decl;
        if (!parserParseDeclarator(parser,
                                   &declaredType,
                                   false,
                                   true,
                                   false,
                                   &decl)) {
            printParseError("invalid declarator", parser);
            free(varNames);
            free(initializers);
            free(perTypes);
            return NULL;
        }
        ASTNode* varName = decl.identifier;

        struct DesignatedInit* init = NULL;
        
        // Handle assignment
        if (parser->currentToken.type == TOKEN_ASSIGN) {
            advance(parser);  // Consume '='
            
            if (parser->currentToken.type == TOKEN_LBRACE) {
                size_t count = 0;
                DesignatedInit** entries = parseInitializerList(parser, decl.type, &count);
                if (!entries) return NULL;
                
                // Wrap all entries into a compound literal node directly
                ASTNode* compound = createCompoundInit(entries, count);  
                
                // Treat the whole initializer as a single simple init (no .field or [index])
                init = createSimpleInit(compound);
            } else {
                ASTNode* expr = parseAssignmentExpression(parser);
                if (!expr) {
                    printParseError("Invalid initializer in variable declaration", parser);
                    init = NULL;
                } else {
                    init = createSimpleInit(expr);
                }
            }
        }
         
        size_t parsedAttrCount = 0;
        ASTAttribute** parsedAttrs = parserParseAttributeSpecifiers(parser, &parsedAttrCount);
        astAttributeListAppend(&trailingAttrs, &trailingAttrCount, parsedAttrs, parsedAttrCount);

        varNames[varCount] = varName;
        initializers[varCount] = init;
        perTypes[varCount] = decl.type;
        varCount++;
        
        // Expand if needed
        if (varCount >= varCapacity) {
            varCapacity *= 2;
            varNames = realloc(varNames, varCapacity * sizeof(ASTNode*));
            initializers = realloc(initializers, varCapacity * sizeof(DesignatedInit*));
            perTypes = realloc(perTypes, varCapacity * sizeof(ParsedType));
            if (!varNames || !initializers || !perTypes) {
                printf("Error: Memory reallocation failed.\n");
                return NULL;
            }
        }
         
        if (parser->currentToken.type == TOKEN_COMMA) {
            advance(parser);
            continue;
        }
        if (parser->currentToken.type != TOKEN_SEMICOLON) {
            if (parser && parser->ctx) {
                compiler_report_diag(parser->ctx,
                                     parser->currentToken.location,
                                     DIAG_ERROR,
                                     CDIAG_PARSER_EXPECT_SEMICOLON,
                                     "did you forget a ';'?",
                                     "expected ';' after declaration");
            }
            free(varNames);
            free(initializers);
            return NULL;
        }
         
        advance(parser);  // Consume ';'
        PARSER_DEBUG_PRINTF("DEBUG: Finished variable declaration, next token: '%s' (line %d)\n",
               parser->currentToken.value, parser->currentToken.line);
        break;
    }
     
    *outVarCount = varCount;
    ASTNode* node = createVariableDeclarationNode(declaredType, varNames, initializers, varCount);
    if (node) {
        node->varDecl.declaredTypes = perTypes;
        astNodeCloneTypeAttributes(node, &declaredType);
        astNodeAppendAttributes(node, trailingAttrs, trailingAttrCount);
        trailingAttrs = NULL;
        trailingAttrCount = 0;
        for (size_t i = 0; i < varCount; ++i) {
            ASTNode* ident = varNames[i];
            if (ident && ident->type == AST_IDENTIFIER && ident->valueNode.value) {
                parserRecordOrdinaryIdentifier(parser, ident->valueNode.value);
            }
        }
    }
    return node;
}
ASTNode* parseDeclarationForLoop(Parser* parser) {
    PARSER_DEBUG_PRINTF("DEBUG: Entering parseDeclarationForLoop() at line %d\n", parser->currentToken.line);
 
    //  Use the proper ParsedType handler
    ParsedType parsedType = parseType(parser);
    if (parsedType.kind == TYPE_INVALID) {
        printf("Error: Invalid type for for-loop initializer at line %d\n", 
				parser->currentToken.line);
        return NULL;
    }       
         
    size_t varCapacity = 4;
    size_t varCount = 0;
    ASTNode** varNames = malloc(varCapacity * sizeof(ASTNode*));
    DesignatedInit** initializers = malloc(varCapacity * sizeof(DesignatedInit*));
    ParsedType* perTypes = malloc(varCapacity * sizeof(ParsedType));
    if (!varNames || !initializers || !perTypes) {
        free(varNames);
        free(initializers);
        free(perTypes);
        return NULL;
    }

    while (1) {
        ParsedDeclarator decl;
        if (!parserParseDeclarator(parser, &parsedType, false, true, false, &decl)) {
            printParseError("invalid declarator in for-loop", parser);
            free(varNames);
            free(initializers);
            free(perTypes);
            return NULL;
        }

        ASTNode* initializer = NULL;
        if (parser->currentToken.type == TOKEN_ASSIGN) {
            advance(parser); // Consume '='
            initializer = parseAssignmentExpression(parser);
            if (!initializer) {
                printf("Error: Invalid initializer in for-loop at line %d\n", parser->currentToken.line);
                free(varNames);
                free(initializers);
                free(perTypes);
                return NULL;
            }
        }

        if (varCount >= varCapacity) {
            varCapacity *= 2;
            ASTNode** nextNames = realloc(varNames, varCapacity * sizeof(ASTNode*));
            DesignatedInit** nextInits = realloc(initializers, varCapacity * sizeof(DesignatedInit*));
            ParsedType* nextTypes = realloc(perTypes, varCapacity * sizeof(ParsedType));
            if (!nextNames || !nextInits || !nextTypes) {
                free(nextNames ? nextNames : varNames);
                free(nextInits ? nextInits : initializers);
                free(nextTypes ? nextTypes : perTypes);
                return NULL;
            }
            varNames = nextNames;
            initializers = nextInits;
            perTypes = nextTypes;
        }

        varNames[varCount] = decl.identifier;
        initializers[varCount] = createSimpleInit(initializer);
        perTypes[varCount] = decl.type;
        varCount++;

        if (parser->currentToken.type == TOKEN_COMMA) {
            advance(parser);
            continue;
        }
        break;
    }

    // Construct variable declaration using ParsedType.
    ASTNode* node = createVariableDeclarationNode(parsedType, varNames, initializers, varCount);
    if (node) {
        node->varDecl.declaredTypes = perTypes;
        astNodeCloneTypeAttributes(node, &parsedType);
        for (size_t i = 0; i < varCount; ++i) {
            ASTNode* ident = varNames[i];
            if (ident && ident->type == AST_IDENTIFIER && ident->valueNode.value) {
                parserRecordOrdinaryIdentifier(parser, ident->valueNode.value);
            }
        }
    } else {
        free(perTypes);
    }
    return node;
}


ASTNode* parseStructDefinition(Parser* parser) {
    if (parser->currentToken.type != TOKEN_STRUCT) {
        printParseError("Expected 'struct'", parser);
        return NULL;
    }
    bool precededByTypedef = false;
    if (parser->tokenBuffer && parser->cursor > 0) {
        const Token* prev = token_buffer_peek(parser->tokenBuffer, parser->cursor - 1);
        if (prev && prev->type == TOKEN_TYPEDEF) {
            precededByTypedef = true;
        }
    }
    advance(parser);  // consume 'struct'
    size_t structAttrCount = 0;
    ASTAttribute** structAttrs = parserParseAttributeSpecifiers(parser, &structAttrCount);
    const char* dbgAttrs = getenv("FISICS_DEBUG_LAYOUT");
    if (dbgAttrs) {
        fprintf(stderr, "[parse] struct leading attrs=%zu\n", structAttrCount);
    }

    char* structName = NULL;
    int structLine = 0;
    if (parser->currentToken.type == TOKEN_IDENTIFIER) {
        const char* tagName = parser->currentToken.value;
        structLine = parser->currentToken.line;
        structName = strdup(tagName);
        // Phase 2 hook: record the tag name in the struct tag namespace
        if (parser->ctx && tagName && tagName[0]) {
            cc_add_tag(parser->ctx, CC_TAG_STRUCT, tagName);
        }
        advance(parser);  // consume tag name
    }

    if (parser->currentToken.type != TOKEN_LBRACE) {
        printParseError("Expected '{' after struct name", parser);
        return NULL;
    }
    advance(parser);  // consume '{'

    size_t fieldCount = 0;
    bool hasFlexible = false;
    ASTNode** fields = parseStructOrUnionFields(parser, &fieldCount, &hasFlexible);

    if (parser->currentToken.type != TOKEN_RBRACE) {
        printParseError("Expected '}' to close struct body", parser);
        return NULL;
    }
    advance(parser);  // consume '}'

    size_t trailingAttrCount = 0;
    ASTAttribute** trailingAttrs = parserParseAttributeSpecifiers(parser, &trailingAttrCount);

    char* typedefAlias = NULL;
    int typedefAliasLine = 0;
    if (parser->currentToken.type == TOKEN_IDENTIFIER) {
        if (precededByTypedef) {
            typedefAlias = strdup(parser->currentToken.value);
            typedefAliasLine = parser->currentToken.line;
            advance(parser);
        } else if (!structName) {
            // Legacy behavior: treat trailing identifier as the tag name when missing.
            structName = strdup(parser->currentToken.value);
            advance(parser);
        }
    }

    if (parser->currentToken.type != TOKEN_SEMICOLON) {
        printParseError("Expected ';' after struct definition", parser);
        return NULL;
    }
    advance(parser);  // consume ';'

    ASTNode* def = createStructOrUnionDefinitionNode(
        AST_STRUCT_DEFINITION, structName, fields, fieldCount
    );
    if (def) {
        def->line = structLine;
        if (def->structDef.structName) def->structDef.structName->line = structLine;
        def->structDef.hasFlexibleArray = hasFlexible;
        astNodeAppendAttributes(def, structAttrs, structAttrCount);
        astNodeAppendAttributes(def, trailingAttrs, trailingAttrCount);
        structAttrs = NULL;
        structAttrCount = 0;
        trailingAttrs = NULL;
        trailingAttrCount = 0;
    } else {
        astAttributeListDestroy(structAttrs, structAttrCount);
        free(structAttrs);
        astAttributeListDestroy(trailingAttrs, trailingAttrCount);
        free(trailingAttrs);
    }
    if (!typedefAlias) {
        return def;
    }

    ParsedType baseType;
    memset(&baseType, 0, sizeof(baseType));
    baseType.kind = TYPE_STRUCT;
    baseType.tag = TAG_STRUCT;
    if (structName) {
        baseType.userTypeName = strdup(structName);
    }
    baseType.inlineStructOrUnionDef = def;

    ASTNode* alias = createIdentifierNode(typedefAlias);
    if (alias) {
        alias->line = typedefAliasLine;
    }
    if (parser->ctx && typedefAlias && typedefAlias[0]) {
        cc_add_typedef(parser->ctx, typedefAlias);
    }
    free(typedefAlias);

    ASTNode* typedefNode = createTypedefNode(baseType, alias);
    if (!typedefNode) {
        parsedTypeFree(&baseType);
        return def;
    }
    return typedefNode;
}


ASTNode* parseUnionDefinition(Parser* parser) {
    if (parser->currentToken.type != TOKEN_UNION) {
        printParseError("Expected 'union'", parser);
        return NULL;
    }
    advance(parser);  // consume 'union'
    size_t unionAttrCount = 0;
    ASTAttribute** unionAttrs = parserParseAttributeSpecifiers(parser, &unionAttrCount);

    char* unionName = NULL;
    int unionLine = 0;
    if (parser->currentToken.type == TOKEN_IDENTIFIER) {
        const char* tagName = parser->currentToken.value;
        unionLine = parser->currentToken.line;
        unionName = strdup(tagName);
        // Phase 2 hook: record union tag
        if (parser->ctx && tagName && tagName[0]) {
            cc_add_tag(parser->ctx, CC_TAG_UNION, tagName);
        }
        advance(parser); // consume tag identifier
    }

    if (parser->currentToken.type != TOKEN_LBRACE) {
        printParseError("Expected '{' after union name", parser);
        return NULL;
    }
    advance(parser);  // consume '{'

    size_t fieldCount = 0;
    bool hasFlexible = false;
    ASTNode** fields = parseStructOrUnionFields(parser, &fieldCount, &hasFlexible);

    if (parser->currentToken.type != TOKEN_RBRACE) {
        printParseError("Expected '}' at end of union definition", parser);
        return NULL;
    }
    advance(parser);  // consume '}'

    size_t trailingAttrCount = 0;
    ASTAttribute** trailingAttrs = parserParseAttributeSpecifiers(parser, &trailingAttrCount);

    if (parser->currentToken.type != TOKEN_SEMICOLON) {
        printParseError("Expected ';' after union definition", parser);
        return NULL;
    }
    advance(parser);  // consume ';'

    ASTNode* def = createStructOrUnionDefinitionNode(
        AST_UNION_DEFINITION, unionName, fields, fieldCount
    );
    if (def) {
        def->line = unionLine;
        if (def->structDef.structName) def->structDef.structName->line = unionLine;
        def->structDef.hasFlexibleArray = hasFlexible;
        astNodeAppendAttributes(def, unionAttrs, unionAttrCount);
        astNodeAppendAttributes(def, trailingAttrs, trailingAttrCount);
        unionAttrs = NULL;
        unionAttrCount = 0;
        trailingAttrs = NULL;
        trailingAttrCount = 0;
    } else {
        astAttributeListDestroy(unionAttrs, unionAttrCount);
        free(unionAttrs);
        astAttributeListDestroy(trailingAttrs, trailingAttrCount);
        free(trailingAttrs);
    }
    return def;
}




ASTNode** parseStructOrUnionFields(Parser* parser, size_t* outCount, bool* outHasFlexible) {
    ASTNode** fields = malloc(sizeof(ASTNode*) * 4);
    size_t count = 0;
    size_t capacity = 4;
    bool hasFlexible = false;

    while (parser->currentToken.type != TOKEN_RBRACE &&
           parser->currentToken.type != TOKEN_EOF) {

        ParsedType type = parseType(parser);
        if (type.kind == TYPE_INVALID) {
            printParseError("Invalid type in field declaration", parser);
            break;
        }

        if (parser->currentToken.type == TOKEN_SEMICOLON &&
            type.inlineStructOrUnionDef) {
            advance(parser); // consume ';'
            ASTNode** names = malloc(sizeof(ASTNode*));
            DesignatedInit** initializers = malloc(sizeof(DesignatedInit*));
            names[0] = NULL;
            initializers[0] = NULL;
            ASTNode* fieldDecl = createVariableDeclarationNode(type, names, initializers, 1);
            if (fieldDecl) {
                ParsedType* per = malloc(sizeof(ParsedType));
                if (per) {
                    per[0] = type;
                    fieldDecl->varDecl.declaredTypes = per;
                }
                astNodeCloneTypeAttributes(fieldDecl, &type);
            }

            if (count >= capacity) {
                capacity *= 2;
                fields = realloc(fields, sizeof(ASTNode*) * capacity);
            }
            fields[count++] = fieldDecl;
            continue;
        }

        for (;;) {
            ParsedDeclarator decl;
            if (!parserParseDeclarator(parser, &type, false, false, false, &decl)) {
                printParseError("Invalid field declarator", parser);
                goto finish_fields;
            }
            ASTNode* fieldName = decl.identifier;

            // Optional bitfield width
            ASTNode* bitFieldWidth = NULL;
            if (parser->currentToken.type == TOKEN_COLON) {
                advance(parser);
                bitFieldWidth = parseAssignmentExpression(parser);
            }

            if (!fieldName && !bitFieldWidth) {
                printParseError("Expected identifier for struct/union field (unnamed only allowed for bitfields)", parser);
                goto finish_fields;
            }

            size_t fieldAttrCount = 0;
            ASTAttribute** fieldAttrs = parserParseAttributeSpecifiers(parser, &fieldAttrCount);

            ASTNode** names = malloc(sizeof(ASTNode*));
            DesignatedInit** initializers = malloc(sizeof(DesignatedInit*));
            names[0] = fieldName;
            initializers[0] = NULL;
            ASTNode* fieldDecl = createVariableDeclarationNode(decl.type, names, initializers, 1);
            if (fieldDecl) {
                ParsedType* per = malloc(sizeof(ParsedType));
                if (per) {
                    per[0] = decl.type;
                    fieldDecl->varDecl.declaredTypes = per;
                }
            }

            if (fieldDecl && fieldDecl->type == AST_VARIABLE_DECLARATION) {
                ParsedType* fieldType = fieldDecl->varDecl.declaredTypes
                    ? &fieldDecl->varDecl.declaredTypes[0]
                    : &fieldDecl->varDecl.declaredType;
                if (fieldType && fieldType->derivationCount > 0) {
                    TypeDerivation* last = parsedTypeGetMutableArrayDerivation(fieldType,
                                                                               fieldType->derivationCount - 1);
                    if (last &&
                        last->kind == TYPE_DERIVATION_ARRAY &&
                        !last->as.array.isVLA &&
                        !last->as.array.hasConstantSize &&
                        last->as.array.sizeExpr == NULL) {
                        last->as.array.isFlexible = true;
                        hasFlexible = true;
                    }
                }
                fieldDecl->varDecl.bitFieldWidth = bitFieldWidth;
            }
            if (fieldDecl) {
                astNodeCloneTypeAttributes(fieldDecl, &decl.type);
                astNodeAppendAttributes(fieldDecl, fieldAttrs, fieldAttrCount);
                fieldAttrs = NULL;
                fieldAttrCount = 0;
            } else {
                astAttributeListDestroy(fieldAttrs, fieldAttrCount);
                free(fieldAttrs);
            }

            if (count >= capacity) {
                capacity *= 2;
                fields = realloc(fields, sizeof(ASTNode*) * capacity);
            }
            fields[count++] = fieldDecl;

            if (parser->currentToken.type == TOKEN_COMMA) {
                advance(parser); // next declarator in same field list
                continue;
            }
            if (parser->currentToken.type == TOKEN_SEMICOLON) {
                advance(parser); // consume ';'
                break;
            }
            printParseError("Expected ';' after struct/union field", parser);
            goto finish_fields;
        }
    }

finish_fields:
    if (outHasFlexible) {
        *outHasFlexible = hasFlexible;
    }
    *outCount = count;
    return fields;
}


ASTNode* parseEnumDefinition(Parser* parser) {
    if (parser->currentToken.type != TOKEN_ENUM) {
        printParseError("Expected 'enum'", parser);
        return NULL;
    }

    int enumLine = parser->currentToken.line;
    advance(parser); // consume 'enum'
    size_t enumAttrCount = 0;
    ASTAttribute** enumAttrs = parserParseAttributeSpecifiers(parser, &enumAttrCount);

    const char* enumName = NULL;
    if (parser->currentToken.type == TOKEN_IDENTIFIER) {
        enumName = parser->currentToken.value;
        enumLine = parser->currentToken.line;
        // Phase 2 hook: record enum tag
        if (parser->ctx && enumName && enumName[0]) {
            cc_add_tag(parser->ctx, CC_TAG_ENUM, enumName);
        }
        advance(parser); // consume name
    }

    if (parser->currentToken.type != TOKEN_LBRACE) {
        printParseError("Expected '{' to begin enum body", parser);
        return NULL;
    }
    advance(parser); // consume '{'

    ASTNode** members = NULL;
    ASTNode** values  = NULL;
    size_t count = 0, capacity = 4;
    members = malloc(capacity * sizeof(ASTNode*));
    values  = malloc(capacity * sizeof(ASTNode*));

    while (parser->currentToken.type == TOKEN_IDENTIFIER) {
        ASTNode* memberName = createIdentifierNode(parser->currentToken.value);
        astNodeSetProvenance(memberName, &parser->currentToken);
        advance(parser); // consume member name
        size_t memberAttrCount = 0;
        ASTAttribute** memberAttrs = parserParseAttributeSpecifiers(parser, &memberAttrCount);
        if (memberAttrs) {
            astAttributeListDestroy(memberAttrs, memberAttrCount);
            free(memberAttrs);
        }
        while (parser->currentToken.type == TOKEN_IDENTIFIER &&
               parser->currentToken.value &&
               strcmp(parser->currentToken.value, "__attribute__") == 0) {
            skip_gnu_attribute_specifier(parser);
        }

        ASTNode* valueExpr = NULL;
        if (parser->currentToken.type == TOKEN_ASSIGN) {
            advance(parser); // consume '='
            valueExpr = parseAssignmentExpression(parser);
            if (!valueExpr) {
                printParseError("Invalid value expression in enum", parser);
                return NULL;
            }
        }

        if (count >= capacity) {
            capacity *= 2;
            members = realloc(members, capacity * sizeof(ASTNode*));
            values  = realloc(values,  capacity * sizeof(ASTNode*));
        }

        members[count] = memberName;
        values [count] = valueExpr;
        count++;

        size_t trailingAttrCount = 0;
        ASTAttribute** trailingAttrs = parserParseAttributeSpecifiers(parser, &trailingAttrCount);
        if (trailingAttrs) {
            astAttributeListDestroy(trailingAttrs, trailingAttrCount);
            free(trailingAttrs);
        }
        while (parser->currentToken.type == TOKEN_IDENTIFIER &&
               parser->currentToken.value &&
               strcmp(parser->currentToken.value, "__attribute__") == 0) {
            skip_gnu_attribute_specifier(parser);
        }

        if (parser->currentToken.type == TOKEN_COMMA) {
            advance(parser); // continue
        } else if (parser->currentToken.type == TOKEN_RBRACE) {
            break; // end of enum
        } else {
            printParseError("Expected ',' or '}' after enum member", parser);
            return NULL;
        }
    }

    if (count == 0) {
        printParseError("Enum declaration requires at least one enumerator", parser);
        free(members);
        free(values);
        astAttributeListDestroy(enumAttrs, enumAttrCount);
        free(enumAttrs);
        return NULL;
    }

    if (parser->currentToken.type != TOKEN_RBRACE) {
        printParseError("Expected '}' to close enum body", parser);
        return NULL;
    }
    advance(parser); // consume '}'

    if (parser->currentToken.type != TOKEN_SEMICOLON) {
        printParseError("Expected ';' after enum declaration", parser);
        return NULL;
    }
    advance(parser); // consume ';'

    char anonName[64];
    if (!enumName) {
        unsigned long long anonId = parser ? parser->anonTagCounter++ : 0ULL;
        snprintf(anonName, sizeof(anonName), "__anon_enum_%llu_%d", anonId, enumLine);
        enumName = anonName;
        if (parser->ctx) {
            cc_add_tag(parser->ctx, CC_TAG_ENUM, enumName);
        }
    }

    ASTNode* node = createEnumDefinitionNode(enumName, members, values, count);
    if (node) {
        node->line = enumLine;
        if (node->enumDef.enumName) node->enumDef.enumName->line = enumLine;
        astNodeAppendAttributes(node, enumAttrs, enumAttrCount);
        enumAttrs = NULL;
        enumAttrCount = 0;
    } else {
        astAttributeListDestroy(enumAttrs, enumAttrCount);
        free(enumAttrs);
    }
    return node;
}



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




ASTNode* handleStructStatements(Parser* parser) {
    // Allow bare struct/union definitions at file scope: `struct S { ... };`
    Parser defProbe = cloneParserWithFreshLexer(parser);
    ParsedType defProbeType = parseType(&defProbe);
    if (defProbeType.inlineStructOrUnionDef && defProbe.currentToken.type == TOKEN_SEMICOLON) {
        parsedTypeFree(&defProbeType);
        freeParserClone(&defProbe);

        ParsedType realType = parseType(parser);
        ASTNode* def = realType.inlineStructOrUnionDef;
        if (!def) {
            parsedTypeFree(&realType);
            printParseError("Invalid struct/union definition", parser);
            return NULL;
        }
        if (parser->currentToken.type != TOKEN_SEMICOLON) {
            parsedTypeFree(&realType);
            printParseError("Expected ';' after struct/union definition", parser);
            return NULL;
        }
        advance(parser); // consume ';'
        parsedTypeFree(&realType); // leaves inline def intact
        return def;
    }
    parsedTypeFree(&defProbeType);
    freeParserClone(&defProbe);

    // First, attempt to treat this as a declaration (including inline definitions)
    Parser probe = cloneParserWithFreshLexer(parser);
    ParsedType probeType = parseType(&probe);
    bool looksLikeDeclaratorStart =
        (probe.currentToken.type == TOKEN_IDENTIFIER) ||
        (probe.currentToken.type == TOKEN_ASTERISK)   ||
        (parser_allows_block_pointers(parser) && probe.currentToken.type == TOKEN_BITWISE_XOR) ||
        (probe.currentToken.type == TOKEN_LPAREN);
    if (probeType.kind != TYPE_INVALID && looksLikeDeclaratorStart) {
        ParsedDeclarator probeDecl;
        if (parserParseDeclarator(&probe,
                                  &probeType,
                                  true,
                                  true,
                                  false,
                                  &probeDecl)) {
            parserDeclaratorDestroy(&probeDecl);
            parsedTypeFree(&probeType);
            freeParserClone(&probe);
            return handleTypeOrFunctionDeclaration(parser);
        }
        parserDeclaratorDestroy(&probeDecl);
    }
    parsedTypeFree(&probeType);
    freeParserClone(&probe);

    Parser look = cloneParserWithFreshLexer(parser);
    advance(&look); // consume 'struct'
    size_t skippedAttrCount = 0;
    ASTAttribute** skippedAttrs = parserParseAttributeSpecifiers(&look, &skippedAttrCount);
    astAttributeListDestroy(skippedAttrs, skippedAttrCount);
    free(skippedAttrs);
    Token next = look.currentToken;
    Token after = peekNextToken(&look);
    Token third = peekTwoTokensAhead(&look);
    freeParserClone(&look);
    
    // Case 1: struct Foo { ... } — full definition
    if (next.type == TOKEN_IDENTIFIER && after.type == TOKEN_LBRACE) {
        return parseStructDefinition(parser);
    }
     
    // Case 2: struct Foo varName; or struct Foo funcName(); — needs deeper inspection
    if (next.type == TOKEN_IDENTIFIER && after.type == TOKEN_IDENTIFIER) {
        // struct Foo varName;
        // struct Foo funcName(); ← handled inside declaration logic
        PARSER_DEBUG_PRINTF("DEBUG: Routing struct return function declaration to handleTypeOrFunctionDeclaration");
        return handleTypeOrFunctionDeclaration(parser);
    }   
    
    // Case 3: struct Foo func(); — detect struct-returning function declaration
    if (next.type == TOKEN_IDENTIFIER &&
        after.type == TOKEN_IDENTIFIER &&
        third.type == TOKEN_LPAREN) {
        return handleTypeOrFunctionDeclaration(parser);  
    }
    
    // Case 4: struct Foo; — forward declaration (ignore for now)
    if (next.type == TOKEN_IDENTIFIER && after.type == TOKEN_SEMICOLON) {
        advance(parser); // struct
        if (parser->currentToken.type != TOKEN_IDENTIFIER) {
            printParseError("Expected identifier after 'struct' in forward declaration", parser);
            return NULL;
	}
        const char* tagName = parser->currentToken.value;
        if (parser->ctx && tagName && tagName[0]) {
            cc_add_tag(parser->ctx, CC_TAG_STRUCT, tagName);
        }
        advance(parser); // Foo
        if (parser->currentToken.type != TOKEN_SEMICOLON) {
            printParseError("Expected ';' after struct forward declaration", parser);
            return NULL;
        }
        advance(parser); // ;
        return NULL;
    }
     
    // Fallback: treat as a normal declaration (covers inline definitions with declarators)
    return handleTypeOrFunctionDeclaration(parser);
}
