#include "fisics_memcheck_runtime.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FISICS_MEMCHECK_CAPACITY 262144u

typedef enum FisicsMemcheckSlotState {
    FISICS_MEMCHECK_SLOT_EMPTY = 0,
    FISICS_MEMCHECK_SLOT_ACTIVE = 1,
    FISICS_MEMCHECK_SLOT_FREED = 2
} FisicsMemcheckSlotState;

typedef struct FisicsMemcheckSlot {
    void* ptr;
    size_t size;
    const char* alloc_file;
    int alloc_line;
    FisicsMemcheckSlotState state;
} FisicsMemcheckSlot;

typedef enum FisicsMemcheckReportPolicy {
    FISICS_MEMCHECK_REPORT_ALWAYS = 0,
    FISICS_MEMCHECK_REPORT_ERRORS = 1,
    FISICS_MEMCHECK_REPORT_LEAKS = 2,
    FISICS_MEMCHECK_REPORT_NEVER = 3
} FisicsMemcheckReportPolicy;

static FisicsMemcheckSlot g_slots[FISICS_MEMCHECK_CAPACITY];
static size_t g_active_count;
static size_t g_total_allocs;
static size_t g_total_frees;
static size_t g_double_frees;
static size_t g_unknown_frees;
static size_t g_tracker_failures;
static unsigned long g_generation;
static unsigned long g_last_report_generation;
static int g_atexit_registered;
static int g_policy_initialized;
static FisicsMemcheckReportPolicy g_report_policy = FISICS_MEMCHECK_REPORT_ALWAYS;

static void __fisics_memcheck_report_at_exit(void);
static void emit_report(const char* trigger);

static size_t ptr_hash(const void* ptr) {
    uintptr_t value = (uintptr_t)ptr;
    value ^= value >> 33u;
    value *= (uintptr_t)0xff51afd7ed558ccdULL;
    value ^= value >> 33u;
    return (size_t)value;
}

static int memcheck_streq(const char* lhs, const char* rhs) {
    return lhs && rhs && strcmp(lhs, rhs) == 0;
}

static const char* display_file_name(const char* file) {
    if (!file || file[0] == '\0') {
        return "<unknown>";
    }
    const char* base = file;
    for (const char* p = file; *p; ++p) {
        if (*p == '/' || *p == '\\') {
            base = p + 1;
        }
    }
    return base;
}

static void init_report_policy(void) {
    if (g_policy_initialized) return;
    g_policy_initialized = 1;

    const char* raw = getenv("FISICS_MEMCHECK_REPORT");
    if (!raw || raw[0] == '\0' || memcheck_streq(raw, "always") ||
        memcheck_streq(raw, "auto") || memcheck_streq(raw, "1")) {
        g_report_policy = FISICS_MEMCHECK_REPORT_ALWAYS;
    } else if (memcheck_streq(raw, "errors") || memcheck_streq(raw, "error")) {
        g_report_policy = FISICS_MEMCHECK_REPORT_ERRORS;
    } else if (memcheck_streq(raw, "leaks") || memcheck_streq(raw, "leak")) {
        g_report_policy = FISICS_MEMCHECK_REPORT_LEAKS;
    } else if (memcheck_streq(raw, "never") || memcheck_streq(raw, "manual") ||
               memcheck_streq(raw, "0") || memcheck_streq(raw, "off")) {
        g_report_policy = FISICS_MEMCHECK_REPORT_NEVER;
    } else {
        g_report_policy = FISICS_MEMCHECK_REPORT_ALWAYS;
    }
}

static void ensure_atexit_registered(void) {
    init_report_policy();
    if (g_atexit_registered || g_report_policy == FISICS_MEMCHECK_REPORT_NEVER) {
        return;
    }
    if (atexit(__fisics_memcheck_report_at_exit) == 0) {
        g_atexit_registered = 1;
    } else {
        ++g_tracker_failures;
        ++g_generation;
        fprintf(stderr, "[fisics:memory-check] tracker failure: atexit registration failed\n");
    }
}

