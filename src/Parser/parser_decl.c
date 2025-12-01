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


static void consumePointerQualifiers(Parser* parser, PointerDeclaratorLayer* layer) {
    while (parser->currentToken.type == TOKEN_CONST ||
           parser->currentToken.type == TOKEN_VOLATILE ||
           parser->currentToken.type == TOKEN_RESTRICT) {
        if (layer) {
            if (parser->currentToken.type == TOKEN_CONST) {
                layer->isConst = true;
            } else if (parser->currentToken.type == TOKEN_VOLATILE) {
                layer->isVolatile = true;
            } else if (parser->currentToken.type == TOKEN_RESTRICT) {
                layer->isRestrict = true;
            }
        }
        advance(parser);
    }
}

PointerChain parsePointerChain(Parser* parser) {
    PointerChain chain = {0};
    while (parser->currentToken.type == TOKEN_ASTERISK) {
        advance(parser);
        PointerDeclaratorLayer layer = {0};
        consumePointerQualifiers(parser, &layer);
        PointerDeclaratorLayer* grown = realloc(chain.layers, (chain.count + 1) * sizeof(PointerDeclaratorLayer));
        if (!grown) {
            pointerChainFree(&chain);
            chain.layers = NULL;
            chain.count = 0;
            return chain;
        }
        chain.layers = grown;
        chain.layers[chain.count++] = layer;
    }
    return chain;
}

void pointerChainFree(PointerChain* chain) {
    if (!chain) return;
    free(chain->layers);
    chain->layers = NULL;
    chain->count = 0;
}

void applyPointerChainToType(ParsedType* type, const PointerChain* chain) {
    if (!type || !chain || chain->count == 0) {
        return;
    }
    for (size_t i = 0; i < chain->count; ++i) {
        if (!parsedTypeAppendPointer(type)) {
            continue;
        }
        if (type->derivationCount == 0) continue;
        TypeDerivation* slot = &type->derivations[type->derivationCount - 1];
        slot->as.pointer.isConst = chain->layers[i].isConst;
        slot->as.pointer.isVolatile = chain->layers[i].isVolatile;
        slot->as.pointer.isRestrict = chain->layers[i].isRestrict;
    }
}

bool parserConsumeArraySuffixes(Parser* parser, ParsedType* type, ASTNode** outSizeChain) {
    if (!parser || !type) return false;
    while (parser->currentToken.type == TOKEN_LBRACKET) {
        ASTNode* sizeExpr = parseArraySize(parser);
        if (!parsedTypeAppendArray(type, sizeExpr, false)) {
            return false;
        }
        if (outSizeChain) {
            *outSizeChain = chainArraySizes(*outSizeChain, sizeExpr);
        }
    }
    return true;
}

static bool probeDeclaratorTokens(Parser* parser, Token* outIdent, Token* outNext, Token* outNextNext) {
    Parser temp = cloneParserWithFreshLexer(parser);
    parsePointerChain(&temp);
    if (temp.currentToken.type != TOKEN_IDENTIFIER) {
        freeParserClone(&temp);
        return false;
    }
    if (outIdent) {
        *outIdent = temp.currentToken;
    }
    advance(&temp);
    if (outNext) {
        *outNext = temp.currentToken;
    }
    advance(&temp);
    if (outNextNext) {
        *outNextNext = temp.currentToken;
    }
    freeParserClone(&temp);
    return true;
}





