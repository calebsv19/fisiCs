// SPDX-License-Identifier: Apache-2.0

#include "Preprocessor/include_resolver.h"
#include "Preprocessor/include_resolver_internal.h"
#include "Utils/profiler.h"
#include "core_io.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

static bool ir_path_is_dir(const char* path);
static bool ir_path_is_absolute(const char* path);

char* ir_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* copy = malloc(len);
    if (copy) memcpy(copy, s, len);
    return copy;
}

static bool ir_path_is_absolute(const char* path) {
    if (!path || !path[0]) return false;
    if (path[0] == '/') return true;
    return ((path[0] >= 'A' && path[0] <= 'Z') ||
            (path[0] >= 'a' && path[0] <= 'z')) &&
           path[1] == ':';
}

static char* ir_canonicalize_path(const char* path) {
    if (!path) return NULL;
    profiler_record_value("pp_count_path_canonicalize_calls", 1);
    ProfilerScope scope = profiler_begin("pp_fs_canonicalize");
    char resolved[4096];
    if (realpath(path, resolved)) {
        profiler_end(scope);
        return ir_strdup(resolved);
    }
    profiler_end(scope);
    return ir_strdup(path);
}

static bool ir_path_exists(const char* path, long* mtimeOut) {
    profiler_record_value("pp_count_fs_stat_calls", 1);
    ProfilerScope scope = profiler_begin("pp_fs_stat");
    struct stat st;
    if (stat(path, &st) != 0) {
        profiler_end(scope);
        return false;
    }
    profiler_end(scope);
    if (mtimeOut) *mtimeOut = (long)st.st_mtime;
    return true;
}

static bool ir_path_is_dir(const char* path) {
    struct stat st;
    if (!path || stat(path, &st) != 0) return false;
    return S_ISDIR(st.st_mode);
}