static FisicsMemcheckSlot* find_slot(const void* ptr) {
    if (!ptr) return NULL;

    size_t start = ptr_hash(ptr) % FISICS_MEMCHECK_CAPACITY;
    for (size_t i = 0; i < FISICS_MEMCHECK_CAPACITY; ++i) {
        FisicsMemcheckSlot* slot = &g_slots[(start + i) % FISICS_MEMCHECK_CAPACITY];
        if (slot->state == FISICS_MEMCHECK_SLOT_EMPTY) {
            return NULL;
        }
        if (slot->ptr == ptr) {
            return slot;
        }
    }
    return NULL;
}

static FisicsMemcheckSlot* find_insert_slot(const void* ptr) {
    if (!ptr) return NULL;

    size_t start = ptr_hash(ptr) % FISICS_MEMCHECK_CAPACITY;
    FisicsMemcheckSlot* first_freed = NULL;
    for (size_t i = 0; i < FISICS_MEMCHECK_CAPACITY; ++i) {
        FisicsMemcheckSlot* slot = &g_slots[(start + i) % FISICS_MEMCHECK_CAPACITY];
        if (slot->ptr == ptr) {
            return slot;
        }
        if (slot->state == FISICS_MEMCHECK_SLOT_FREED && !first_freed) {
            first_freed = slot;
        }
        if (slot->state == FISICS_MEMCHECK_SLOT_EMPTY) {
            return slot;
        }
    }
    return first_freed;
}

static void track_allocation(void* ptr, size_t size, const char* file, int line) {
    if (!ptr) return;

    FisicsMemcheckSlot* slot = find_insert_slot(ptr);
    if (!slot) {
        ++g_tracker_failures;
        ++g_generation;
        fprintf(stderr, "[fisics:memory-check] tracker failure: allocation table full\n");
        return;
    }

    if (slot->state != FISICS_MEMCHECK_SLOT_ACTIVE) {
        ++g_active_count;
    }
    slot->ptr = ptr;
    slot->size = size;
    slot->alloc_file = file;
    slot->alloc_line = line;
    slot->state = FISICS_MEMCHECK_SLOT_ACTIVE;
    ++g_total_allocs;
    ++g_generation;
}

static void mark_released(FisicsMemcheckSlot* slot) {
    if (!slot || slot->state != FISICS_MEMCHECK_SLOT_ACTIVE) return;
    slot->state = FISICS_MEMCHECK_SLOT_FREED;
    slot->size = 0u;
    slot->alloc_file = NULL;
    slot->alloc_line = 0;
    if (g_active_count > 0u) {
        --g_active_count;
    }
    ++g_total_frees;
    ++g_generation;
}

void* __fisics_memcheck_malloc(size_t size) {
    return __fisics_memcheck_malloc_site(size, NULL, 0);
}

void* __fisics_memcheck_calloc(size_t count, size_t size) {
    return __fisics_memcheck_calloc_site(count, size, NULL, 0);
}

void* __fisics_memcheck_realloc(void* ptr, size_t size) {
    return __fisics_memcheck_realloc_site(ptr, size, NULL, 0);
}

void __fisics_memcheck_free(void* ptr) {
    __fisics_memcheck_free_site(ptr, NULL, 0);
}

void* __fisics_memcheck_malloc_site(size_t size, const char* file, int line) {
    ensure_atexit_registered();
    void* ptr = malloc(size);
    if (ptr) {
        track_allocation(ptr, size, file, line);
    }
    return ptr;
}

void* __fisics_memcheck_calloc_site(size_t count, size_t size, const char* file, int line) {
    ensure_atexit_registered();
    void* ptr = calloc(count, size);
    if (ptr) {
        track_allocation(ptr, count * size, file, line);
    }
    return ptr;
}

void* __fisics_memcheck_realloc_site(void* ptr, size_t size, const char* file, int line) {
    ensure_atexit_registered();
    if (!ptr) {
        void* new_ptr = realloc(NULL, size);
        if (new_ptr) {
            track_allocation(new_ptr, size, file, line);
        }
        return new_ptr;
    }

    FisicsMemcheckSlot* slot = find_slot(ptr);
    if (!slot || slot->state == FISICS_MEMCHECK_SLOT_EMPTY) {
        ++g_unknown_frees;
        ++g_generation;
        fprintf(stderr, "[fisics:memory-check] unknown pointer realloc\n");
        return NULL;
    }
    if (slot->state == FISICS_MEMCHECK_SLOT_FREED) {
        ++g_double_frees;
        ++g_generation;
        fprintf(stderr, "[fisics:memory-check] realloc after free\n");
        return NULL;
    }

    void* new_ptr = realloc(ptr, size);
    if (!new_ptr) {
        return NULL;
    }

    mark_released(slot);
    track_allocation(new_ptr, size, file, line);
    return new_ptr;
}