ASTNode* handleTypeOrFunctionDeclaration(Parser* parser) {
    PARSER_DEBUG_PRINTF("DEBUG: Entered handleTypeOrFunctionDeclaration()\n");

    // Parse the leading type (handles storage/specifiers)
    ParsedType parsedType = parseType(parser);
    if (parsedType.kind == TYPE_INVALID) {
        printParseError("Invalid type in declaration", parser);
        return NULL;
    }

    // Function pointer: type followed by '(' '*'
    if (parser->currentToken.type == TOKEN_LPAREN) {
        Token afterParen = peekNextToken(parser);
        if (afterParen.type == TOKEN_ASTERISK) {
            return parseFunctionPointerDeclaration(parser, parsedType);
        }
    }

    Token identToken;
    Token nextToken;
    Token afterToken;
    bool hasIdent = probeDeclaratorTokens(parser, &identToken, &nextToken, &afterToken);
    if (!hasIdent) {
        printParseError("Expected identifier after type", parser);
        return NULL;
    }

    // --- Case 1: Function (definition or declaration) ---
    if (nextToken.type == TOKEN_LPAREN) {
        PARSER_DEBUG_PRINTF("DEBUG: Detected function declaration or definition\n");
        return parseFunctionDefinition(parser, parsedType);
    }

    // --- Case 2: Array Declaration ---
    if (nextToken.type == TOKEN_LBRACKET) {
        PARSER_DEBUG_PRINTF("DEBUG: Detected array declaration\n");
        return parseArrayDeclaration(parser, parsedType);
    }

    // --- Case 3: Variable Declaration (with optional init or commas) ---
    if (nextToken.type == TOKEN_ASSIGN || nextToken.type == TOKEN_SEMICOLON || nextToken.type == TOKEN_COMMA) {
        PARSER_DEBUG_PRINTF("DEBUG: Detected variable declaration\n");
        size_t varCount = 0;
        return parseVariableDeclaration(parser, parsedType, &varCount);
    }

    // --- Fallback ---
    printParseError("Unrecognized declaration or function definition pattern", parser);
    return NULL;
}


ASTNode* parseDeclaration(Parser* parser, ParsedType declaredType, size_t* varCount) {
    PARSER_DEBUG_PRINTF("DEBUG: Entering parseDeclaration() at line %d with token '%s'\n",
           parser->currentToken.line, parser->currentToken.value);
           
    // Expect at least one identifier after type
    if (parser->currentToken.type != TOKEN_IDENTIFIER) {
        printParseError("Expected variable name after type", parser);
        return NULL;
    }
     
    // Check if this is an array declaration
    if (peekNextToken(parser).type == TOKEN_LBRACKET) {
        return parseArrayDeclaration(parser, declaredType);
    }
     
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
        ParsedType varType = parsedTypeClone(&declaredType);
        PointerChain chain = parsePointerChain(parser);
        applyPointerChainToType(&varType, &chain);
        pointerChainFree(&chain);

        if (parser->currentToken.type != TOKEN_IDENTIFIER) {
            printf("Error: expected variable name at line %d\n", parser->currentToken.line);
            free(varNames);
            free(initializers);
            free(perTypes);
            return NULL;
        }
         
        ASTNode* varName = createIdentifierNode(parser->currentToken.value);
        if (varName) varName->line = parser->currentToken.line;
        advance(parser);  // Consume identifier
        
//        ASTNode* arraySize = parseArraySize(parser);  // May be NULL
        struct DesignatedInit* init = NULL;
        
        // Handle assignment
        if (parser->currentToken.type == TOKEN_ASSIGN) {
            advance(parser);  // Consume '='
            
            if (parser->currentToken.type == TOKEN_LBRACE) {
                size_t count = 0;
                DesignatedInit** entries = parseInitializerList(parser, declaredType, &count);
                if (!entries) return NULL;
                
                // Wrap all entries into a compound literal node directly
                ASTNode* compound = createCompoundInit(entries, count);  
                
                // Treat the whole initializer as a single simple init (no .field or [index])
                init = createSimpleInit(compound);
            } else {
                ASTNode* expr = parseExpression(parser);
                if (!expr) {
                    printf("Error: Invalid initializer in variable declaration at line %d\n",
                           parser->currentToken.line);
                    free(varNames);
                    free(initializers);
                    return NULL;
                }
                init = createSimpleInit(expr);
            }
        }
         
        size_t parsedAttrCount = 0;
        ASTAttribute** parsedAttrs = parserParseAttributeSpecifiers(parser, &parsedAttrCount);
        astAttributeListAppend(&trailingAttrs, &trailingAttrCount, parsedAttrs, parsedAttrCount);

        varNames[varCount] = varName;
        initializers[varCount] = init;
        perTypes[varCount] = varType;
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
            printf("Error: expected ';' at line %d\n", parser->currentToken.line);
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
    }
    return node;
}




