#ifndef MEMORY_POOL_LIFECYCLE_H
#define MEMORY_POOL_LIFECYCLE_H

#include <stddef.h>

typedef struct DemoEntity {
    int id;
    int energy;
} DemoEntity;

typedef struct MemoryPoolDemo {
    DemoEntity* entities;
    int* event_counts;
    size_t entity_count;
    size_t event_count;
} MemoryPoolDemo;

int memory_pool_demo_acquire(MemoryPoolDemo* demo,
                             size_t entity_count,
                             size_t event_count);
void memory_pool_demo_tick(MemoryPoolDemo* demo, int tick);
int memory_pool_demo_total_energy(const MemoryPoolDemo* demo);
int memory_pool_demo_total_events(const MemoryPoolDemo* demo);
void memory_pool_demo_release_entities(MemoryPoolDemo* demo);
void memory_pool_demo_release_events(MemoryPoolDemo* demo);
void memory_pool_demo_release_all(MemoryPoolDemo* demo);

#endif