void __fisics_memcheck_free_site(void* ptr, const char* file, int line) {
    ensure_atexit_registered();
    if (!ptr) {
        return;
    }

    FisicsMemcheckSlot* slot = find_slot(ptr);
    if (!slot || slot->state == FISICS_MEMCHECK_SLOT_EMPTY) {
        ++g_unknown_frees;
        ++g_generation;
        fprintf(stderr, "[fisics:memory-check] unknown pointer free\n");
        if (file && line > 0) {
            fprintf(stderr, "[fisics:memory-check]   free_site=%s:%d\n", display_file_name(file), line);
        }
        return;
    }
    if (slot->state == FISICS_MEMCHECK_SLOT_FREED) {
        ++g_double_frees;
        ++g_generation;
        fprintf(stderr, "[fisics:memory-check] double free\n");
        if (file && line > 0) {
            fprintf(stderr, "[fisics:memory-check]   free_site=%s:%d\n", display_file_name(file), line);
        }
        return;
    }

    mark_released(slot);
    free(ptr);
}

static size_t compute_leaked_bytes(void) {
    size_t leaked_bytes = 0u;
    for (size_t i = 0; i < FISICS_MEMCHECK_CAPACITY; ++i) {
        if (g_slots[i].state == FISICS_MEMCHECK_SLOT_ACTIVE) {
            leaked_bytes += g_slots[i].size;
        }
    }
    return leaked_bytes;
}

static void emit_leak_sites(void) {
    for (size_t i = 0; i < FISICS_MEMCHECK_CAPACITY; ++i) {
        FisicsMemcheckSlot* slot = &g_slots[i];
        if (slot->state != FISICS_MEMCHECK_SLOT_ACTIVE) {
            continue;
        }
        if (slot->alloc_file && slot->alloc_line > 0) {
            fprintf(stderr,
                    "[fisics:memory-check] leak: size=%zu allocated_at=%s:%d\n",
                    slot->size,
                    display_file_name(slot->alloc_file),
                    slot->alloc_line);
        } else {
            fprintf(stderr,
                    "[fisics:memory-check] leak: size=%zu allocated_at=<unknown>\n",
                    slot->size);
        }
    }
}

static void emit_json_string(FILE* fp, const char* value) {
    fputc('"', fp);
    if (value) {
        for (const unsigned char* p = (const unsigned char*)value; *p; ++p) {
            switch (*p) {
                case '"':
                    fputs("\\\"", fp);
                    break;
                case '\\':
                    fputs("\\\\", fp);
                    break;
                case '\b':
                    fputs("\\b", fp);
                    break;
                case '\f':
                    fputs("\\f", fp);
                    break;
                case '\n':
                    fputs("\\n", fp);
                    break;
                case '\r':
                    fputs("\\r", fp);
                    break;
                case '\t':
                    fputs("\\t", fp);
                    break;
                default:
                    if (*p < 0x20u) {
                        fprintf(fp, "\\u%04x", (unsigned int)*p);
                    } else {
                        fputc((int)*p, fp);
                    }
                    break;
            }
        }
    }
    fputc('"', fp);
}

