#include <stdalign.h>
#include <stddef.h>
#include <stdio.h>

int main(void) {
    struct Wave15BridgeA {
        char c;
        int i;
    };

    struct Wave15BridgeB {
        char c;
        long double ld;
    };

    printf(
        "%llu %llu %llu %llu %llu %d %d\n",
        (unsigned long long)_Alignof(struct Wave15BridgeA),
        (unsigned long long)_Alignof(struct Wave15BridgeB),
        (unsigned long long)_Alignof(int),
        (unsigned long long)_Alignof(long double),
        (unsigned long long)offsetof(struct Wave15BridgeB, ld),
        __alignas_is_defined,
        __alignof_is_defined
    );
    return 0;
}
