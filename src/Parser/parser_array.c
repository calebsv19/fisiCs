// SPDX-License-Identifier: Apache-2.0

#include "parser_array.h"
#include "Parser/Helpers/parser_helpers.h"
#include "Parser/Expr/parser_expr.h"
#include "Parser/Expr/parser_expr_pratt.h"
#include <string.h>


DesignatedInit** parseInitializerList(Parser* parser, ParsedType type, size_t* outCount) {
    if (parser->currentToken.type == TOKEN_LBRACE) {
        if (parser->nextToken.type == TOKEN_DOT) {
            return parseStructInitializer(parser, type, outCount);
        }
        if (parser->nextToken.type == TOKEN_LBRACKET) {
            return parseArrayInitializer(parser, type, outCount);
        }
    }
    if (type.kind == TYPE_STRUCT || type.kind == TYPE_UNION || type.kind == TYPE_NAMED) {
        // Struct initializer with known parent type
        return parseStructInitializer(parser, type, outCount);
    } else if (parsedTypeIsDirectArray(&type) || type.kind == TYPE_PRIMITIVE) {
        // Array initializer — pass parent type as fallback
        return parseArrayInitializer(parser, type, outCount);
    } else {
        // Unknown or fallback — assume array style
        return parseArrayInitializer(parser, type, outCount);
    }
}
 
 
// Array parsing
ASTNode* parseArraySize(Parser* parser, bool* outIsVLA, ParsedArrayInfo* outInfo) {
    if (outInfo) {
        memset(outInfo, 0, sizeof(*outInfo));
    }
    if (parser->currentToken.type == TOKEN_LBRACKET) { //  Check for `[`
        advance(parser); // Consume `[`

        bool scanning = true;
        while (scanning) {
            switch (parser->currentToken.type) {
                case TOKEN_STATIC:
                    if (outInfo) outInfo->hasStatic = true;
                    advance(parser);
                    break;
                case TOKEN_CONST:
                    if (outInfo) outInfo->qualConst = true;
                    advance(parser);
                    break;
                case TOKEN_VOLATILE:
                    if (outInfo) outInfo->qualVolatile = true;
                    advance(parser);
                    break;
                case TOKEN_RESTRICT:
                    if (outInfo) outInfo->qualRestrict = true;
                    advance(parser);
                    break;
                default:
                    scanning = false;
                    break;
            }
        }

        if (parser->currentToken.type == TOKEN_ASTERISK) {
            if (outInfo && outInfo->hasStatic) {
                printParseError("Array parameter 'static' requires a size expression", parser);
                return NULL;
            }
            advance(parser);
            if (parser->currentToken.type != TOKEN_RBRACKET) {
                printf("Error: expected ']' after '*' array size at line %d\n", parser->currentToken.line);
                return NULL;
            }
            advance(parser);
            if (outIsVLA) *outIsVLA = true;
            if (outInfo) outInfo->isVLA = true;
            return NULL;
        }

        if (parser->currentToken.type == TOKEN_RBRACKET) {
            if (outInfo && outInfo->hasStatic) {
                printParseError("Array parameter 'static' requires a size expression", parser);
                return NULL;
            }
            advance(parser); // consume `]`
            if (outIsVLA) *outIsVLA = false;
            if (outInfo) outInfo->isVLA = false;
            return NULL; // This means: array size is unspecified
        }
         
        ASTNode* size = parseAssignmentExpression(parser); //  Parse array size
        if (!size) {
            printf("Error: Invalid array size at line %d\n", parser->currentToken.line);
            return NULL;
        }
        if (parser->currentToken.type != TOKEN_RBRACKET) {
            printf("Error: expected ']' after array size at line %d\n", parser->currentToken.line);
            return NULL;
        }
        advance(parser); //  Consume `]`
        if (outIsVLA) *outIsVLA = false;
        if (outInfo) {
            outInfo->isVLA = false;
            outInfo->sizeExpr = size;
        }
        return size;
    }
    return NULL;
}

