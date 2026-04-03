#include <stdio.h>

long long abi_var_forward_scaled(int count, ...);

int main(void) {
    float a = 1.25f;
    float b = 2.0f;
    double c = 3.5;
    printf("%lld\n", abi_var_forward_scaled(3, a, b, c));
    return 0;
}
