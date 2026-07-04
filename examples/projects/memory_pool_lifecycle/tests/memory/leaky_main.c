#include <stdio.h>

#include "memory_pool_lifecycle.h"

void __fisics_memcheck_report(void);

int main(void) {
    MemoryPoolDemo demo = {0};

    if (!memory_pool_demo_acquire(&demo, 3, 2)) {
        return 2;
    }

    memory_pool_demo_tick(&demo, 0);
    printf("leaky energy=%d events=%d\n",
           memory_pool_demo_total_energy(&demo),
           memory_pool_demo_total_events(&demo));

    memory_pool_demo_release_events(&demo);
    __fisics_memcheck_report();
    return 0;
}