static void ir_classify_missing_path(const char* path,
                                     bool* missingDirOut,
                                     bool* missingLeafOut) {
    if (missingDirOut) *missingDirOut = false;
    if (missingLeafOut) *missingLeafOut = false;
    if (!path) return;

    const char* slash = strrchr(path, '/');
    if (!slash) {
        if (missingLeafOut) *missingLeafOut = true;
        return;
    }

    size_t dirLen = (size_t)(slash - path);
    if (dirLen == 0 || dirLen >= 4096) {
        if (missingLeafOut) *missingLeafOut = true;
        return;
    }

    char dir[4096];
    memcpy(dir, path, dirLen);
    dir[dirLen] = '\0';

    struct stat st;
    if (stat(dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
        if (missingDirOut) *missingDirOut = true;
        return;
    }
    if (missingLeafOut) *missingLeafOut = true;
}

static char* ir_read_file(const char* path) {
    profiler_record_value("pp_count_file_open_read_calls", 1);
    ProfilerScope scope = profiler_begin("pp_fs_read");
    CoreBuffer file_data = {0};
    CoreResult read_result = core_io_read_all(path, &file_data);
    if (read_result.code != CORE_OK) {
        profiler_end(scope);
        return NULL;
    }
    profiler_record_value("pp_bytes_file_read", file_data.size);

    char* buffer = malloc(file_data.size + 1u);
    if (!buffer) {
        core_io_buffer_free(&file_data);
        profiler_end(scope);
        return NULL;
    }
    if (file_data.size > 0u) {
        memcpy(buffer, file_data.data, file_data.size);
    }
    buffer[file_data.size] = '\0';
    core_io_buffer_free(&file_data);
    profiler_end(scope);
    return buffer;
}

static size_t ir_lookup_exact_index(const IncludeResolver* resolver, const char* path);
static size_t ir_lookup_canonical_index(const IncludeResolver* resolver, const char* canonicalPath);
static const IncludeFile* ir_lookup_by_canonical_path(const IncludeResolver* resolver,
                                                      const char* canonicalPath);

bool ir_append_file(IncludeResolver* resolver, IncludeFile file) {
    if (resolver->count == resolver->capacity) {
        size_t newCap = resolver->capacity ? resolver->capacity * 2 : 8;
        IncludeFile* files = realloc(resolver->files, newCap * sizeof(IncludeFile));
        if (!files) return false;
        resolver->files = files;
        resolver->capacity = newCap;
    }
    resolver->files[resolver->count++] = file;
    return true;
}

static size_t ir_lookup_exact_index(const IncludeResolver* resolver, const char* path) {
    if (!resolver || !path) return (size_t)-1;
    for (size_t i = 0; i < resolver->count; ++i) {
        if (strcmp(resolver->files[i].path, path) == 0) {
            return i;
        }
    }
    return (size_t)-1;
}

static size_t ir_lookup_canonical_index(const IncludeResolver* resolver, const char* canonicalPath) {
    if (!resolver || !canonicalPath) return (size_t)-1;
    for (size_t i = 0; i < resolver->count; ++i) {
        const char* existing = resolver->files[i].canonicalPath
                                   ? resolver->files[i].canonicalPath
                                   : resolver->files[i].path;
        bool equal = existing && strcmp(existing, canonicalPath) == 0;
        if (equal) {
            return i;
        }
    }
    return (size_t)-1;
}

const IncludeFile* ir_lookup_exact_path(const IncludeResolver* resolver, const char* path) {
    size_t index = ir_lookup_exact_index(resolver, path);
    if (index == (size_t)-1) return NULL;
    return &resolver->files[index];
}

static const IncludeFile* ir_lookup_by_canonical_path(const IncludeResolver* resolver,
                                                      const char* canonicalPath) {
    size_t index = ir_lookup_canonical_index(resolver, canonicalPath);
    if (index == (size_t)-1) return NULL;
    return &resolver->files[index];
}

const IncludeFile* include_resolver_lookup(const IncludeResolver* resolver, const char* path) {
    return ir_lookup_exact_path(resolver, path);
}

bool include_resolver_set_root_buffer(IncludeResolver* resolver,
                                      const char* path,
                                      char* contents_owned,
                                      long mtime) {
    if (!resolver || !path || !contents_owned) return false;
    if (ir_lookup_exact_path(resolver, path)) {
        free(contents_owned);
        return true;
    }
    IncludeFile file = {0};
    file.path = ir_strdup(path);
    file.contents = contents_owned;
    file.lexedTokens = NULL;
    file.lexedTokenCount = 0;
    file.lexedTokenCapacity = 0;
    file.cachedGuardName = NULL;
    file.canonicalPath = ir_canonicalize_path(path);
    file.summaryProbe = (IncludeSummaryProbe){0};
    file.summaryActions = NULL;
    file.summaryActionCount = 0;
    file.mtime = mtime;
    file.pragmaOnce = false;
    file.includedOnce = false;
    file.origin = INCLUDE_SEARCH_RAW;
    file.originIndex = (size_t)-1;
    if (!file.path || !file.canonicalPath) {
        free(file.path);
        free(file.canonicalPath);
        free(contents_owned);
        return false;
    }
    if (!ir_append_file(resolver, file)) {
        free(file.path);
        free(file.contents);
        free(file.canonicalPath);
        return false;
    }
    return true;
}

static bool ir_build_path(char* buffer, size_t bufSize, const char* dir, const char* name) {
    if (!dir || dir[0] == '\0') {
        return snprintf(buffer, bufSize, "%s", name) < (int)bufSize;
    }
    size_t len = snprintf(buffer, bufSize, "%s/%s", dir, name);
    return len < bufSize;
}

static const IncludeFile* ir_try_load(IncludeResolver* resolver,
                                      const char* path,
                                      IncludeSearchOrigin origin,
                                      size_t originIndex);

static const IncludeFile* ir_search_ancestor_dirs(IncludeResolver* resolver,
                                                  const char* includingFile,
                                                  const char* name,
                                                  IncludeSearchOrigin* originOut,
                                                  size_t* originIndexOut) {
    char dir[4096];
    if (!resolver || !includingFile || !name) return NULL;

    const char* slash = strrchr(includingFile, '/');
    if (!slash) return NULL;
    size_t dirLen = (size_t)(slash - includingFile);
    if (dirLen == 0 || dirLen >= sizeof(dir)) return NULL;
    memcpy(dir, includingFile, dirLen);
    dir[dirLen] = '\0';

    // The caller already probed the includer's directory; start ancestor fallback one level up.
    char* up = strrchr(dir, '/');
    if (!up || up == dir) return NULL;
    *up = '\0';

    for (int depth = 0; depth < 24; ++depth) {
        char candidate[4096];
        if (ir_build_path(candidate, sizeof(candidate), dir, name)) {
            ir_set_trace_phase(resolver, INCLUDE_PROFILE_PHASE_ANCESTOR);
            const IncludeFile* file = ir_try_load(resolver,
                                                  candidate,
                                                  INCLUDE_SEARCH_SAME_DIR,
                                                  (size_t)-1);
            if (file) {
                if (originOut) *originOut = INCLUDE_SEARCH_SAME_DIR;
                if (originIndexOut) *originIndexOut = (size_t)-1;
                return file;
            }
        }

        up = strrchr(dir, '/');
        if (!up || up == dir) break;
        *up = '\0';
    }

    return NULL;
}

static const IncludeFile* ir_try_framework_header(IncludeResolver* resolver,
                                                  const char* name,
                                                  IncludeSearchOrigin* originOut,
                                                  size_t* originIndexOut) {
    char framework[256];
    char candidate[4096];
    const char* slash = NULL;
    const char* sdkRoot = NULL;
    const char* roots[3];
    size_t rootCount = 0;

    if (!resolver || !name) return NULL;
    slash = strchr(name, '/');
    if (!slash) return NULL;
    if ((size_t)(slash - name) >= sizeof(framework)) return NULL;

    memcpy(framework, name, (size_t)(slash - name));
    framework[slash - name] = '\0';

    sdkRoot = getenv("SDKROOT");
    if (sdkRoot && sdkRoot[0] != '\0') {
        roots[rootCount++] = sdkRoot;
    }
    roots[rootCount++] = "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk";
    roots[rootCount++] = "";

    for (size_t i = 0; i < rootCount; ++i) {
        const char* base = roots[i];
        if (base && base[0] != '\0') {
            if (snprintf(candidate,
                         sizeof(candidate),
                         "%s/System/Library/Frameworks/%s.framework/Headers/%s",
                         base,
                         framework,
                         slash + 1) < (int)sizeof(candidate)) {
                ir_set_trace_phase(resolver, INCLUDE_PROFILE_PHASE_RAW);
                const IncludeFile* file = ir_try_load(resolver, candidate, INCLUDE_SEARCH_RAW, (size_t)-1);
                if (file) {
                    if (originOut) *originOut = INCLUDE_SEARCH_RAW;
                    if (originIndexOut) *originIndexOut = (size_t)-1;
                    return file;
                }
            }
            if (snprintf(candidate,
                         sizeof(candidate),
                         "%s/Library/Frameworks/%s.framework/Headers/%s",
                         base,
                         framework,
                         slash + 1) < (int)sizeof(candidate)) {
                ir_set_trace_phase(resolver, INCLUDE_PROFILE_PHASE_RAW);
                const IncludeFile* file = ir_try_load(resolver, candidate, INCLUDE_SEARCH_RAW, (size_t)-1);
                if (file) {
                    if (originOut) *originOut = INCLUDE_SEARCH_RAW;
                    if (originIndexOut) *originIndexOut = (size_t)-1;
                    return file;
                }
            }
        } else {
            if (snprintf(candidate,
                         sizeof(candidate),
                         "/System/Library/Frameworks/%s.framework/Headers/%s",
                         framework,
                         slash + 1) < (int)sizeof(candidate)) {
                ir_set_trace_phase(resolver, INCLUDE_PROFILE_PHASE_RAW);
                const IncludeFile* file = ir_try_load(resolver, candidate, INCLUDE_SEARCH_RAW, (size_t)-1);
                if (file) {
                    if (originOut) *originOut = INCLUDE_SEARCH_RAW;
                    if (originIndexOut) *originIndexOut = (size_t)-1;
                    return file;
                }
            }
            if (snprintf(candidate,
                         sizeof(candidate),
                         "/Library/Frameworks/%s.framework/Headers/%s",
                         framework,
                         slash + 1) < (int)sizeof(candidate)) {
                ir_set_trace_phase(resolver, INCLUDE_PROFILE_PHASE_RAW);
                const IncludeFile* file = ir_try_load(resolver, candidate, INCLUDE_SEARCH_RAW, (size_t)-1);
                if (file) {
                    if (originOut) *originOut = INCLUDE_SEARCH_RAW;
                    if (originIndexOut) *originIndexOut = (size_t)-1;
                    return file;
                }
            }
        }
    }

    return NULL;
}

static const IncludeFile* ir_try_shared_workspace_header(IncludeResolver* resolver,
                                                         const char* includingFile,
                                                         const char* name,
                                                         IncludeSearchOrigin* originOut,
                                                         size_t* originIndexOut) {
    char dir[4096];
    if (!resolver || !includingFile || !name || !name[0]) return NULL;

    const char* slash = strrchr(includingFile, '/');
    if (!slash) return NULL;
    size_t dirLen = (size_t)(slash - includingFile);
    if (dirLen == 0 || dirLen >= sizeof(dir)) return NULL;
    memcpy(dir, includingFile, dirLen);
    dir[dirLen] = '\0';

    for (int depth = 0; depth < 24; ++depth) {
        char sharedRoot[4096];
        if (!ir_build_path(sharedRoot, sizeof(sharedRoot), dir, "shared")) {
            break;
        }
        if (ir_path_is_dir(sharedRoot)) {
            char candidate[4096];
            if (snprintf(candidate, sizeof(candidate), "%s/include/%s", sharedRoot, name) < (int)sizeof(candidate)) {
                ir_set_trace_phase(resolver, INCLUDE_PROFILE_PHASE_RAW);
                const IncludeFile* file = ir_try_load(resolver, candidate, INCLUDE_SEARCH_RAW, (size_t)-1);
                if (file) {
                    if (originOut) *originOut = INCLUDE_SEARCH_RAW;
                    if (originIndexOut) *originIndexOut = (size_t)-1;
                    return file;
                }
            }

            DIR* d1 = opendir(sharedRoot);
            if (d1) {
                struct dirent* e1 = NULL;
                while ((e1 = readdir(d1)) != NULL) {
                    if (e1->d_name[0] == '.') continue;
                    char l1[4096];
                    if (snprintf(l1, sizeof(l1), "%s/%s", sharedRoot, e1->d_name) >= (int)sizeof(l1)) {
                        continue;
                    }
                    if (!ir_path_is_dir(l1)) continue;

                    if (snprintf(candidate, sizeof(candidate), "%s/include/%s", l1, name) < (int)sizeof(candidate)) {
                        ir_set_trace_phase(resolver, INCLUDE_PROFILE_PHASE_RAW);
                        const IncludeFile* file = ir_try_load(resolver, candidate, INCLUDE_SEARCH_RAW, (size_t)-1);
                        if (file) {
                            closedir(d1);
                            if (originOut) *originOut = INCLUDE_SEARCH_RAW;
                            if (originIndexOut) *originIndexOut = (size_t)-1;
                            return file;
                        }
                    }

                    DIR* d2 = opendir(l1);
                    if (!d2) continue;
                    struct dirent* e2 = NULL;
                    while ((e2 = readdir(d2)) != NULL) {
                        if (e2->d_name[0] == '.') continue;
                        char l2[4096];
                        if (snprintf(l2, sizeof(l2), "%s/%s", l1, e2->d_name) >= (int)sizeof(l2)) {
                            continue;
                        }
                        if (!ir_path_is_dir(l2)) continue;
                        if (snprintf(candidate, sizeof(candidate), "%s/include/%s", l2, name) >= (int)sizeof(candidate)) {
                            continue;
                        }
                        ir_set_trace_phase(resolver, INCLUDE_PROFILE_PHASE_RAW);
                        const IncludeFile* file = ir_try_load(resolver, candidate, INCLUDE_SEARCH_RAW, (size_t)-1);
                        if (file) {
                            closedir(d2);
                            closedir(d1);
                            if (originOut) *originOut = INCLUDE_SEARCH_RAW;
                            if (originIndexOut) *originIndexOut = (size_t)-1;
                            return file;
                        }
                    }
                    closedir(d2);
                }
                closedir(d1);
            }
        }

        char* up = strrchr(dir, '/');
        if (!up) break;
        if (up == dir) break;
        *up = '\0';
    }

    return NULL;
}

IncludeResolver* include_resolver_create(const char* const* includePaths, size_t pathCount) {
    IncludeResolver* resolver = calloc(1, sizeof(IncludeResolver));
    if (!resolver) return NULL;
    resolver->headerProfileEnabled = getenv("FISICS_PP_FIRST_INCLUDE_REPORT") != NULL;
    resolver->headerProfileTopN = ir_parse_size_t_env("FISICS_PP_FIRST_INCLUDE_REPORT_TOP", 40);
    if (pathCount > 0) {
        resolver->includePaths = calloc(pathCount, sizeof(char*));
        if (!resolver->includePaths) {
            free(resolver);
            return NULL;
        }
        for (size_t i = 0; i < pathCount; ++i) {
            resolver->includePaths[i] = ir_strdup(includePaths[i]);
        }
        resolver->includePathCount = pathCount;
    }
    return resolver;
}

void include_resolver_destroy(IncludeResolver* resolver) {
    if (!resolver) return;
    ir_dump_profile_report(resolver);
    for (size_t i = 0; i < resolver->requestCacheCount; ++i) {
        free(resolver->requestCache[i].parentDir);
        free(resolver->requestCache[i].includeName);
    }
    free(resolver->requestCache);
    free(resolver->requestCacheHashSlots);
    for (size_t i = 0; i < resolver->includePathHintCount; ++i) {
        free(resolver->includePathHints[i].includeName);
    }
    free(resolver->includePathHints);
    for (size_t i = 0; i < resolver->count; ++i) {
        free(resolver->files[i].path);
        free(resolver->files[i].contents);
        free(resolver->files[i].lexedTokens);
        free(resolver->files[i].cachedGuardName);
        free(resolver->files[i].canonicalPath);
        free(resolver->files[i].summaryActions);
    }
    free(resolver->files);
    if (resolver->includePaths) {
        for (size_t i = 0; i < resolver->includePathCount; ++i) {
            free(resolver->includePaths[i]);
        }
        free(resolver->includePaths);
    }
    for (size_t i = 0; i < resolver->profileEntryCount; ++i) {
        free(resolver->profileEntries[i].path);
    }
    free(resolver->profileEntries);
    free(resolver);
}

static const IncludeFile* ir_try_load(IncludeResolver* resolver,
                                      const char* path,
                                      IncludeSearchOrigin origin,
                                      size_t originIndex) {
    profiler_record_value("pp_count_resolver_try_load", 1);
    const IncludeFile* exactCached = ir_lookup_exact_path(resolver, path);
    if (exactCached) {
        profiler_record_value("pp_count_resolver_exact_cache_hit", 1);
        ir_trace_add_probe_duration(resolver, 0, false);
        return exactCached;
    }

    uint64_t probeStartNs = ir_now_ns();
    long mtime = 0;
    uint64_t statStartNs = ir_now_ns();
    bool exists = ir_path_exists(path, &mtime);
    uint64_t statEndNs = ir_now_ns();
    if (resolver && resolver->activeTrace) {
        resolver->activeTrace->statNs += (statEndNs - statStartNs);
    }
    if (!exists) {
        bool missingDir = false;
        bool missingLeaf = false;
        ir_classify_missing_path(path, &missingDir, &missingLeaf);
        ir_trace_note_missing_kind(resolver, missingDir, missingLeaf);
        ir_trace_add_probe_duration(resolver, ir_now_ns() - probeStartNs, true);
        return NULL;
    }

    uint64_t canonicalStartNs = ir_now_ns();
    char* canonical = ir_canonicalize_path(path);
    uint64_t canonicalEndNs = ir_now_ns();
    if (!canonical) {
        ir_trace_add_probe_duration(resolver, ir_now_ns() - probeStartNs, true);
        return NULL;
    }
    if (resolver && resolver->activeTrace) {
        resolver->activeTrace->canonicalizeNs += (canonicalEndNs - canonicalStartNs);
    }

    const IncludeFile* cached = ir_lookup_by_canonical_path(resolver, canonical);
    if (cached && cached->mtime == mtime) {
        profiler_record_value("pp_count_resolver_cache_hit", 1);
        ir_trace_add_probe_duration(resolver, ir_now_ns() - probeStartNs, false);
        free(canonical);
        // cached origin is authoritative; ignore requested origin/index
        return cached;
    }
    profiler_record_value("pp_count_resolver_cache_miss", 1);

    uint64_t readStartNs = ir_now_ns();
    char* data = ir_read_file(path);
    uint64_t readEndNs = ir_now_ns();
    if (resolver && resolver->activeTrace) {
        resolver->activeTrace->readNs += (readEndNs - readStartNs);
    }
    if (!data) {
        ir_trace_add_probe_duration(resolver, ir_now_ns() - probeStartNs, true);
        free(canonical);
        return NULL;
    }

    IncludeFile file = {0};
    file.path = ir_strdup(path);
    if (!file.path) {
        free(data);
        free(canonical);
        ir_trace_add_probe_duration(resolver, ir_now_ns() - probeStartNs, true);
        return NULL;
    }
    file.contents = data;
    file.lexedTokens = NULL;
    file.lexedTokenCount = 0;
    file.lexedTokenCapacity = 0;
    file.cachedGuardName = NULL;
    file.canonicalPath = canonical;
    file.summaryProbe = (IncludeSummaryProbe){0};
    file.summaryActions = NULL;
    file.summaryActionCount = 0;
    file.mtime = mtime;
    file.pragmaOnce = false;
    file.includedOnce = false;
    file.origin = origin;
    file.originIndex = originIndex;

    if (!ir_append_file(resolver, file)) {
        free(file.path);
        free(file.contents);
        free(file.canonicalPath);
        ir_trace_add_probe_duration(resolver, ir_now_ns() - probeStartNs, true);
        return NULL;
    }
    profiler_record_value("pp_count_unique_files_loaded", 1);
    ir_trace_add_probe_duration(resolver, ir_now_ns() - probeStartNs, false);
    if (resolver && resolver->activeTrace) {
        resolver->activeTrace->loadedNewFile = true;
        resolver->activeTrace->finalOrigin = origin;
    }

    return &resolver->files[resolver->count - 1];
}

static const IncludeFile* ir_search_and_load(IncludeResolver* resolver,
                                             const char* includingFile,
                                             const char* name,
                                             bool isSystem,
                                             bool isIncludeNext,
                                             IncludeSearchOrigin* originOut,
                                             size_t* originIndexOut) {
    char candidate[4096];
    IncludeSearchOrigin origin = INCLUDE_SEARCH_RAW;
    size_t originIndex = (size_t)-1;
    IncludeSearchOrigin parentOrigin = INCLUDE_SEARCH_RAW;
    size_t parentIndex = (size_t)-1;

    if (includingFile) {
        const IncludeFile* parent = ir_lookup_exact_path(resolver, includingFile);
        if (parent) {
            parentOrigin = parent->origin;
            parentIndex = parent->originIndex;
        }
    }

    // 1) If quoted include and includingFile provided, search its directory.
    if (!isSystem && includingFile && !isIncludeNext) {
        const char* slash = strrchr(includingFile, '/');
        if (slash) {
            size_t dirLen = (size_t)(slash - includingFile);
            char dir[4096];
            if (dirLen < sizeof(dir)) {
                memcpy(dir, includingFile, dirLen);
                dir[dirLen] = '\0';
                if (ir_build_path(candidate, sizeof(candidate), dir, name)) {
                    ir_set_trace_phase(resolver, INCLUDE_PROFILE_PHASE_SAME_DIR);
                    const IncludeFile* file = ir_try_load(resolver,
                                                          candidate,
                                                          INCLUDE_SEARCH_SAME_DIR,
                                                          (size_t)-1);
                    if (file) {
                        origin = INCLUDE_SEARCH_SAME_DIR;
                        originIndex = (size_t)-1;
                        if (originOut) *originOut = origin;
                        if (originIndexOut) *originIndexOut = originIndex;
                        return file;
                    }
                }
            }
        }
    }

    size_t startIdx = 0;
    if (isIncludeNext && parentOrigin == INCLUDE_SEARCH_INCLUDE_PATH && parentIndex != (size_t)-1) {
        startIdx = parentIndex + 1;
    }

    size_t hintedIndex = (size_t)-1;
    bool hintedIndexTried = false;
    if (startIdx < resolver->includePathCount) {
        size_t hintEntryIndex =
            ir_lookup_include_path_hint_index(resolver, name, isSystem, isIncludeNext);
        if (hintEntryIndex != (size_t)-1) {
            size_t candidateIndex = resolver->includePathHints[hintEntryIndex].includePathIndex;
            if (candidateIndex >= startIdx && candidateIndex < resolver->includePathCount) {
                hintedIndex = candidateIndex;
                profiler_record_value("pp_count_include_path_hint_hit", 1);
                if (ir_build_path(candidate, sizeof(candidate), resolver->includePaths[hintedIndex], name)) {
                    ir_set_trace_phase(resolver, INCLUDE_PROFILE_PHASE_INCLUDE_PATH);
                    const IncludeFile* file = ir_try_load(resolver,
                                                          candidate,
                                                          INCLUDE_SEARCH_INCLUDE_PATH,
                                                          hintedIndex);
                    if (file) {
                        origin = INCLUDE_SEARCH_INCLUDE_PATH;
                        originIndex = hintedIndex;
                        if (originOut) *originOut = origin;
                        if (originIndexOut) *originIndexOut = originIndex;
                        return file;
                    }
                }
            } else {
                profiler_record_value("pp_count_include_path_hint_stale", 1);
            }
        } else {
            profiler_record_value("pp_count_include_path_hint_miss", 1);
        }
        if (hintedIndex == (size_t)-1) {
            size_t exactAliasIndex =
                ir_lookup_include_path_exact_alias_index(resolver, name, startIdx);
            if (exactAliasIndex != (size_t)-1) {
                hintedIndex = exactAliasIndex;
                profiler_record_value("pp_count_include_path_exact_alias_hint_hit", 1);
            } else {
                profiler_record_value("pp_count_include_path_exact_alias_hint_miss", 1);
            }
        }
        if (hintedIndex == (size_t)-1) {
            size_t stemMatchIndex =
                ir_lookup_include_path_stem_match_index(resolver, name, startIdx);
            if (stemMatchIndex != (size_t)-1) {
                hintedIndex = stemMatchIndex;
                profiler_record_value("pp_count_include_path_stem_hint_hit", 1);
            } else {
                profiler_record_value("pp_count_include_path_stem_hint_miss", 1);
            }
        }
    }

    // 2) For quoted includes, use a known include-path winner before paying
    // ancestor traversal. Same-dir still wins if present.
    if (!isSystem && !isIncludeNext && includingFile && hintedIndex != (size_t)-1) {
        profiler_record_value("pp_count_include_path_hint_preancestor_probe", 1);
        hintedIndexTried = true;
        if (ir_build_path(candidate, sizeof(candidate), resolver->includePaths[hintedIndex], name)) {
            ir_set_trace_phase(resolver, INCLUDE_PROFILE_PHASE_INCLUDE_PATH);
            const IncludeFile* file = ir_try_load(resolver,
                                                  candidate,
                                                  INCLUDE_SEARCH_INCLUDE_PATH,
                                                  hintedIndex);
            if (file) {
                profiler_record_value("pp_count_include_path_hint_preancestor_hit", 1);
                origin = INCLUDE_SEARCH_INCLUDE_PATH;
                originIndex = hintedIndex;
                if (originOut) *originOut = origin;
                if (originIndexOut) *originIndexOut = originIndex;
                return file;
            }
        }
    }

    // 3) Project ancestor search for quoted includes without a local hit.
    if (!isSystem && !isIncludeNext && includingFile) {
        const IncludeFile* ancestorFile = ir_search_ancestor_dirs(resolver,
                                                                  includingFile,
                                                                  name,
                                                                  originOut,
                                                                  originIndexOut);
        if (ancestorFile) {
            return ancestorFile;
        }
    }

    // 4) Project include paths
    for (size_t i = startIdx; i < resolver->includePathCount; ++i) {
        if (hintedIndexTried && i == hintedIndex) continue;
        if (ir_build_path(candidate, sizeof(candidate), resolver->includePaths[i], name)) {
            ir_set_trace_phase(resolver, INCLUDE_PROFILE_PHASE_INCLUDE_PATH);
            const IncludeFile* file = ir_try_load(resolver,
                                                  candidate,
                                                  INCLUDE_SEARCH_INCLUDE_PATH,
                                                  i);
            if (file) {
                (void)ir_cache_include_path_hint(resolver, name, isSystem, isIncludeNext, i);
                origin = INCLUDE_SEARCH_INCLUDE_PATH;
                originIndex = i;
                if (originOut) *originOut = origin;
                if (originIndexOut) *originIndexOut = originIndex;
                return file;
            }
        }
    }

    // 5) Fallback: raw name
    if (isSystem) {
        ir_set_trace_phase(resolver, INCLUDE_PROFILE_PHASE_RAW);
        const IncludeFile* frameworkFile = ir_try_framework_header(resolver, name, originOut, originIndexOut);
        if (frameworkFile) {
            return frameworkFile;
        }
    }

    origin = INCLUDE_SEARCH_RAW;
    originIndex = (size_t)-1;
    ir_set_trace_phase(resolver, INCLUDE_PROFILE_PHASE_RAW);
    const IncludeFile* file = ir_try_load(resolver, name, origin, originIndex);
    if (file) {
        if (originOut) *originOut = origin;
        if (originIndexOut) *originIndexOut = originIndex;
        return file;
    }
    if (!isSystem) {
        const IncludeFile* sharedFile = ir_try_shared_workspace_header(resolver,
                                                                       includingFile,
                                                                       name,
                                                                       originOut,
                                                                       originIndexOut);
        if (sharedFile) {
            return sharedFile;
        }
    }
    return NULL;
}

const IncludeFile* include_resolver_load(IncludeResolver* resolver,
                                         const char* includingFile,
                                         const char* name,
                                         bool isSystem,
                                         bool isIncludeNext,
                                         IncludeSearchOrigin* originOut,
                                         size_t* originIndexOut) {
    if (!resolver || !name) return NULL;
    profiler_record_value("pp_count_include_resolver_load_calls", 1);

    // Fast path for resolver-seeded files (for example, in-memory root buffers)
    // so callers are not forced to provide a filesystem-backed path.
    const IncludeFile* seeded = ir_lookup_exact_path(resolver, name);
    if (seeded) {
        if (originOut) *originOut = seeded->origin;
        if (originIndexOut) *originIndexOut = seeded->originIndex;
        return seeded;
    }

    if (isSystem) {
        const IncludeFile* virtualAudioToolbox = ir_try_virtual_audio_toolbox(resolver, name);
        if (virtualAudioToolbox) {
            if (originOut) *originOut = INCLUDE_SEARCH_RAW;
            if (originIndexOut) *originIndexOut = (size_t)-1;
            return virtualAudioToolbox;
        }
        const IncludeFile* virtualTgmath = ir_try_virtual_tgmath(resolver, name);
        if (virtualTgmath) {
            if (originOut) *originOut = INCLUDE_SEARCH_RAW;
            if (originIndexOut) *originIndexOut = (size_t)-1;
            return virtualTgmath;
        }
    }

    size_t parentFileIndex = (size_t)-1;
    bool canUseRequestCache = true;
    char parentDir[4096];
    parentDir[0] = '\0';
    bool useParentDirKey = includingFile && !isSystem && !isIncludeNext;
    if (useParentDirKey) {
        if (!ir_build_parent_dir(parentDir, sizeof(parentDir), includingFile)) {
            canUseRequestCache = false;
        }
    } else if (includingFile && (!isSystem || isIncludeNext)) {
        parentFileIndex = ir_lookup_exact_index(resolver, includingFile);
        if (parentFileIndex == (size_t)-1) {
            canUseRequestCache = false;
        }
    }

    if (canUseRequestCache) {
        size_t cacheIndex = ir_lookup_request_cache_index(resolver,
                                                          parentFileIndex,
                                                          useParentDirKey ? parentDir : NULL,
                                                          name,
                                                          isSystem,
                                                          isIncludeNext);
        if (cacheIndex != (size_t)-1) {
            const IncludeRequestCacheEntry* entry = &resolver->requestCache[cacheIndex];
            if (entry->fileIndex < resolver->count) {
                profiler_record_value("pp_count_resolver_front_cache_hit", 1);
                if (originOut) *originOut = entry->origin;
                if (originIndexOut) *originIndexOut = entry->originIndex;
                return &resolver->files[entry->fileIndex];
            }
        }
    }

    if (ir_path_is_absolute(name)) {
        char* canonicalName = ir_canonicalize_path(name);
        if (canonicalName) {
            seeded = ir_lookup_by_canonical_path(resolver, canonicalName);
            free(canonicalName);
            if (seeded) {
                if (originOut) *originOut = seeded->origin;
                if (originIndexOut) *originIndexOut = seeded->originIndex;
                return seeded;
            }
        }
    }

    if (canUseRequestCache) {
        profiler_record_value("pp_count_resolver_front_cache_miss", 1);
    }
    IncludeLoadAttemptTrace trace = {0};
    trace.currentPhase = ir_phase_from_origin(INCLUDE_SEARCH_RAW);
    if (resolver->headerProfileEnabled) {
        resolver->activeTrace = &trace;
    }
    const IncludeFile* file = ir_search_and_load(resolver,
                                                 includingFile,
                                                 name,
                                                 isSystem,
                                                 isIncludeNext,
                                                 originOut,
                                                 originIndexOut);
    if (resolver->headerProfileEnabled) {
        resolver->activeTrace = NULL;
    }
    if (file && trace.loadedNewFile) {
        ir_record_profile_entry(resolver, file, &trace);
    }
    if (file && canUseRequestCache) {
        size_t fileIndex = (size_t)(file - resolver->files);
        IncludeSearchOrigin cachedOrigin = originOut ? *originOut : file->origin;
        size_t cachedOriginIndex = originIndexOut ? *originIndexOut : file->originIndex;
        (void)ir_cache_request_result(resolver,
                                      parentFileIndex,
                                      useParentDirKey ? parentDir : NULL,
                                      name,
                                      isSystem,
                                      isIncludeNext,
                                      fileIndex,
                                      cachedOrigin,
                                      cachedOriginIndex);
    }
    return file;
}

void include_resolver_mark_pragma_once(IncludeResolver* resolver, const char* resolvedPath) {
    if (!resolver || !resolvedPath) return;
    IncludeFile* file = (IncludeFile*)ir_lookup_exact_path(resolver, resolvedPath);
    if (file) {
        file->pragmaOnce = true;
    }
}

void include_resolver_mark_included(IncludeResolver* resolver, const char* resolvedPath) {
    if (!resolver || !resolvedPath) return;
    IncludeFile* file = (IncludeFile*)ir_lookup_exact_path(resolver, resolvedPath);
    if (file) {
        file->includedOnce = true;
    }
}

bool include_resolver_was_included(const IncludeResolver* resolver, const char* resolvedPath) {
    const IncludeFile* file = ir_lookup_exact_path(resolver, resolvedPath);
    return file ? file->includedOnce : false;
}

const char* include_resolver_get_cached_guard(const IncludeResolver* resolver, const char* resolvedPath) {
    const IncludeFile* file = ir_lookup_exact_path(resolver, resolvedPath);
    return file ? file->cachedGuardName : NULL;
}

void include_resolver_cache_guard(IncludeResolver* resolver, const char* resolvedPath, const char* guardName) {
    if (!resolver || !resolvedPath || !guardName || !guardName[0]) return;
    IncludeFile* file = (IncludeFile*)ir_lookup_exact_path(resolver, resolvedPath);
    if (!file) return;
    if (file->cachedGuardName && strcmp(file->cachedGuardName, guardName) == 0) {
        return;
    }
    char* copy = ir_strdup(guardName);
    if (!copy) return;
    free(file->cachedGuardName);
    file->cachedGuardName = copy;
}

void include_resolver_cache_summary_probe(IncludeResolver* resolver,
                                          const char* resolvedPath,
                                          IncludeSummaryProbe probe) {
    if (!resolver || !resolvedPath) return;
    IncludeFile* file = (IncludeFile*)ir_lookup_exact_path(resolver, resolvedPath);
    if (!file) return;
    file->summaryProbe = probe;
}

void include_resolver_cache_summary_actions(IncludeResolver* resolver,
                                            const char* resolvedPath,
                                            const IncludeSummaryAction* actions,
                                            size_t actionCount) {
    if (!resolver || !resolvedPath) return;
    IncludeFile* file = (IncludeFile*)ir_lookup_exact_path(resolver, resolvedPath);
    if (!file) return;

    IncludeSummaryAction* copy = NULL;
    if (actions && actionCount > 0) {
        copy = (IncludeSummaryAction*)malloc(actionCount * sizeof(IncludeSummaryAction));
        if (!copy) return;
        memcpy(copy, actions, actionCount * sizeof(IncludeSummaryAction));
    }

    free(file->summaryActions);
    file->summaryActions = copy;
    file->summaryActionCount = copy ? actionCount : 0;
}

const IncludeSummaryAction* include_resolver_get_cached_summary_actions(const IncludeResolver* resolver,
                                                                        const char* resolvedPath,
                                                                        size_t* actionCountOut) {
    const IncludeFile* file = ir_lookup_exact_path(resolver, resolvedPath);
    if (!file) {
        if (actionCountOut) *actionCountOut = 0;
        return NULL;
    }
    if (actionCountOut) *actionCountOut = file->summaryActionCount;
    return file->summaryActions;
}