ASTNode* parseArrayAccess(Parser* parser, ASTNode* baseArray) {
    // Only proceed if the next token starts an index
    if (parser->currentToken.type != TOKEN_LBRACKET) {
        return baseArray;
    }

    do {
        advance(parser); // consume '['

        // Use Pratt for the index expression (full precedence, commas handled upstream)
        ASTNode* index = parseExpressionPratt(parser, 0);
        if (!index) {
            printParseError("Invalid array index expression", parser);
            return NULL;
        }

        if (parser->currentToken.type != TOKEN_RBRACKET) {
            printParseError("Expected ']' after array index", parser);
            return NULL;
        }
        advance(parser); // consume ']'

        // Support chained/multi-dimensional access: arr[i][j]...
        baseArray = createArrayAccessNode(baseArray, index);

    } while (parser->currentToken.type == TOKEN_LBRACKET);

    return baseArray;
}


DesignatedInit** parseArrayInitializer(Parser* parser, ParsedType parentType, size_t* valueCount) {
    if (parser->currentToken.type != TOKEN_LBRACE) return NULL;
    
    advance(parser); // Consume `{`
    
    size_t capacity = 4;
    DesignatedInit** values = malloc(capacity * sizeof(DesignatedInit*));
    if (!values) {
        printf("Error: Memory allocation failed for array initializer.\n");
        return NULL;
    }
     
    *valueCount = 0;
    
    while (parser->currentToken.type != TOKEN_RBRACE &&
           parser->currentToken.type != TOKEN_EOF) {   
           
        // Expand array if needed
        if (*valueCount >= capacity) {
            capacity *= 2;
            values = realloc(values, capacity * sizeof(DesignatedInit*));
            if (!values) {
                printf("Error: Memory reallocation failed.\n");
                return NULL;
            }
        }
         
        PARSER_DEBUG_PRINTF("        DEBUG: Parsing array initializer — token '%s' (Type: %d)\n",
               parser->currentToken.value, parser->currentToken.type);
               
        DesignatedInit* init = NULL;
        
        if (parser->currentToken.type == TOKEN_LBRACKET) {
            // [index] = value
            advance(parser); // consume `[`
            ASTNode* indexExpr = parseAssignmentExpression(parser);
            if (!indexExpr) {
                printf("Error: Invalid index expression at line %d\n", parser->currentToken.line);
                free(values);
                return NULL; 
            }
             
            if (parser->currentToken.type != TOKEN_RBRACKET) {
                printf("Error: expected ']' after index at line %d\n", parser->currentToken.line);
                free(values);
                return NULL; 
            }
            advance(parser); // consume `]`

            ASTNode* valueExpr = NULL;

            if (parser->currentToken.type == TOKEN_DOT) {
                advance(parser);
                if (parser->currentToken.type != TOKEN_IDENTIFIER) {
                    printParseError("Expected field name after '.' in array designator", parser);
                    free(values);
                    return NULL;
                }
                char* fieldName = strdup(parser->currentToken.value);
                advance(parser);
                if (parser->currentToken.type != TOKEN_ASSIGN) {
                    printParseError("Expected '=' after field name in array designator", parser);
                    free(values);
                    return NULL;
                }
                advance(parser);
                if (parser->currentToken.type == TOKEN_LBRACE) {
                    size_t nestedCount = 0;
                    DesignatedInit** nested = parseInitializerList(parser, parentType, &nestedCount);
                    if (!nested) {
                        printParseError("Invalid compound initializer after [index].field =", parser);
                        free(values);
                        return NULL;
                    }
                    valueExpr = createCompoundInit(nested, nestedCount);
                } else {
                    valueExpr = parseAssignmentExpression(parser);
                }
                if (!valueExpr) {
                    printf("Error: Invalid value expression at line %d\n", parser->currentToken.line);
                    free(values);
                    return NULL;
                }
                DesignatedInit* fieldInit = createDesignatedInit(fieldName, valueExpr);
                DesignatedInit** nestedEntries = malloc(sizeof(DesignatedInit*));
                if (!nestedEntries) {
                    free(values);
                    return NULL;
                }
                nestedEntries[0] = fieldInit;
                valueExpr = createCompoundInit(nestedEntries, 1);
            } else {
                if (parser->currentToken.type != TOKEN_ASSIGN) {
                    printf("Error: expected '=' after [index] at line %d\n", parser->currentToken.line);
                    free(values);
                    return NULL; 
                }
                advance(parser); // consume '='
            
                if (parser->currentToken.type == TOKEN_LBRACE) {
                    size_t nestedCount = 0;
                    DesignatedInit** nested = parseInitializerList(parser, parentType, &nestedCount);
                    if (!nested) {
                        printf("Error: Invalid compound initializer after [index] = at line %d\n",
                                    parser->currentToken.line);
                        free(values);
                        return NULL; 
                    }
                    valueExpr = createCompoundInit(nested, nestedCount);
                } else {
                    valueExpr = parseAssignmentExpression(parser);
                }
            }
             
            if (!valueExpr) {
                printf("Error: Invalid value expression at line %d\n", parser->currentToken.line);
                free(values);
                return NULL; 
            }
             
            init = createIndexedInit(indexExpr, valueExpr);
            
        } else {
            // Positional value — allow `{ ... }` compound or normal expression
            ASTNode* expr = NULL;
            
            if (parser->currentToken.type == TOKEN_LBRACE) {
                size_t nestedCount = 0;
                DesignatedInit** nested = parseInitializerList(parser, parentType, &nestedCount);
                if (!nested) {
                    printf("Error: Failed to parse nested initializer at line %d\n",
                                parser->currentToken.line);
                    free(values);
                    return NULL; 
                }
                expr = createCompoundInit(nested, nestedCount);
            } else {
                expr = parseAssignmentExpression(parser);
            }
             
            if (!expr) {
                printf("Error: Invalid positional initializer at line %d\n", parser->currentToken.line);
                free(values);
                return NULL; 
            }
             
            init = createSimpleInit(expr);
        }
         
        values[*valueCount] = init;
        (*valueCount)++;
        if (parser->currentToken.type == TOKEN_COMMA) {
            advance(parser); // consume `,`
            continue;
        } else if (parser->currentToken.type == TOKEN_RBRACE) {
            break;
        } else {  
            printf("Error: expected ',' or '}' in array initializer at line %d\n",
                   parser->currentToken.line);
            free(values);
            return NULL; 
        }
    }
     
    if (parser->currentToken.type != TOKEN_RBRACE) {
        printf("Error: expected closing '}' at line %d\n", parser->currentToken.line);
        free(values);
        return NULL; 
    }
     
    advance(parser); // Consume '}'
    return values;
}



