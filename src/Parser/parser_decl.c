#include "parser_decl.h"
#include "parser_helpers.h"
#include "parser_lookahead.h"
#include "parser_func.h"
#include "parser_array.h"
#include "parser_expr.h"

ASTNode* handleTypeOrFunctionDeclaration(Parser* parser) {
    printf("DEBUG: Entered handleTypeOrFunctionDeclaration()\n");

    // --- Case 0: Function Pointer Declaration (before parseType) ---
    if (looksLikeFunctionPointerDeclaration(parser)) {
        printf("DEBUG: Detected function pointer declaration via lookahead\n");
        return parseFunctionPointerDeclaration(parser);
    }

    // Normal type-based parsing follows
    ParsedType parsedType = parseType(parser);
    if (parsedType.kind == TYPE_INVALID) {
        printParseError("Invalid type in declaration", parser);
        return NULL;
    }

    // After type, we expect an identifier
    if (parser->currentToken.type != TOKEN_IDENTIFIER) {
        printParseError("Expected identifier after type", parser);
        return NULL;
    }

    Token identifier = parser->currentToken;
    Token next = peekNextToken(parser);
    Token after = peekTwoTokensAhead(parser);

    // --- Case 1: Function (definition or declaration) ---
    if (next.type == TOKEN_LPAREN) {
        printf("DEBUG: Detected function declaration or definition\n");
        return parseFunctionDefinition(parser, parsedType);
    }

    // --- Case 2: Array Declaration ---
    if (next.type == TOKEN_LBRACKET &&
        (after.type == TOKEN_NUMBER || after.type == TOKEN_RBRACKET)) {
        printf("DEBUG: Detected array declaration\n");
        return parseArrayDeclaration(parser, parsedType);
    }

    // --- Case 3: Variable Declaration (with optional init or commas) ---
    if (next.type == TOKEN_ASSIGN || next.type == TOKEN_SEMICOLON || next.type == TOKEN_COMMA) {
        printf("DEBUG: Detected variable declaration\n");
        size_t varCount = 0;
        return parseVariableDeclaration(parser, parsedType, &varCount);
    }

    // --- Fallback ---
    printParseError("Unrecognized declaration or function definition pattern", parser);
    return NULL;
}


