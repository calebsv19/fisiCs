#include <stdio.h>

int abi_cv_reduce(const int* xs, int n, int bias);
int abi_cv_window(const int* xs, int n, int base, int width);

int main(void) {
    const int xs[7] = {5, 1, 9, 2, 6, 5, 3};
    int a = abi_cv_reduce(xs, 7, 4);
    int b = abi_cv_window(xs, 7, 1, 4);
    int c = abi_cv_window(xs, 7, 2, 3);
    printf("%d %d %d\n", a, b, c);
    return 0;
}
