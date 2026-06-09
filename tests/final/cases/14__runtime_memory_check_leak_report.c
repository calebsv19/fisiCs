#include <stdlib.h>

void __fisics_memcheck_report(void);

int main(void) {
    void* p = malloc(13);
    if (!p) return 2;
    __fisics_memcheck_report();
    return 0;
}