ASTNode* parseDeclaration(Parser* parser, ParsedType declaredType, size_t* varCount) {
    printf("DEBUG: Entering parseDeclaration() at line %d with token '%s'\n",
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
    
    if (!varNames || !initializers) {
        printf("Error: Memory allocation failed for variable declaration.\n");
        return NULL;
    }
     
    while (1) {
        // Expect variable name
        if (parser->currentToken.type != TOKEN_IDENTIFIER) {
            printf("Error: expected variable name at line %d\n", parser->currentToken.line);
            free(varNames);
            free(initializers);
            return NULL;
        }
         
        ASTNode* varName = createIdentifierNode(parser->currentToken.value);
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
         
        varNames[varCount] = varName;
        initializers[varCount] = init;
        varCount++;
        
        // Expand if needed
        if (varCount >= varCapacity) {
            varCapacity *= 2;
            varNames = realloc(varNames, varCapacity * sizeof(ASTNode*));
            initializers = realloc(initializers, varCapacity * sizeof(DesignatedInit*));
            if (!varNames || !initializers) {
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
        printf("DEBUG: Finished variable declaration, next token: '%s' (line %d)\n",
               parser->currentToken.value, parser->currentToken.line);
        break;
    }
     
    *outVarCount = varCount;
    return createVariableDeclarationNode(declaredType, varNames, initializers, varCount);
}


ASTNode* parseArrayDeclaration(Parser* parser, ParsedType type) {
    // assume token is IDENTIFIER
    ASTNode* name = createIdentifierNode(parser->currentToken.value);
    advance(parser); // consume name
    
    ASTNode* sizeExpr = NULL;
    while (parser->currentToken.type == TOKEN_LBRACKET) {
        ASTNode* dim = parseArraySize(parser); // handles one `[num]`
        sizeExpr = chainArraySizes(sizeExpr, dim);
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
    advance(parser); // consume ';'
    
    return createArrayDeclarationNode(type, name, sizeExpr, initValues, valueCount);
}


ASTNode* parseDeclarationForLoop(Parser* parser) {
    printf("DEBUG: Entering parseDeclarationForLoop() at line %d\n", parser->currentToken.line);
 
    //  Use the proper ParsedType handler
    ParsedType parsedType = parseType(parser);
    if (parsedType.kind == TYPE_INVALID) {
        printf("Error: Invalid type for for-loop initializer at line %d\n", 
				parser->currentToken.line);
        return NULL;
    }       
         
    if (parser->currentToken.type != TOKEN_IDENTIFIER) {
        printf("Error: expected variable name in for-loop initializer at line %d\n",
               parser->currentToken.line);
        return NULL;
    }

    ASTNode* idNode = createIdentifierNode(parser->currentToken.value);
    advance(parser); // Consume identifier
    
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
    DesignatedInit** initializers = malloc(sizeof(ASTNode*));
    
    varNames[0] = idNode;
    initializers[0] = createSimpleInit(initializer);
    
    
    //  Construct variable declaration using ParsedType
    return createVariableDeclarationNode(parsedType, varNames, initializers, varCount);
}


ASTNode* parseUnionDefinition(Parser* parser) {
    if (parser->currentToken.type != TOKEN_UNION) {
        printParseError("Expected 'union'", parser);
        return NULL;
    }
    advance(parser);  // consume 'union'

    char* unionName = NULL;
    if (parser->currentToken.type == TOKEN_IDENTIFIER) {
        unionName = strdup(parser->currentToken.value);
        advance(parser);
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

    return createStructOrUnionDefinitionNode(AST_UNION_DEFINITION, unionName, fields, fieldCount);
}


ASTNode* parseStructDefinition(Parser* parser) {
    printf("    entering struct parsing\n");
    advance(parser);  // consume 'struct'

    char* structName = NULL;
    if (parser->currentToken.type == TOKEN_IDENTIFIER) {
        structName = strdup(parser->currentToken.value);
        advance(parser);  // consume name
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

    // Optional trailing name
    if (!structName && parser->currentToken.type == TOKEN_IDENTIFIER) {
        structName = strdup(parser->currentToken.value);
        advance(parser);
    }

    if (parser->currentToken.type != TOKEN_SEMICOLON) {
        printParseError("Expected ';' after struct definition", parser);
        return NULL;
    }
    advance(parser);  // consume ';'

    return createStructOrUnionDefinitionNode(AST_STRUCT_DEFINITION, structName, fields, fieldCount);
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

        if (parser->currentToken.type != TOKEN_IDENTIFIER) {
            printParseError("Expected field name in struct/union", parser);
            break;
        }

        ASTNode* fieldName = createIdentifierNode(parser->currentToken.value);
        advance(parser); // consume field name

        // Parse array sizes if present
        ASTNode* arraySize = NULL;
        while (parser->currentToken.type == TOKEN_LBRACKET) {
            ASTNode* size = parseArraySize(parser);
            if (!size) break;
            arraySize = chainArraySizes(arraySize, size);
        }

        // Optional bitfield width
        ASTNode* bitFieldWidth = NULL;
        if (parser->currentToken.type == TOKEN_COLON) {
            advance(parser);
            bitFieldWidth = parseAssignmentExpression(parser);
        }

        if (parser->currentToken.type != TOKEN_SEMICOLON) {
            printParseError("Expected ';' after struct/union field", parser);
            break;
        }
        advance(parser); // consume ';'

        ASTNode* fieldDecl = NULL;
        if (arraySize) {
            fieldDecl = createArrayDeclarationNode(type, fieldName, arraySize, NULL, 0);
        } else {
            ASTNode** names = malloc(sizeof(ASTNode*));
            DesignatedInit** initializers = malloc(sizeof(DesignatedInit*));
            names[0] = fieldName;
            initializers[0] = NULL;
            fieldDecl = createVariableDeclarationNode(type, names, initializers, 1);
        }

        fieldDecl->varDecl.bitFieldWidth = bitFieldWidth;

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
            
    if (parser->currentToken.type != TOKEN_IDENTIFIER) {
        printParseError("Expected enum name after 'enum'", parser);
        return NULL;
    }
        
    char* enumName = parser->currentToken.value;
    advance(parser); // consume name
        
    if (parser->currentToken.type != TOKEN_LBRACE) {   
        printParseError("Expected '{' to begin enum body", parser);
        return NULL;
    }
        
    advance(parser); // consume '{'
            
    ASTNode** members = NULL;
    ASTNode** values = NULL;
    size_t count = 0, capacity = 4;
    members = malloc(capacity * sizeof(ASTNode*));  
    values = malloc(capacity * sizeof(ASTNode*));
        
     
    printf("entering while loop");
    while (parser->currentToken.type == TOKEN_IDENTIFIER) {
        ASTNode* memberName = createIdentifierNode(parser->currentToken.value);
        advance(parser); // consume member name
        
        ASTNode* valueExpr = NULL;
        if (parser->currentToken.type == TOKEN_ASSIGN) {
            advance(parser); // consume '='
     
            //  Carefully parse expression and check result
            valueExpr = parseAssignmentExpression(parser);
            if (!valueExpr) {
                printParseError("Invalid value expression in enum", parser);
                return NULL;
            }
        }
        
        if (count >= capacity) {
            capacity *= 2;
            members = realloc(members, capacity * sizeof(ASTNode*));
            values = realloc(values, capacity * sizeof(ASTNode*));
        }
    
        members[count] = memberName;
        values[count] = valueExpr;
        count++;
     
        if (parser->currentToken.type == TOKEN_COMMA) {
            advance(parser); // consume and continue
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
    if (parser->currentToken.type == TOKEN_SEMICOLON) {
        advance(parser); // consume trailing ';' after enum
    } else {
        printParseError("Expected ';' after enum declaration", parser);
        return NULL;
    }
     
    return createEnumDefinitionNode(enumName, members, values, count);
}



ASTNode* parseTypedef(Parser* parser) {
    if (parser->currentToken.type != TOKEN_TYPEDEF) {
        printParseError("Expected 'typedef'", parser);
        return NULL;
    }
     
    advance(parser); // consume 'typedef'
    
    ParsedType baseType = parseType(parser);
    
    if (parser->currentToken.type != TOKEN_IDENTIFIER) {
        printParseError("Expected identifier name for typedef", parser);
        return NULL;
    }
     
    ASTNode* alias = createIdentifierNode(parser->currentToken.value);
    advance(parser); // consume identifier
    
    if (parser->currentToken.type != TOKEN_SEMICOLON) {
        printParseError("Expected ';' after typedef", parser);
        return NULL;
    }
     
    advance(parser); // consume ';'
    
    // TODO: Add to symbol table later
    return createTypedefNode(baseType, alias);
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
        printf("DEBUG: Routing struct return function declaration to handleTypeOrFunctionDeclaration");
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
        advance(parser); // Foo
        advance(parser); // ;
        return NULL;
    }
     
    // Case 5: Anonymous struct definition
    if (next.type == TOKEN_LBRACE) {
        return parseStructDefinition(parser);
    }
    
    // Unrecognized pattern
    printParseError("Invalid use of 'struct'", parser);
    return NULL;
}





ParsedType parseType(Parser* parser) {
    ParsedType type = {
        .kind = TYPE_INVALID,
        .primitiveType = TOKEN_VOID,
        .userTypeName = NULL,
            
        .isConst = false,
        .isSigned = false,
        .isUnsigned = false,
        .isShort = false,
        .isLong = false,
        .isVolatile = false,
        .isRestrict = false,
        .isInline = false,
        
        .isStatic = false,
        .isExtern = false,
        .isRegister = false,
        .isAuto = false,
    
        .pointerDepth = 0
    };
        
    // Handle storage class specifiers first
    while (true) {
        switch (parser->currentToken.type) {
            case TOKEN_STATIC:
                type.isStatic = true; 
                advance(parser);
                break;
            case TOKEN_EXTERN:
                type.isExtern = true;
                advance(parser);
                break;
            case TOKEN_REGISTER:
                type.isRegister = true;
                advance(parser);
                break;  
            case TOKEN_AUTO:
                type.isAuto = true;
                advance(parser);
                break;
            default:
                goto modifiers;
        }
    }
        
modifiers:
    // Now parse other modifiers and qualifiers
    while (true) {
        switch (parser->currentToken.type) {
            case TOKEN_CONST:
                type.isConst = true;
                advance(parser);
                break;
            case TOKEN_SIGNED:
                type.isSigned = true;
                advance(parser);
                break;
            case TOKEN_UNSIGNED:
                type.isUnsigned = true;
                advance(parser);
                break;
            case TOKEN_SHORT:
                type.isShort = true;
                advance(parser);
                break;
            case TOKEN_LONG:
                type.isLong = true;
                advance(parser);   
                break;
            case TOKEN_VOLATILE:
                type.isVolatile = true;
                advance(parser);
                break;
            case TOKEN_RESTRICT:
                type.isRestrict = true;
                advance(parser);
                break;
            case TOKEN_INLINE:
                type.isInline = true;
                advance(parser);
                break;
            default:  
                goto type_name;
        }
    }

type_name:

    // === Handle primitive types or modifiers ===
    if (isPrimitiveTypeToken(parser->currentToken.type)) {
        type.kind = TYPE_PRIMITIVE;
        type.primitiveType = parser->currentToken.type;
        advance(parser);
    }
     
    // === Handle struct types ===
    else if (parser->currentToken.type == TOKEN_STRUCT) {
        advance(parser); // consume 'struct'
        if (parser->currentToken.type != TOKEN_IDENTIFIER) {
            printParseError("Expected identifier after 'struct'", parser);
            type.kind = TYPE_INVALID;
            return type;
        }
        type.kind = TYPE_USER_DEFINED;
        type.userTypeName = strdup(parser->currentToken.value);
        type.tag = TAG_STRUCT;
        advance(parser);
    }
     
    // === Handle user-defined types (typedefs or unknown types) ===
    else if (parser->currentToken.type == TOKEN_IDENTIFIER) {
        type.kind = TYPE_USER_DEFINED;
        type.userTypeName = strdup(parser->currentToken.value);
        advance(parser);
    }
     
    // === No explicit base type, but modifiers imply "int" ===
    else if (type.isUnsigned || type.isSigned || type.isShort || type.isLong) {
        type.kind = TYPE_PRIMITIVE;
        type.primitiveType = TOKEN_INT;
        // Do not advance — no base type token present
    }
     
    // === Error case ===
    else {
        printParseError("Expected type name (after modifiers)", parser);
        return type;
    }
     
    // === Parse pointer depth ===
    while (parser->currentToken.type == TOKEN_ASTERISK) {
        type.pointerDepth += 1;
        advance(parser);
    }
     
    return type;
}
