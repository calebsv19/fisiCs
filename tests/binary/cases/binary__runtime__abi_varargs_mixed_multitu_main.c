#include <stdio.h>

long long abi_var_mixed(const char *fmt, ...);

int main(void) {
    float f = 2.5f;
    char c = 3;
    printf("%lld\n", abi_var_mixed("idci", 7, f, c, 9));
    return 0;
}
