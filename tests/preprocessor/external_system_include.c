#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    size_t size;
    uint32_t flags;
    bool enabled;
} TestConfig;

int use_system_headers(TestConfig* cfg) {
    if (!cfg) return 0;
    uintptr_t raw = (uintptr_t)cfg;
    cfg->flags = (uint32_t)(raw & 0xFFFFu);
    cfg->enabled = (cfg->flags != 0);
    return cfg->enabled ? 1 : 0;
}
