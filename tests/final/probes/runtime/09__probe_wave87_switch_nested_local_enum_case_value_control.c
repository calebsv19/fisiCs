#include <stdio.h>

static int wave87_nested_local_enum_case(int selector) {
    switch (selector) {
        case 0: {
            enum { WAVE87_LOCAL_CASE = 2 };
            case WAVE87_LOCAL_CASE:
                return 23;
        }
        default:
            return 0;
    }
}

int main(void) {
    printf("%d\n", wave87_nested_local_enum_case(2));
    return 0;
}
