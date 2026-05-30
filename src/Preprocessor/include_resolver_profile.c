// SPDX-License-Identifier: Apache-2.0

#include "Preprocessor/include_resolver_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static char* ir_profile_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* copy = malloc(len);
    if (copy) memcpy(copy, s, len);
    return copy;
}

uint64_t ir_now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static const char* ir_origin_label(IncludeSearchOrigin origin) {
    switch (origin) {
        case INCLUDE_SEARCH_SAME_DIR: return "same_dir";
        case INCLUDE_SEARCH_INCLUDE_PATH: return "include_path";
        case INCLUDE_SEARCH_RAW:
        default: return "raw";
    }
}

size_t ir_parse_size_t_env(const char* name, size_t defaultValue) {
    const char* raw = getenv(name);
    if (!raw || !raw[0]) return defaultValue;
    char* end = NULL;
    unsigned long long value = strtoull(raw, &end, 10);
    if (!end || *end != '\0') return defaultValue;
    return (size_t)value;
}

IncludeProfilePhase ir_phase_from_origin(IncludeSearchOrigin origin) {
    switch (origin) {
        case INCLUDE_SEARCH_SAME_DIR: return INCLUDE_PROFILE_PHASE_SAME_DIR;
        case INCLUDE_SEARCH_INCLUDE_PATH: return INCLUDE_PROFILE_PHASE_INCLUDE_PATH;
        case INCLUDE_SEARCH_RAW:
        default: return INCLUDE_PROFILE_PHASE_RAW;
    }
}

void ir_set_trace_phase(IncludeResolver* resolver, IncludeProfilePhase phase) {
    if (!resolver || !resolver->activeTrace) return;
    resolver->activeTrace->currentPhase = phase;
}

void ir_trace_add_probe_duration(IncludeResolver* resolver,
                                 uint64_t durationNs,
                                 bool failedProbe) {
    if (!resolver || !resolver->activeTrace) return;
    IncludeLoadAttemptTrace* trace = resolver->activeTrace;
    size_t phase = (size_t)trace->currentPhase;
    if (phase >= INCLUDE_PROFILE_PHASE_COUNT) return;
    trace->probeCount[phase] += 1u;
    trace->probeTimeNs[phase] += durationNs;
    if (failedProbe) {
        trace->failedProbeCount[phase] += 1u;
    }
}

void ir_trace_note_missing_kind(IncludeResolver* resolver,
                                bool missingDir,
                                bool missingLeaf) {
    if (!resolver || !resolver->activeTrace) return;
    IncludeLoadAttemptTrace* trace = resolver->activeTrace;
    size_t phase = (size_t)trace->currentPhase;
    if (phase >= INCLUDE_PROFILE_PHASE_COUNT) return;
    if (missingDir) {
        trace->missingDirCount[phase] += 1u;
    }
    if (missingLeaf) {
        trace->missingLeafCount[phase] += 1u;
    }
}

static IncludeLoadProfileEntry* ir_find_profile_entry(IncludeResolver* resolver, const char* path) {
    if (!resolver || !path) return NULL;
    for (size_t i = 0; i < resolver->profileEntryCount; ++i) {
        if (strcmp(resolver->profileEntries[i].path, path) == 0) {
            return &resolver->profileEntries[i];
        }
    }
    return NULL;
}

void ir_record_profile_entry(IncludeResolver* resolver,
                             const IncludeFile* file,
                             const IncludeLoadAttemptTrace* trace) {
    if (!resolver || !resolver->headerProfileEnabled || !file || !trace || !trace->loadedNewFile) {
        return;
    }
    const char* path = file->canonicalPath ? file->canonicalPath : file->path;
    if (!path) return;

    IncludeLoadProfileEntry* entry = ir_find_profile_entry(resolver, path);
    if (!entry) {
        if (resolver->profileEntryCount == resolver->profileEntryCapacity) {
            size_t newCapacity = resolver->profileEntryCapacity ? resolver->profileEntryCapacity * 2 : 32;
            IncludeLoadProfileEntry* resized =
                realloc(resolver->profileEntries, newCapacity * sizeof(*resized));
            if (!resized) return;
            resolver->profileEntries = resized;
            resolver->profileEntryCapacity = newCapacity;
        }
        entry = &resolver->profileEntries[resolver->profileEntryCount++];
        memset(entry, 0, sizeof(*entry));
        entry->path = ir_profile_strdup(path);
        if (!entry->path) {
            resolver->profileEntryCount--;
            return;
        }
    }

    entry->finalOrigin = trace->finalOrigin;
    entry->firstLoadCount += 1u;
    for (size_t i = 0; i < INCLUDE_PROFILE_PHASE_COUNT; ++i) {
        entry->probeCount[i] += trace->probeCount[i];
        entry->failedProbeCount[i] += trace->failedProbeCount[i];
        entry->missingDirCount[i] += trace->missingDirCount[i];
        entry->missingLeafCount[i] += trace->missingLeafCount[i];
        entry->probeTimeNs[i] += trace->probeTimeNs[i];
    }
    entry->canonicalizeNs += trace->canonicalizeNs;
    entry->statNs += trace->statNs;
    entry->readNs += trace->readNs;
}