ASTNode* parseArrayDeclaration(Parser* parser, ParsedType type) {
    ParsedType arrayType = parsedTypeClone(&type);
    PointerChain chain = parsePointerChain(parser);
    applyPointerChainToType(&arrayType, &chain);
    pointerChainFree(&chain);
    if (parser->currentToken.type != TOKEN_IDENTIFIER) {
        printParseError("Expected identifier in array declaration", parser);
        return NULL;
    }
    ASTNode* name = createIdentifierNode(parser->currentToken.value);
    if (name) name->line = parser->currentToken.line;
    advance(parser); // consume name
    
    ASTNode* sizeExpr = NULL;
    if (!parserConsumeArraySuffixes(parser, &arrayType, &sizeExpr)) {
        printParseError("Failed to parse array suffix", parser);
        return NULL;
    }
     
    DesignatedInit** initValues = NULL;
    size_t valueCount = 0;
    
    if (parser->currentToken.type == TOKEN_ASSIGN) {
        advance(parser); // consume '='
        initValues = parseArrayInitializer(parser, type, &valueCount); // parses [i] = v, etc.
        if (!initValues) return NULL;
    }
     
    if (parser->currentToken.type != TOKEN_SEMICOLON) {
        printParseError("Expected ';' after array declaration", parser);
        return NULL;
    }

    size_t attrCount = 0;
    ASTAttribute** attrs = parserParseAttributeSpecifiers(parser, &attrCount);

    advance(parser); // consume ';'
    
    ASTNode* node = createArrayDeclarationNode(arrayType, name, sizeExpr, initValues, valueCount);
    if (node) {
        astNodeCloneTypeAttributes(node, &type);
        astNodeAppendAttributes(node, attrs, attrCount);
    } else {
        astAttributeListDestroy(attrs, attrCount);
        free(attrs);
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
         
    ParsedType varType = parsedTypeClone(&parsedType);
    PointerChain chain = parsePointerChain(parser);
    applyPointerChainToType(&varType, &chain);
    pointerChainFree(&chain);

    if (parser->currentToken.type != TOKEN_IDENTIFIER) {
        printf("Error: expected variable name in for-loop initializer at line %d\n",
               parser->currentToken.line);
        return NULL;
    }

    ASTNode* idNode = createIdentifierNode(parser->currentToken.value);
    if (idNode) idNode->line = parser->currentToken.line;
    advance(parser); // Consume identifier
    
    parserConsumeArraySuffixes(parser, &varType, NULL);

    ASTNode* initializer = NULL;
    if (parser->currentToken.type == TOKEN_ASSIGN) {
        advance(parser); // Consume '='
        initializer = parseExpression(parser);
        if (!initializer) {
            printf("Error: Invalid initializer in for-loop at line %d\n", parser->currentToken.line);
            return NULL;
        }
    }
    
    // Allocate identifier and initializer arrays  
    size_t varCount = 1;
    ASTNode** varNames = malloc(sizeof(ASTNode*));
    DesignatedInit** initializers = malloc(sizeof(DesignatedInit*));
    
    varNames[0] = idNode;
    initializers[0] = createSimpleInit(initializer);
    
    
    //  Construct variable declaration using ParsedType
    ASTNode* node = createVariableDeclarationNode(parsedType, varNames, initializers, varCount);
    if (node) {
        ParsedType* perTypes = malloc(sizeof(ParsedType));
        if (perTypes) {
            perTypes[0] = varType;
            node->varDecl.declaredTypes = perTypes;
        }
        astNodeCloneTypeAttributes(node, &parsedType);
    }
    return node;
}


ASTNode* parseStructDefinition(Parser* parser) {
    printf("    entering struct parsing\n");
    if (parser->currentToken.type != TOKEN_STRUCT) {
        printParseError("Expected 'struct'", parser);
        return NULL;
    }
    advance(parser);  // consume 'struct'
    size_t structAttrCount = 0;
    ASTAttribute** structAttrs = parserParseAttributeSpecifiers(parser, &structAttrCount);

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
    ASTNode** fields = parseStructOrUnionFields(parser, &fieldCount);

    if (parser->currentToken.type != TOKEN_RBRACE) {
        printParseError("Expected '}' to close struct body", parser);
        return NULL;
    }
    advance(parser);  // consume '}'

    // NOTE: In standard C, an identifier *after* '}' would start a declarator
    // (e.g., 'struct S { ... } var;'). Your function treats it as an "optional trailing name".
    // That trailing identifier is *not* a tag; do NOT record it as a tag.
    if (!structName && parser->currentToken.type == TOKEN_IDENTIFIER) {
        structName = strdup(parser->currentToken.value);
        advance(parser);
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
        astNodeAppendAttributes(def, structAttrs, structAttrCount);
        structAttrs = NULL;
        structAttrCount = 0;
    } else {
        astAttributeListDestroy(structAttrs, structAttrCount);
        free(structAttrs);
    }
    return def;
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
    ASTNode** fields = parseStructOrUnionFields(parser, &fieldCount);

    if (parser->currentToken.type != TOKEN_RBRACE) {
        printParseError("Expected '}' at end of union definition", parser);
        return NULL;
    }
    advance(parser);  // consume '}'

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
        astNodeAppendAttributes(def, unionAttrs, unionAttrCount);
        unionAttrs = NULL;
        unionAttrCount = 0;
    } else {
        astAttributeListDestroy(unionAttrs, unionAttrCount);
        free(unionAttrs);
    }
    return def;
}




