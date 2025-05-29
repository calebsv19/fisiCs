#include "parser.h"
#include "parser_main.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

ASTNode* parse(Parser* parser) {			// root parsing method (modify for debug)
	return parseProgram(parser);
}

//------------------------------------------------------------------------------------
// 		MAIN BLOCKS


// 		MAIN BLOCKS
// ---------------------------------------------------------------------------------------------
// 		PREPROCESSORS

// ---------------------------------------------------------------------------------------------
//              EXPRESSIONS


//		EXPRESSIONS
// ---------------------------------------------------------------------------------------------
//		ASSIGNMENT & DECLARATION	






//              ASSIGNMENT & DECLARATION
// -------------------------------------------------------------------------------------------
//              FUNCTIONS



//		FUNCTIONS		
// ---------------------------------------------------------------------------------------------
//              SWITCH PARSING

//              SWITCH PARSING
// ---------------------------------------------------------------------------------------------
//              ARRAY PARSING


//              ARRAY PARSING
// ---------------------------------------------------------------------------------------------
//              JUMP LOCATIONS




//  		JUMP LOCATIONS
// ---------------------------------------------------------------------------------------------
// 		TESTS AND LOOPS


//		TEST AND LOOPS
// ---------------------------------------------------------------------------------------------
// 		HELPER PARSERS



// 		HELPER PARSERS
// --------------------------------------------------------------------------------
// 		LOOKS LIKE HELPERS

/*
bool looksLikeTypeDeclaration(Parser* parser) {
    Parser temp = *parser;
    Lexer* clonedLexer = cloneLexer(parser->lexer);
    temp.lexer = clonedLexer;
 
    // Reset token queue so you always use getNextToken() with clonedLexer
    temp.currentToken = getNextToken(clonedLexer);
    temp.nextToken = (Token){0};
    temp.nextNextToken = (Token){0};
    temp.nextNextNextToken = (Token){0};


    // Accept storage specifiers and modifiers
    while (isStorageSpecifier(temp.currentToken.type) || isModifierToken(temp.currentToken.type)) {
        advanceParserWithLexer(&temp, clonedLexer); 
    }

    // Accept primitive types
    if (isPrimitiveTypeToken(temp.currentToken.type)) {
        advanceParserWithLexer(&temp, clonedLexer);
    }
    // Accept user-defined types
    else if (temp.currentToken.type == TOKEN_IDENTIFIER) {
        advanceParserWithLexer(&temp, clonedLexer);
    }
    // Accept struct/union/enum types
    else if (temp.currentToken.type == TOKEN_STRUCT ||   
             temp.currentToken.type == TOKEN_UNION  ||
             temp.currentToken.type == TOKEN_ENUM) {
        advanceParserWithLexer(&temp, clonedLexer);
        if (temp.currentToken.type == TOKEN_IDENTIFIER) {
            advanceParserWithLexer(&temp, clonedLexer);
        } else {
            free(clonedLexer);
            return false; // invalid struct/union without name
        }
    } else {
        free(clonedLexer);
        return false; // didn't find a recognizable type
    }

    // Accept optional pointer(s)
    while (temp.currentToken.type == TOKEN_ASTERISK) {
        advanceParserWithLexer(&temp, clonedLexer);
    }

    // Final check: must be followed by an identifier
    bool result = (temp.currentToken.type == TOKEN_IDENTIFIER);

    printf("bool result: %d\n", result);

    free(clonedLexer);
    return result;
}
*/




// 		LOOKS LIKE HELPERS
// -----------------------------------------------
// 		BASIC HELPERS


