#include <stdlib.h>

void __fisics_memcheck_report(void);

int main(void) {
    void* p = malloc(8);
    if (!p) return 2;
    free(0);
    free(p);
    __fisics_memcheck_report();
    return 0;
}