ASTNode** parseStructOrUnionFields(Parser* parser, size_t* outCount) {
    ASTNode** fields = malloc(sizeof(ASTNode*) * 4);
    size_t count = 0;
    size_t capacity = 4;

    while (parser->currentToken.type != TOKEN_RBRACE &&
           parser->currentToken.type != TOKEN_EOF) {

        ParsedType type = parseType(parser);
        if (type.kind == TYPE_INVALID) {
            printParseError("Invalid type in field declaration", parser);
            break;
        }

        ParsedType fieldType = parsedTypeClone(&type);
        PointerChain chain = parsePointerChain(parser);
        applyPointerChainToType(&fieldType, &chain);
        pointerChainFree(&chain);

        if (parser->currentToken.type != TOKEN_IDENTIFIER) {
            printParseError("Expected field name in struct/union", parser);
            break;
        }

        ASTNode* fieldName = createIdentifierNode(parser->currentToken.value);
        if (fieldName) fieldName->line = parser->currentToken.line;
        advance(parser); // consume field name

        // Parse array sizes if present
        ASTNode* arraySize = NULL;
        if (!parserConsumeArraySuffixes(parser, &fieldType, &arraySize)) {
            printParseError("Failed to parse array suffix in field", parser);
            break;
        }

        // Optional bitfield width
        ASTNode* bitFieldWidth = NULL;
        if (parser->currentToken.type == TOKEN_COLON) {
            advance(parser);
            bitFieldWidth = parseAssignmentExpression(parser);
        }

        size_t fieldAttrCount = 0;
        ASTAttribute** fieldAttrs = parserParseAttributeSpecifiers(parser, &fieldAttrCount);

        if (parser->currentToken.type != TOKEN_SEMICOLON) {
            printParseError("Expected ';' after struct/union field", parser);
            break;
        }
        advance(parser); // consume ';'

        ASTNode* fieldDecl = NULL;
        if (arraySize) {
            fieldDecl = createArrayDeclarationNode(fieldType, fieldName, arraySize, NULL, 0);
        } else {
            ASTNode** names = malloc(sizeof(ASTNode*));
            DesignatedInit** initializers = malloc(sizeof(DesignatedInit*));
            names[0] = fieldName;
            initializers[0] = NULL;
            fieldDecl = createVariableDeclarationNode(fieldType, names, initializers, 1);
            if (fieldDecl) {
                ParsedType* per = malloc(sizeof(ParsedType));
                if (per) {
                    per[0] = fieldType;
                    fieldDecl->varDecl.declaredTypes = per;
                }
            }
        }

        if (fieldDecl && fieldDecl->type == AST_VARIABLE_DECLARATION) {
            fieldDecl->varDecl.bitFieldWidth = bitFieldWidth;
        }
        if (fieldDecl) {
            astNodeCloneTypeAttributes(fieldDecl, &fieldType);
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
    }

    *outCount = count;
    return fields;
}


ASTNode* parseEnumDefinition(Parser* parser) {
    if (parser->currentToken.type != TOKEN_ENUM) {
        printParseError("Expected 'enum'", parser);
        return NULL;
    }

    advance(parser); // consume 'enum'
    size_t enumAttrCount = 0;
    ASTAttribute** enumAttrs = parserParseAttributeSpecifiers(parser, &enumAttrCount);

    if (parser->currentToken.type != TOKEN_IDENTIFIER) {
        printParseError("Expected enum name after 'enum'", parser);
        return NULL;
    }

    const char* enumName = parser->currentToken.value;
    int enumLine = parser->currentToken.line;
    // Phase 2 hook: record enum tag
    if (parser->ctx && enumName && enumName[0]) {
        cc_add_tag(parser->ctx, CC_TAG_ENUM, enumName);
    }
    advance(parser); // consume name

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
        if (memberName) memberName->line = parser->currentToken.line;
        advance(parser); // consume member name

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

        if (parser->currentToken.type == TOKEN_COMMA) {
            advance(parser); // continue
        } else if (parser->currentToken.type == TOKEN_RBRACE) {
            break; // end of enum
        } else {
            printParseError("Expected ',' or '}' after enum member", parser);
            return NULL;
        }
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
    PointerChain chain = parsePointerChain(parser);
    applyPointerChainToType(&baseType, &chain);
    pointerChainFree(&chain);

    // NOTE: Your current implementation supports exactly one alias.
    // We'll keep that, but we also record the alias in the CompilerContext.
    if (parser->currentToken.type != TOKEN_IDENTIFIER) {
        printParseError("Expected identifier name for typedef", parser);
        return NULL;
    }

    // Capture the alias name *before* advance, for context recording
    const char* aliasName = parser->currentToken.value;

    ASTNode* alias = createIdentifierNode(aliasName);
    if (alias) alias->line = parser->currentToken.line;
    advance(parser); // consume identifier

    parserConsumeArraySuffixes(parser, &baseType, NULL);

    size_t typedefAttrCount = 0;
    ASTAttribute** typedefAttrs = parserParseAttributeSpecifiers(parser, &typedefAttrCount);

    if (parser->currentToken.type != TOKEN_SEMICOLON) {
        printParseError("Expected ';' after typedef", parser);
        return NULL;
    }
    advance(parser); // consume ';'

    // Phase 2 hook: remember typedef name for later disambiguation
    if (parser->ctx && aliasName && aliasName[0]) {
        cc_add_typedef(parser->ctx, aliasName);
    }

    ASTNode* node = createTypedefNode(baseType, alias);
    if (node) {
        astNodeCloneTypeAttributes(node, &baseType);
        astNodeAppendAttributes(node, typedefAttrs, typedefAttrCount);
        typedefAttrs = NULL;
        typedefAttrCount = 0;
    } else {
        astAttributeListDestroy(typedefAttrs, typedefAttrCount);
        free(typedefAttrs);
    }
    return node;
}




ASTNode* handleStructStatements(Parser* parser) {
    Token next = peekNextToken(parser);
    Token after = peekTwoTokensAhead(parser);
    Token third = peekThreeTokensAhead(parser);
    
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
     
    // Case 5: Anonymous struct definition
    if (next.type == TOKEN_LBRACE) {
        return parseStructDefinition(parser);
    }
    
    // Fallback: treat as a normal declaration
    return handleTypeOrFunctionDeclaration(parser);
}
