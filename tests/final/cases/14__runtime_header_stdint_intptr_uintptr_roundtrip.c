#include <stdint.h>
#include <stdio.h>

int main(void) {
    int a[4] = {1, 2, 3, 4};
    uintptr_t p0 = (uintptr_t)(void*)&a[0];
    uintptr_t p3 = (uintptr_t)(void*)&a[3];
    uintptr_t delta = p3 - p0;
    intptr_t sdelta = (intptr_t)p3 - (intptr_t)p0;

    int* r0 = (int*)(void*)p0;
    int* r3 = (int*)(void*)p3;
    int ok = (r0 == &a[0]) && (r3 == &a[3]);

    printf("%llu %lld %d %d\n",
           (unsigned long long)delta,
           (long long)sdelta,
           ok,
           (int)(sizeof(intptr_t) * 8u));
    return 0;
}