DesignatedInit** parseStructInitializer(Parser* parser, ParsedType parentType, size_t* outCount) {
    if (parser->currentToken.type != TOKEN_LBRACE) return NULL;
    advance(parser);  // Consume `{`
    
    size_t capacity = 4;
    *outCount = 0;
    
    DesignatedInit** entries = malloc(capacity * sizeof(DesignatedInit*));
    if (!entries) return NULL;
    
    while (parser->currentToken.type != TOKEN_RBRACE &&
           parser->currentToken.type != TOKEN_EOF) {   
           
        if (*outCount >= capacity) {
            capacity *= 2;
            entries = realloc(entries, capacity * sizeof(DesignatedInit*));
            if (!entries) {
                printf("Error: Memory reallocation failed in parseStructInitializer.\n");
                return NULL;
            }
        }
         
        DesignatedInit* init = NULL;
        
        if (parser->currentToken.type == TOKEN_DOT) {
            size_t fieldCount = 0;
            size_t fieldCap = 4;
            char** fieldNames = malloc(fieldCap * sizeof(char*));
            if (!fieldNames) {
                free(entries);
                return NULL;
            }

            while (parser->currentToken.type == TOKEN_DOT) {
                advance(parser);  // Consume '.'
                if (parser->currentToken.type != TOKEN_IDENTIFIER) {
                    printParseError("Expected field name after '.' in struct initializer", parser);
                    free(fieldNames);
                    free(entries);
                    return NULL;
                }
                if (fieldCount >= fieldCap) {
                    fieldCap *= 2;
                    char** grown = realloc(fieldNames, fieldCap * sizeof(char*));
                    if (!grown) {
                        free(fieldNames);
                        free(entries);
                        return NULL;
                    }
                    fieldNames = grown;
                }
                fieldNames[fieldCount++] = strdup(parser->currentToken.value);
                advance(parser);  // Consume identifier
            }

            if (fieldCount == 0 || parser->currentToken.type != TOKEN_ASSIGN) {
                printParseError("Expected '=' after field name in struct initializer", parser);
                for (size_t i = 0; i < fieldCount; ++i) free(fieldNames[i]);
                free(fieldNames);
                free(entries);
                return NULL;
            }

            advance(parser);  // Consume '='

            ASTNode* expr = NULL;
            ParsedType fieldType = parentType;
            if (parser->currentToken.type == TOKEN_LBRACE) {
                size_t nestedCount = 0;
                DesignatedInit** nested = parseInitializerList(parser, fieldType, &nestedCount);
                if (!nested) {
                    printParseError("Failed to parse nested initializer list", parser);
                    for (size_t i = 0; i < fieldCount; ++i) free(fieldNames[i]);
                    free(fieldNames);
                    free(entries);
                    return NULL;
                }
                expr = createCompoundInit(nested, nestedCount);
            } else {
                expr = parseAssignmentExpression(parser);
            }

            if (!expr) {
                printParseError("Expected expression after '=' in struct initializer", parser);
                for (size_t i = 0; i < fieldCount; ++i) free(fieldNames[i]);
                free(fieldNames);
                free(entries);
                return NULL;
            }

            if (fieldCount == 1) {
                init = createDesignatedInit(fieldNames[0], expr);
            } else {
                ASTNode* nestedExpr = expr;
                for (size_t i = fieldCount; i-- > 1;) {
                    DesignatedInit* fieldInit = createDesignatedInit(fieldNames[i], nestedExpr);
                    DesignatedInit** nestedEntries = malloc(sizeof(DesignatedInit*));
                    if (!fieldInit || !nestedEntries) {
                        for (size_t j = 0; j < fieldCount; ++j) free(fieldNames[j]);
                        free(fieldNames);
                        free(entries);
                        return NULL;
                    }
                    nestedEntries[0] = fieldInit;
                    nestedExpr = createCompoundInit(nestedEntries, 1);
                }
                init = createDesignatedInit(fieldNames[0], nestedExpr);
            }

            for (size_t i = 0; i < fieldCount; ++i) free(fieldNames[i]);
            free(fieldNames);
        } else {
            ASTNode* expr = NULL;
            
            if (parser->currentToken.type == TOKEN_LBRACE) {
                size_t count = 0;
                DesignatedInit** nested = parseInitializerList(parser, parentType, &count);
                if (!nested) {
                    printParseError("Failed to parse nested initializer list", parser);
                    free(entries);
                    return NULL;  
                }
                expr = createCompoundInit(nested, count);
            } else {
                expr = parseAssignmentExpression(parser);
            }
             
            if (!expr) {
                printParseError("Invalid expression in struct initializer", parser);
                free(entries);
                return NULL;  
            }
             
            init = createSimpleInit(expr);
        }
         
        entries[*outCount] = init;
        (*outCount)++;
        
        if (parser->currentToken.type == TOKEN_COMMA) {
            advance(parser);  // Consume `,`
        } else if (parser->currentToken.type != TOKEN_RBRACE) {
            printParseError("Expected ',' or '}' in struct initializer", parser);
            free(entries);
            return NULL;  
        }
    }
    if (parser->currentToken.type != TOKEN_RBRACE) {
        printParseError("Expected closing '}' in struct initializer", parser);
        free(entries);
        return NULL;  
    }
     
    advance(parser);  // Consume `}`
    return entries;
}
