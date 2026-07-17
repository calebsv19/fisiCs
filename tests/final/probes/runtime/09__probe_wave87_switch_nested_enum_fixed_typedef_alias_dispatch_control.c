#include <stdio.h>

static int wave87_nested_fixed_typedef_case(int selector) {
    switch (selector) {
        case 0: {
            enum { WAVE87_COLS = 4 };
            typedef int Wave87Row[WAVE87_COLS];
            typedef Wave87Row Wave87Alias;
            case 1:
                return (int)sizeof(Wave87Alias);
        }
        default:
            return 0;
    }
}

int main(void) {
    printf("%d\n", wave87_nested_fixed_typedef_case(1));
    return 0;
}
