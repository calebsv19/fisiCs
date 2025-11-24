#include "syntax_errors.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_ERROR_CAPACITY 64

static ErrorList errorList;

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
}

void reportErrors(void) {
    for (size_t i = 0; i < errorList.count; i++) {
        SyntaxError err = errorList.errors[i];
        printf("%s at (%d:%d): %s\n", err.isWarning ? "Warning" : "Error", err.line, err.column, err.message);
        if (err.hint) {
            printf("   Hint: %s\n", err.hint);
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
