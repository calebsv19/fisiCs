#include <stdio.h>

extern int linkage_counter;
void linkage_add(int delta);
int linkage_get(void);

int main(void) {
    linkage_add(5);
    linkage_counter += 2;
    if (linkage_get() != 17) {
        return 1;
    }

    printf("linkage_extern_global_roundtrip_ok counter=%d\n", linkage_get());
    return 0;
}
