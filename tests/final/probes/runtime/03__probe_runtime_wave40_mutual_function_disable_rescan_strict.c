#include <stdio.h>

#define W40_MUTUAL_STR_RAW(x) #x
#define W40_MUTUAL_STR(x) W40_MUTUAL_STR_RAW(x)
#define W40_MUTUAL_A(x) x + W40_MUTUAL_B
#define W40_MUTUAL_B(x) x + W40_MUTUAL_A

int main(void) {
    puts(W40_MUTUAL_STR(W40_MUTUAL_A(1)(2)(3)));
    return 0;
}
