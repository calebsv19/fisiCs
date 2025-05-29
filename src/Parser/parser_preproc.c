#include "parser_preproc.h"
#include "parser_helpers.h"
#include "parser_main.h"

ASTNode* handlePreprocessorDirectives(Parser* parser) {   
    switch (parser->currentToken.type) {
        case TOKEN_INCLUDE:
            return parseIncludeDirective(parser);
        case TOKEN_DEFINE:  
            return parseDefineDirective(parser);
        case TOKEN_IFDEF:
        case TOKEN_IFNDEF:
            return parseConditionalDirective(parser);
        case TOKEN_ENDIF:
            return parseEndifDirective(parser);
        default:
            printParseError("Preprocessor directive", parser);
            return NULL;
    }
}

ASTNode* parseIncludeDirective(Parser* parser) {
    advance(parser);  // Move past the 'include' token
    
    Token token = parser->currentToken;
    bool isSystem = false;
    char buffer[256] = {0};
    
    if (token.type == TOKEN_STRING) {
        // User-defined include: #include "file.h"
        snprintf(buffer, sizeof(buffer), "%s", token.value);
        advance(parser);
    } else if (token.type == TOKEN_LESS) {
        // System include: #include <stdio.h>
        isSystem = true;
        advance(parser);  // Move past '<'
        
        while (parser->currentToken.type != TOKEN_GREATER &&
               parser->currentToken.type != TOKEN_EOF) {
            strncat(buffer, parser->currentToken.value, sizeof(buffer) - strlen(buffer) - 1);
            advance(parser);
        }
         
        if (parser->currentToken.type != TOKEN_GREATER) {
            printParseError("Expected closing '>' for #include directive", parser);
            return NULL;
        }
         
        advance(parser);  // Move past '>'
    } else {
        printParseError("Expected \"filename\" or <filename> after #include", parser);
        return NULL;
    }
     
    return createIncludeDirectiveNode(strdup(buffer), isSystem);
}

ASTNode* parseDefineDirective(Parser* parser) {
    advance(parser);  // Move past the 'define' token
    
    if (parser->currentToken.type != TOKEN_IDENTIFIER) {
        printParseError("Expected macro name after #define", parser);
        return NULL;
    }
     
    char* macroName = strdup(parser->currentToken.value);
    int macroLine = parser->currentToken.line;
    printf("DEBUG: Found macro name '%s' at line %d\n", macroName, macroLine);
    
    advance(parser);  // Move past macro name
    
    // Only collect value if we're still on the same line as the macro name
    if (parser->currentToken.line != macroLine) {
        printf("DEBUG: No macro value found — macro is a flag only.\n");
        return createDefineDirectiveNode(macroName, NULL);
    }
     
    // Otherwise, begin collecting value tokens
    char buffer[1024] = {0};
    size_t len = 0;
    printf("DEBUG: Starting macro value collection on line %d\n", macroLine);
    
    while (parser->currentToken.type != TOKEN_EOF &&
           parser->currentToken.line == macroLine) {
        printf("DEBUG: Adding token '%s' (Type: %d) at line %d to macro value\n",
               parser->currentToken.value,
               parser->currentToken.type, 
               parser->currentToken.line);
        strncat(buffer, parser->currentToken.value, sizeof(buffer) - len - 1);
        len = strlen(buffer);
        advance(parser);
    }
     
    printf("DEBUG: Final macro value = '%s'\n", buffer);
    char* macroValue = strdup(buffer);
    
    return createDefineDirectiveNode(macroName, macroValue);
}

ASTNode* parseConditionalDirective(Parser* parser) {
    bool isNegated = (parser->currentToken.type == TOKEN_IFNDEF);
    advance(parser);  // Consume 'ifdef' or 'ifndef'
    
    if (parser->currentToken.type != TOKEN_IDENTIFIER) {
        printParseError("Expected symbol name after #ifdef/#ifndef", parser);
        return NULL;
    }
     
    char* symbol = strdup(parser->currentToken.value);
    advance(parser);  // Consume the identifier
    
    // Parse the conditional body until #endif
    ASTNode** statements = malloc(sizeof(ASTNode*) * 128);  // TODO: dynamically grow
    size_t count = 0;
    
    while (parser->currentToken.type != TOKEN_ENDIF &&
           parser->currentToken.type != TOKEN_EOF) {  
        ASTNode* stmt = parseStatement(parser);
        if (stmt) {
            statements[count++] = stmt;
        } else {
            break;
        }
    }
     
    if (parser->currentToken.type == TOKEN_ENDIF) {
        advance(parser);  // Consume 'endif'
    } else {
        printParseError("Missing #endif for conditional block", parser);
    }
     
    ASTNode* bodyBlock = createBlockNode(statements, count);
    
    ASTNode* directiveNode = createConditionalDirectiveNode(symbol, isNegated);
    directiveNode->conditionalDirective.body = bodyBlock;
    return directiveNode;
}
 
ASTNode* parseEndifDirective(Parser* parser) {
    advance(parser);  // Move past the 'endif' token
    
    // No extra data needed, just create the node
    ASTNode* node = createEndifDirectiveNode();  
    return node;
}