static uint64_t ir_entry_total_traversal_ns(const IncludeLoadProfileEntry* entry) {
    uint64_t total = 0;
    if (!entry) return 0;
    for (size_t i = 0; i < INCLUDE_PROFILE_PHASE_COUNT; ++i) {
        total += entry->probeTimeNs[i];
    }
    return total;
}

static int ir_profile_entry_cmp_desc(const void* lhs, const void* rhs) {
    const IncludeLoadProfileEntry* const* a = lhs;
    const IncludeLoadProfileEntry* const* b = rhs;
    uint64_t aTotal = ir_entry_total_traversal_ns(*a);
    uint64_t bTotal = ir_entry_total_traversal_ns(*b);
    if (aTotal < bTotal) return 1;
    if (aTotal > bTotal) return -1;
    return strcmp((*a)->path, (*b)->path);
}

void ir_dump_profile_report(const IncludeResolver* resolver) {
    if (!resolver || !resolver->headerProfileEnabled || resolver->profileEntryCount == 0) return;
    IncludeLoadProfileEntry** sorted =
        calloc(resolver->profileEntryCount, sizeof(*sorted));
    if (!sorted) return;
    for (size_t i = 0; i < resolver->profileEntryCount; ++i) {
        sorted[i] = &resolver->profileEntries[i];
    }
    qsort(sorted,
          resolver->profileEntryCount,
          sizeof(*sorted),
          ir_profile_entry_cmp_desc);

    size_t limit = resolver->headerProfileTopN;
    if (limit == 0 || limit > resolver->profileEntryCount) {
        limit = resolver->profileEntryCount;
    }

    for (size_t i = 0; i < limit; ++i) {
        const IncludeLoadProfileEntry* entry = sorted[i];
        uint64_t totalProbes = 0;
        uint64_t totalFailed = 0;
        for (size_t phase = 0; phase < INCLUDE_PROFILE_PHASE_COUNT; ++phase) {
            totalProbes += entry->probeCount[phase];
            totalFailed += entry->failedProbeCount[phase];
        }
        fprintf(stderr,
                "[pp-first-include] rank=%zu origin=%s first_loads=%llu total_ms=%.3f probes=%llu failed=%llu same_dir_ms=%.3f ancestor_ms=%.3f include_path_ms=%.3f raw_ms=%.3f same_dir_dir_miss=%llu same_dir_leaf_miss=%llu ancestor_dir_miss=%llu ancestor_leaf_miss=%llu include_path_dir_miss=%llu include_path_leaf_miss=%llu raw_dir_miss=%llu raw_leaf_miss=%llu canonicalize_ms=%.3f stat_ms=%.3f read_ms=%.3f path=%s\n",
                i + 1u,
                ir_origin_label(entry->finalOrigin),
                (unsigned long long)entry->firstLoadCount,
                (double)ir_entry_total_traversal_ns(entry) / 1000000.0,
                (unsigned long long)totalProbes,
                (unsigned long long)totalFailed,
                (double)entry->probeTimeNs[INCLUDE_PROFILE_PHASE_SAME_DIR] / 1000000.0,
                (double)entry->probeTimeNs[INCLUDE_PROFILE_PHASE_ANCESTOR] / 1000000.0,
                (double)entry->probeTimeNs[INCLUDE_PROFILE_PHASE_INCLUDE_PATH] / 1000000.0,
                (double)entry->probeTimeNs[INCLUDE_PROFILE_PHASE_RAW] / 1000000.0,
                (unsigned long long)entry->missingDirCount[INCLUDE_PROFILE_PHASE_SAME_DIR],
                (unsigned long long)entry->missingLeafCount[INCLUDE_PROFILE_PHASE_SAME_DIR],
                (unsigned long long)entry->missingDirCount[INCLUDE_PROFILE_PHASE_ANCESTOR],
                (unsigned long long)entry->missingLeafCount[INCLUDE_PROFILE_PHASE_ANCESTOR],
                (unsigned long long)entry->missingDirCount[INCLUDE_PROFILE_PHASE_INCLUDE_PATH],
                (unsigned long long)entry->missingLeafCount[INCLUDE_PROFILE_PHASE_INCLUDE_PATH],
                (unsigned long long)entry->missingDirCount[INCLUDE_PROFILE_PHASE_RAW],
                (unsigned long long)entry->missingLeafCount[INCLUDE_PROFILE_PHASE_RAW],
                (double)entry->canonicalizeNs / 1000000.0,
                (double)entry->statNs / 1000000.0,
                (double)entry->readNs / 1000000.0,
                entry->path ? entry->path : "<unknown>");
    }
    free(sorted);
}