static void emit_json_report(const char* trigger, size_t leaked_bytes) {
    const char* path = getenv("FISICS_MEMCHECK_REPORT_JSON");
    if (!path || path[0] == '\0') {
        return;
    }

    FILE* fp = fopen(path, "w");
    if (!fp) {
        fprintf(stderr, "[fisics:memory-check] failed to write JSON report: %s\n", path);
        return;
    }

    fprintf(fp, "{\n");
    fprintf(fp, "  \"profile\": \"memory_check_report_v1\",\n");
    fprintf(fp, "  \"schema_version\": 1,\n");
    fprintf(fp, "  \"runtime\": \"fisics_memory_check\",\n");
    fprintf(fp, "  \"trigger\": ");
    emit_json_string(fp, trigger);
    fprintf(fp, ",\n");
    fprintf(fp, "  \"summary\": {\n");
    fprintf(fp, "    \"active\": %zu,\n", g_active_count);
    fprintf(fp, "    \"leaked_bytes\": %zu,\n", leaked_bytes);
    fprintf(fp, "    \"allocs\": %zu,\n", g_total_allocs);
    fprintf(fp, "    \"frees\": %zu,\n", g_total_frees);
    fprintf(fp, "    \"double_free\": %zu,\n", g_double_frees);
    fprintf(fp, "    \"unknown_free\": %zu,\n", g_unknown_frees);
    fprintf(fp, "    \"tracker_failures\": %zu\n", g_tracker_failures);
    fprintf(fp, "  },\n");
    fprintf(fp, "  \"leaks\": [\n");

    int first = 1;
    for (size_t i = 0; i < FISICS_MEMCHECK_CAPACITY; ++i) {
        FisicsMemcheckSlot* slot = &g_slots[i];
        if (slot->state != FISICS_MEMCHECK_SLOT_ACTIVE) {
            continue;
        }

        if (!first) {
            fprintf(fp, ",\n");
        }
        first = 0;

        fprintf(fp, "    {\"size\": %zu, \"allocated_at\": {\"file\": ", slot->size);
        if (slot->alloc_file && slot->alloc_file[0] != '\0') {
            emit_json_string(fp, display_file_name(slot->alloc_file));
        } else {
            fprintf(fp, "null");
        }
        fprintf(fp, ", \"line\": ");
        if (slot->alloc_line > 0) {
            fprintf(fp, "%d", slot->alloc_line);
        } else {
            fprintf(fp, "null");
        }
        fprintf(fp, "}}");
    }

    fprintf(fp, "\n  ]\n");
    fprintf(fp, "}\n");
    if (fclose(fp) != 0) {
        fprintf(stderr, "[fisics:memory-check] failed to close JSON report: %s\n", path);
    }
}

static int should_emit_report(size_t leaked_bytes) {
    init_report_policy();
    switch (g_report_policy) {
        case FISICS_MEMCHECK_REPORT_NEVER:
            return 0;
        case FISICS_MEMCHECK_REPORT_ERRORS:
            return g_double_frees > 0u || g_unknown_frees > 0u || g_tracker_failures > 0u;
        case FISICS_MEMCHECK_REPORT_LEAKS:
            return leaked_bytes > 0u || g_double_frees > 0u ||
                   g_unknown_frees > 0u || g_tracker_failures > 0u;
        case FISICS_MEMCHECK_REPORT_ALWAYS:
        default:
            return g_total_allocs > 0u || g_total_frees > 0u ||
                   g_double_frees > 0u || g_unknown_frees > 0u ||
                   g_tracker_failures > 0u;
    }
}

static void emit_report(const char* trigger) {
    size_t leaked_bytes = compute_leaked_bytes();
    fprintf(stderr,
            "[fisics:memory-check] summary: active=%zu leaked_bytes=%zu allocs=%zu frees=%zu double_free=%zu unknown_free=%zu tracker_failures=%zu\n",
            g_active_count,
            leaked_bytes,
            g_total_allocs,
            g_total_frees,
            g_double_frees,
            g_unknown_frees,
            g_tracker_failures);
    if (leaked_bytes > 0u) {
        emit_leak_sites();
    }
    emit_json_report(trigger, leaked_bytes);
    g_last_report_generation = g_generation;
}

void __fisics_memcheck_report(void) {
    emit_report("manual");
}

static void __fisics_memcheck_report_at_exit(void) {
    if (g_last_report_generation == g_generation) {
        return;
    }
    size_t leaked_bytes = compute_leaked_bytes();
    if (!should_emit_report(leaked_bytes)) {
        return;
    }
    emit_report("atexit");
}

void __fisics_memcheck_reset(void) {
    for (size_t i = 0; i < FISICS_MEMCHECK_CAPACITY; ++i) {
        g_slots[i].ptr = NULL;
        g_slots[i].size = 0u;
        g_slots[i].alloc_file = NULL;
        g_slots[i].alloc_line = 0;
        g_slots[i].state = FISICS_MEMCHECK_SLOT_EMPTY;
    }
    g_active_count = 0u;
    g_total_allocs = 0u;
    g_total_frees = 0u;
    g_double_frees = 0u;
    g_unknown_frees = 0u;
    g_tracker_failures = 0u;
    g_generation = 0u;
    g_last_report_generation = 0u;
}
