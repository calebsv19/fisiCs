#include <stdio.h>

static int wave87_nested_fixed_case(int selector) {
    switch (selector) {
        case 0: {
            int row[4];
            row[0] = 10;
            case 1:
                return 10;
        }
        default:
            return 0;
    }
}

int main(void) {
    printf("%d\n", wave87_nested_fixed_case(1));
    return 0;
}
