#include <limits.h>
int main(void) {
    int ok = (INT_MAX > 0) && (CHAR_BIT == 8);
    return ok ? 0 : 1;
}
