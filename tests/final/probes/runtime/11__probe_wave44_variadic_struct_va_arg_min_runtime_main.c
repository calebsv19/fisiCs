#include <stdio.h>

struct Wave44Mini {
    int a;
    int b;
};

int wave44_struct_va_arg_min(int seed, ...);

int main(void) {
    struct Wave44Mini item = { 7, 31 };
    printf("%d\n", wave44_struct_va_arg_min(5, item));
    return 0;
}
