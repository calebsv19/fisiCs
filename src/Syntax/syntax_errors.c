#include "syntax_errors.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_ERROR_CAPACITY 64

static ErrorList errorList;

static SourceRange empty_range(void) {
    SourceRange r;
    r.start.file = NULL;
    r.start.line = 0;
    r.start.column = 0;
    r.end = r.start;
    return r;
}

void initErrorList(void) {
    errorList.errors = malloc(sizeof(SyntaxError) * INITIAL_ERROR_CAPACITY);
    errorList.count = 0;
    errorList.capacity = INITIAL_ERROR_CAPACITY;
    errorList.errorCount = 0;
}

void addError(int line, int column, const char* message, const char* hint) {
    if (errorList.count >= errorList.capacity) {
        errorList.capacity *= 2;
        errorList.errors = realloc(errorList.errors, sizeof(SyntaxError) * errorList.capacity);
    }

    SyntaxError* err = &errorList.errors[errorList.count++];
    err->line = line;
    err->column = column;
    err->message = strdup(message);
    err->hint = hint ? strdup(hint) : NULL;
    err->isWarning = false;
    err->spelling = empty_range();
    err->macroCallSite = empty_range();
    err->macroDefinition = empty_range();
    errorList.errorCount++;
}

void addWarning(int line, int column, const char* message, const char* hint) {
    if (errorList.count >= errorList.capacity) {
        errorList.capacity *= 2;
        errorList.errors = realloc(errorList.errors, sizeof(SyntaxError) * errorList.capacity);
    }

    SyntaxError* err = &errorList.errors[errorList.count++];
    err->line = line;
    err->column = column;
    err->message = strdup(message);
    err->hint = hint ? strdup(hint) : NULL;
    err->isWarning = true;
    err->spelling = empty_range();
    err->macroCallSite = empty_range();
    err->macroDefinition = empty_range();
}

void addErrorFromToken(const Token* tok, const char* message, const char* hint) {
    int line = tok ? tok->line : 0;
    int col = (tok && tok->location.start.column) ? tok->location.start.column : 0;
    addError(line, col, message, hint);
    if (errorList.count > 0 && tok) {
        SyntaxError* err = &errorList.errors[errorList.count - 1];
        err->spelling = tok->location;
        err->macroCallSite = tok->macroCallSite;
        err->macroDefinition = tok->macroDefinition;
    }
}

void addErrorWithRanges(SourceRange spelling,
                        SourceRange macroCallSite,
                        SourceRange macroDefinition,
                        const char* message,
                        const char* hint) {
    int line = spelling.start.line;
    int col = spelling.start.column;
    addError(line, col, message, hint);
    if (errorList.count > 0) {
        SyntaxError* err = &errorList.errors[errorList.count - 1];
        err->spelling = spelling;
        err->macroCallSite = macroCallSite;
        err->macroDefinition = macroDefinition;
    }
}

void reportErrors(void) {
    for (size_t i = 0; i < errorList.count; i++) {
        SyntaxError err = errorList.errors[i];
        printf("%s at (%d:%d): %s\n", err.isWarning ? "Warning" : "Error", err.line, err.column, err.message);
        if (err.hint) {
            printf("   Hint: %s\n", err.hint);
        }
        if (err.spelling.start.file) {
            printf("   Spelling: %s:%d:%d\n",
                   err.spelling.start.file,
                   err.spelling.start.line,
                   err.spelling.start.column);
        }
        if (err.macroCallSite.start.file) {
            printf("   Macro call: %s:%d:%d\n",
                   err.macroCallSite.start.file,
                   err.macroCallSite.start.line,
                   err.macroCallSite.start.column);
        }
        if (err.macroDefinition.start.file) {
            printf("   Macro definition: %s:%d:%d\n",
                   err.macroDefinition.start.file,
                   err.macroDefinition.start.line,
                   err.macroDefinition.start.column);
        }
    }
}

size_t getErrorCount(void) {
    return errorList.errorCount;
}

void freeErrorList(void) {
    for (size_t i = 0; i < errorList.count; i++) {
        free(errorList.errors[i].message);
        free(errorList.errors[i].hint);
    }
    free(errorList.errors);
    errorList.errors = NULL;
    errorList.count = 0;
    errorList.capacity = 0;
    errorList.errorCount = 0;
}
#include "Lexer/tokens.h"
