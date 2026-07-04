#include "memory_pool_lifecycle.h"

#include <stdlib.h>

int memory_pool_demo_acquire(MemoryPoolDemo* demo,
                             size_t entity_count,
                             size_t event_count) {
    size_t i;

    demo->entities = (DemoEntity*)malloc(sizeof(DemoEntity) * entity_count);
    demo->event_counts = (int*)calloc(event_count, sizeof(int));
    demo->entity_count = entity_count;
    demo->event_count = event_count;

    if (!demo->entities || !demo->event_counts) {
        memory_pool_demo_release_all(demo);
        return 0;
    }

    for (i = 0; i < entity_count; ++i) {
        demo->entities[i].id = (int)i + 1;
        demo->entities[i].energy = 10 + (int)i;
    }

    return 1;
}

void memory_pool_demo_tick(MemoryPoolDemo* demo, int tick) {
    size_t i;

    for (i = 0; i < demo->entity_count; ++i) {
        demo->entities[i].energy += tick + (int)i;
    }
    for (i = 0; i < demo->event_count; ++i) {
        demo->event_counts[i] += tick + 1;
    }
}

int memory_pool_demo_total_energy(const MemoryPoolDemo* demo) {
    int total = 0;
    size_t i;

    for (i = 0; i < demo->entity_count; ++i) {
        total += demo->entities[i].energy;
    }
    return total;
}

int memory_pool_demo_total_events(const MemoryPoolDemo* demo) {
    int total = 0;
    size_t i;

    for (i = 0; i < demo->event_count; ++i) {
        total += demo->event_counts[i];
    }
    return total;
}

void memory_pool_demo_release_entities(MemoryPoolDemo* demo) {
    free(demo->entities);
    demo->entities = NULL;
    demo->entity_count = 0;
}

void memory_pool_demo_release_events(MemoryPoolDemo* demo) {
    free(demo->event_counts);
    demo->event_counts = NULL;
    demo->event_count = 0;
}

void memory_pool_demo_release_all(MemoryPoolDemo* demo) {
    memory_pool_demo_release_events(demo);
    memory_pool_demo_release_entities(demo);
}
