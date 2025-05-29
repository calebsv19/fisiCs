#ifndef SYNTAX_ERRORS_H
#define SYNTAX_ERRORS_H

#include <stddef.h>

typedef struct {
    int line;
    int column;
    char* message;
    char* hint;
} SyntaxError;

typedef struct {
    SyntaxError* errors;
    size_t count;
    size_t capacity;
} ErrorList;

void initErrorList(void);
void addError(int line, int column, const char* message, const char* hint);
void reportErrors(void);
void freeErrorList(void);

#endif // SYNTAX_ERRORS_H

