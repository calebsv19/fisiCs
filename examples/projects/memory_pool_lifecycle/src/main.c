#include <stdio.h>

#include "memory_pool_lifecycle.h"

void __fisics_memcheck_report(void);

int main(void) {
    MemoryPoolDemo demo = {0};

    if (!memory_pool_demo_acquire(&demo, 3, 2)) {
        return 2;
    }

    printf("tick energy events\n");
    for (int tick = 0; tick < 2; ++tick) {
        memory_pool_demo_tick(&demo, tick);
        printf("%d %d %d\n",
               tick,
               memory_pool_demo_total_energy(&demo),
               memory_pool_demo_total_events(&demo));
    }

    memory_pool_demo_release_all(&demo);
    __fisics_memcheck_report();
    return 0;
}
