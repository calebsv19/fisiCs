// SPDX-License-Identifier: Apache-2.0

#include "Utils/profiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct ProfilerEntry {
    const char* name;
    uint64_t duration_ns;
} ProfilerEntry;

static int g_enabled = -1;
static int g_stream = 0;
static char* g_output_path = NULL;
static ProfilerEntry* g_entries = NULL;
static size_t g_entry_count = 0;
static size_t g_entry_capacity = 0;
static FILE* g_stream_file = NULL;

static uint64_t profiler_now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static void profiler_add_entry(const char* name, uint64_t duration_ns) {
    if (!name) return;
    if (g_entry_count == g_entry_capacity) {
        size_t next = g_entry_capacity ? g_entry_capacity * 2 : 64;
        ProfilerEntry* resized = (ProfilerEntry*)realloc(g_entries, next * sizeof(*g_entries));
        if (!resized) {
            return;
        }
        g_entries = resized;
        g_entry_capacity = next;
    }
    g_entries[g_entry_count++] = (ProfilerEntry){ .name = name, .duration_ns = duration_ns };
}

void profiler_init(void) {
    if (g_enabled != -1) return;
    const char* env = getenv("FISICS_PROFILE");
    g_enabled = (env && env[0] && env[0] != '0') ? 1 : 0;
    if (!g_enabled) return;

    const char* out = getenv("FISICS_PROFILE_PATH");
    if (out && out[0]) {
        g_output_path = strdup(out);
    }
    const char* stream = getenv("FISICS_PROFILE_STREAM");
    g_stream = (stream && stream[0] && stream[0] != '0') ? 1 : 0;
    if (g_stream && g_output_path) {
        g_stream_file = fopen(g_output_path, "w");
        if (g_stream_file) {
            fprintf(g_stream_file, "name,duration_ns,duration_ms\n");
            fflush(g_stream_file);
        }
    }
}

int profiler_enabled(void) {
    if (g_enabled == -1) {
        profiler_init();
    }
    return g_enabled == 1;
}

ProfilerScope profiler_begin(const char* name) {
    ProfilerScope scope = { .name = name, .start_ns = 0 };
    if (!profiler_enabled()) {
        return scope;
    }
    scope.start_ns = profiler_now_ns();
    return scope;
}

void profiler_end(ProfilerScope scope) {
    if (!profiler_enabled() || !scope.name || scope.start_ns == 0) {
        return;
    }
    uint64_t end = profiler_now_ns();
    uint64_t duration = end - scope.start_ns;
    profiler_add_entry(scope.name, duration);
    if (g_stream_file) {
        double ms = (double)duration / 1000000.0;
        fprintf(g_stream_file, "%s,%llu,%.3f\n", scope.name, (unsigned long long)duration, ms);
        fflush(g_stream_file);
    }
}

void profiler_shutdown(void) {
    if (!profiler_enabled()) return;

    if (!g_stream_file) {
        FILE* out = stderr;
        if (g_output_path) {
            FILE* f = fopen(g_output_path, "w");
            if (f) out = f;
        }

        fprintf(out, "name,duration_ns,duration_ms\n");
        for (size_t i = 0; i < g_entry_count; ++i) {
            const ProfilerEntry* e = &g_entries[i];
            double ms = (double)e->duration_ns / 1000000.0;
            fprintf(out, "%s,%llu,%.3f\n", e->name, (unsigned long long)e->duration_ns, ms);
        }

        if (out != stderr) {
            fclose(out);
        }
    } else {
        fclose(g_stream_file);
        g_stream_file = NULL;
    }

    free(g_entries);
    g_entries = NULL;
    g_entry_count = 0;
    g_entry_capacity = 0;
    free(g_output_path);
    g_output_path = NULL;
    g_enabled = 0;
    g_stream = 0;
}
