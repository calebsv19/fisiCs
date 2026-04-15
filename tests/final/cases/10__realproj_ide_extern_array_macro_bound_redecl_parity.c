// Regression: array-bound redeclarations must be compatible when the bound
// expressions are equivalent constant expressions.
#define MAX_SELECTION_LENGTH (1024 * 1024)

extern char selectionBuffer[MAX_SELECTION_LENGTH];
char selectionBuffer[MAX_SELECTION_LENGTH] = {0};

int first_selection_char(void) {
    return selectionBuffer[0];
}
