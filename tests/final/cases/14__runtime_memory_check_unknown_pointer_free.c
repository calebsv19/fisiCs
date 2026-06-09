#include <stdlib.h>

void __fisics_memcheck_report(void);

int main(void) {
    int local = 0;
    free(&local);
    __fisics_memcheck_report();
    return 0;
}
