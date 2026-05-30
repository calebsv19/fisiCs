// SPDX-License-Identifier: Apache-2.0

#ifndef PREPROCESSOR_INCLUDE_RESOLVER_INTERNAL_H
#define PREPROCESSOR_INCLUDE_RESOLVER_INTERNAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "Preprocessor/include_resolver.h"

typedef enum {
    INCLUDE_PROFILE_PHASE_SAME_DIR = 0,
    INCLUDE_PROFILE_PHASE_ANCESTOR = 1,
    INCLUDE_PROFILE_PHASE_INCLUDE_PATH = 2,
    INCLUDE_PROFILE_PHASE_RAW = 3,
    INCLUDE_PROFILE_PHASE_COUNT = 4
} IncludeProfilePhase;

struct IncludeLoadProfileEntry {
    char* path;
    IncludeSearchOrigin finalOrigin;
    uint64_t firstLoadCount;
    uint64_t probeCount[INCLUDE_PROFILE_PHASE_COUNT];
    uint64_t failedProbeCount[INCLUDE_PROFILE_PHASE_COUNT];
    uint64_t missingDirCount[INCLUDE_PROFILE_PHASE_COUNT];
    uint64_t missingLeafCount[INCLUDE_PROFILE_PHASE_COUNT];
    uint64_t probeTimeNs[INCLUDE_PROFILE_PHASE_COUNT];
    uint64_t canonicalizeNs;
    uint64_t statNs;
    uint64_t readNs;
};

struct IncludeLoadAttemptTrace {
    IncludeProfilePhase currentPhase;
    uint64_t probeCount[INCLUDE_PROFILE_PHASE_COUNT];
    uint64_t failedProbeCount[INCLUDE_PROFILE_PHASE_COUNT];
    uint64_t missingDirCount[INCLUDE_PROFILE_PHASE_COUNT];
    uint64_t missingLeafCount[INCLUDE_PROFILE_PHASE_COUNT];
    uint64_t probeTimeNs[INCLUDE_PROFILE_PHASE_COUNT];
    uint64_t canonicalizeNs;
    uint64_t statNs;
    uint64_t readNs;
    bool loadedNewFile;
    IncludeSearchOrigin finalOrigin;
};

struct IncludePathHintEntry {
    char* includeName;
    bool isSystem;
    bool isIncludeNext;
    size_t includePathIndex;
};

char* ir_strdup(const char* s);
uint64_t ir_now_ns(void);
size_t ir_parse_size_t_env(const char* name, size_t defaultValue);
IncludeProfilePhase ir_phase_from_origin(IncludeSearchOrigin origin);
void ir_set_trace_phase(IncludeResolver* resolver, IncludeProfilePhase phase);
void ir_trace_add_probe_duration(IncludeResolver* resolver,
                                 uint64_t durationNs,
                                 bool failedProbe);
void ir_trace_note_missing_kind(IncludeResolver* resolver,
                                bool missingDir,
                                bool missingLeaf);
void ir_record_profile_entry(IncludeResolver* resolver,
                             const IncludeFile* file,
                             const IncludeLoadAttemptTrace* trace);
void ir_dump_profile_report(const IncludeResolver* resolver);
size_t ir_lookup_include_path_hint_index(const IncludeResolver* resolver,
                                         const char* name,
                                         bool isSystem,
                                         bool isIncludeNext);
size_t ir_lookup_include_path_exact_alias_index(const IncludeResolver* resolver,
                                                const char* name,
                                                size_t startIdx);
size_t ir_lookup_include_path_stem_match_index(const IncludeResolver* resolver,
                                               const char* name,
                                               size_t startIdx);
bool ir_cache_include_path_hint(IncludeResolver* resolver,
                                const char* name,
                                bool isSystem,
                                bool isIncludeNext,
                                size_t includePathIndex);
bool ir_build_parent_dir(char* buffer, size_t bufSize, const char* path);
size_t ir_lookup_request_cache_index(const IncludeResolver* resolver,
                                     size_t parentFileIndex,
                                     const char* parentDir,
                                     const char* name,
                                     bool isSystem,
                                     bool isIncludeNext);
bool ir_cache_request_result(IncludeResolver* resolver,
                             size_t parentFileIndex,
                             const char* parentDir,
                             const char* name,
                             bool isSystem,
                             bool isIncludeNext,
                             size_t fileIndex,
                             IncludeSearchOrigin origin,
                             size_t originIndex);
bool ir_append_file(IncludeResolver* resolver, IncludeFile file);
const IncludeFile* ir_lookup_exact_path(const IncludeResolver* resolver, const char* path);
const IncludeFile* ir_try_virtual_audio_toolbox(IncludeResolver* resolver, const char* name);

#endif /* PREPROCESSOR_INCLUDE_RESOLVER_INTERNAL_H */
