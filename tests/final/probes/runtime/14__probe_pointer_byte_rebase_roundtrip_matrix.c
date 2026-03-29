#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

int main(void) {
    int a[6] = {11, 22, 33, 44, 55, 66};
    char* bytes = (char*)(void*)a;

    int* p2 = (int*)(void*)(bytes + (ptrdiff_t)(2 * (int)sizeof(int)));
    int* p5 = (int*)(void*)(bytes + (ptrdiff_t)(5 * (int)sizeof(int)));
    uintptr_t u2 = (uintptr_t)(void*)p2;
    uintptr_t u5 = (uintptr_t)(void*)p5;

    ptrdiff_t elem = p5 - p2;
    uintptr_t byte_delta = u5 - u2;
    int back = *(int*)(void*)(bytes + (ptrdiff_t)(elem * (ptrdiff_t)sizeof(int)));

    printf("%d %d %td %llu %d\n", *p2, *p5, elem, (unsigned long long)byte_delta, back);
    return 0;
}
