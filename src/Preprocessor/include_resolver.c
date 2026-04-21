// SPDX-License-Identifier: Apache-2.0

#include "Preprocessor/include_resolver.h"
#include "Utils/profiler.h"
#include "core_io.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

static char* ir_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* copy = malloc(len);
    if (copy) memcpy(copy, s, len);
    return copy;
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

static bool ir_append_file(IncludeResolver* resolver, IncludeFile file);
static size_t ir_lookup_exact_index(const IncludeResolver* resolver, const char* path);
static size_t ir_lookup_canonical_index(const IncludeResolver* resolver, const char* canonicalPath);
static const IncludeFile* ir_lookup_exact_path(const IncludeResolver* resolver, const char* path);
static const IncludeFile* ir_lookup_by_canonical_path(const IncludeResolver* resolver,
                                                      const char* canonicalPath);
static size_t ir_lookup_request_cache_index(const IncludeResolver* resolver,
                                            size_t parentFileIndex,
                                            const char* name,
                                            bool isSystem,
                                            bool isIncludeNext);
static bool ir_cache_request_result(IncludeResolver* resolver,
                                    size_t parentFileIndex,
                                    const char* name,
                                    bool isSystem,
                                    bool isIncludeNext,
                                    size_t fileIndex,
                                    IncludeSearchOrigin origin,
                                    size_t originIndex);

static const IncludeFile* ir_try_virtual_audio_toolbox(IncludeResolver* resolver, const char* name) {
    static const char* kAudioToolboxName = "AudioToolbox/AudioToolbox.h";
    static const char* kVirtualPath = "<fisics>/AudioToolbox/AudioToolbox.h";
    static const char* kShim =
        "#ifndef FISICS_AUDIO_TOOLBOX_SHIM_H\n"
        "#define FISICS_AUDIO_TOOLBOX_SHIM_H\n"
        "typedef int OSStatus;\n"
        "typedef unsigned int UInt32;\n"
        "typedef unsigned char UInt8;\n"
        "typedef long CFIndex;\n"
        "typedef long long SInt64;\n"
        "typedef double Float64;\n"
        "typedef void* CFURLRef;\n"
        "typedef void* CFStringRef;\n"
        "typedef struct OpaqueExtAudioFile* ExtAudioFileRef;\n"
        "enum { noErr = 0 };\n"
        "enum { kExtAudioFileProperty_FileDataFormat = 1 };\n"
        "enum { kExtAudioFileProperty_FileLengthFrames = 2 };\n"
        "enum { kExtAudioFileProperty_ClientDataFormat = 3 };\n"
        "enum { kAudioFormatLinearPCM = 1819304813 };\n"
        "enum { kAudioFormatFlagIsFloat = 1u << 0 };\n"
        "enum { kAudioFormatFlagIsPacked = 1u << 3 };\n"
        "enum { kAudioFormatFlagsNativeEndian = 0u };\n"
        "typedef struct AudioStreamBasicDescription {\n"
        "  Float64 mSampleRate;\n"
        "  UInt32 mFormatID;\n"
        "  UInt32 mFormatFlags;\n"
        "  UInt32 mBytesPerPacket;\n"
        "  UInt32 mFramesPerPacket;\n"
        "  UInt32 mBytesPerFrame;\n"
        "  UInt32 mChannelsPerFrame;\n"
        "  UInt32 mBitsPerChannel;\n"
        "} AudioStreamBasicDescription;\n"
        "typedef struct AudioBuffer {\n"
        "  UInt32 mNumberChannels;\n"
        "  UInt32 mDataByteSize;\n"
        "  void* mData;\n"
        "} AudioBuffer;\n"
        "typedef struct AudioBufferList {\n"
        "  UInt32 mNumberBuffers;\n"
        "  AudioBuffer mBuffers[1];\n"
        "} AudioBufferList;\n"
        "CFURLRef CFURLCreateFromFileSystemRepresentation(void* alloc,\n"
        "                                                  const UInt8* buffer,\n"
        "                                                  CFIndex len,\n"
        "                                                  int isDir);\n"
        "void CFRelease(void* cf);\n"
        "const char* CFStringGetCStringPtr(CFStringRef str, unsigned int enc);\n"
        "OSStatus ExtAudioFileOpenURL(CFURLRef url, ExtAudioFileRef* outExtAudioFile);\n"
        "OSStatus ExtAudioFileGetProperty(ExtAudioFileRef inExtAudioFile,\n"
        "                                 UInt32 inPropertyID,\n"
        "                                 UInt32* ioPropertyDataSize,\n"
        "                                 void* outPropertyData);\n"
        "OSStatus ExtAudioFileSetProperty(ExtAudioFileRef inExtAudioFile,\n"
        "                                 UInt32 inPropertyID,\n"
        "                                 UInt32 inPropertyDataSize,\n"
        "                                 const void* inPropertyData);\n"
        "OSStatus ExtAudioFileRead(ExtAudioFileRef inExtAudioFile,\n"
        "                          UInt32* ioNumberFrames,\n"
        "                          AudioBufferList* ioData);\n"
        "OSStatus ExtAudioFileDispose(ExtAudioFileRef inExtAudioFile);\n"
        "#endif\n";

    if (!resolver || !name || strcmp(name, kAudioToolboxName) != 0) {
        return NULL;
    }
    const IncludeFile* cached = ir_lookup_exact_path(resolver, kVirtualPath);
    if (cached) return cached;

    IncludeFile file = {0};
    file.path = ir_strdup(kVirtualPath);
    file.contents = ir_strdup(kShim);
    file.cachedGuardName = NULL;
    file.canonicalPath = ir_strdup(kVirtualPath);
    file.summaryProbe = (IncludeSummaryProbe){0};
    file.summaryActions = NULL;
    file.summaryActionCount = 0;
    file.mtime = 0;
    file.pragmaOnce = true;
    file.includedOnce = false;
    file.origin = INCLUDE_SEARCH_RAW;
    file.originIndex = (size_t)-1;
    if (!file.path || !file.contents || !file.canonicalPath) {
        free(file.path);
        free(file.contents);
        free(file.canonicalPath);
        return NULL;
    }
    if (!ir_append_file(resolver, file)) {
        free(file.path);
        free(file.contents);
        free(file.canonicalPath);
        return NULL;
    }
    return &resolver->files[resolver->count - 1];
}

static bool ir_append_file(IncludeResolver* resolver, IncludeFile file) {
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

static const IncludeFile* ir_lookup_exact_path(const IncludeResolver* resolver, const char* path) {
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

static size_t ir_lookup_request_cache_index(const IncludeResolver* resolver,
                                            size_t parentFileIndex,
                                            const char* name,
                                            bool isSystem,
                                            bool isIncludeNext) {
    if (!resolver || !name) return (size_t)-1;
    for (size_t i = 0; i < resolver->requestCacheCount; ++i) {
        const IncludeRequestCacheEntry* entry = &resolver->requestCache[i];
        if (entry->parentFileIndex != parentFileIndex) continue;
        if (entry->isSystem != isSystem) continue;
        if (entry->isIncludeNext != isIncludeNext) continue;
        if (strcmp(entry->includeName, name) != 0) continue;
        return i;
    }
    return (size_t)-1;
}

static bool ir_cache_request_result(IncludeResolver* resolver,
                                    size_t parentFileIndex,
                                    const char* name,
                                    bool isSystem,
                                    bool isIncludeNext,
                                    size_t fileIndex,
                                    IncludeSearchOrigin origin,
                                    size_t originIndex) {
    if (!resolver || !name || fileIndex >= resolver->count) return false;

    size_t existingIndex = ir_lookup_request_cache_index(resolver,
                                                         parentFileIndex,
                                                         name,
                                                         isSystem,
                                                         isIncludeNext);
    if (existingIndex != (size_t)-1) {
        IncludeRequestCacheEntry* entry = &resolver->requestCache[existingIndex];
        entry->fileIndex = fileIndex;
        entry->origin = origin;
        entry->originIndex = originIndex;
        return true;
    }

    if (resolver->requestCacheCount == resolver->requestCacheCapacity) {
        size_t newCapacity = resolver->requestCacheCapacity ? resolver->requestCacheCapacity * 2 : 16;
        IncludeRequestCacheEntry* entries =
            realloc(resolver->requestCache, newCapacity * sizeof(IncludeRequestCacheEntry));
        if (!entries) return false;
        resolver->requestCache = entries;
        resolver->requestCacheCapacity = newCapacity;
    }

    char* includeName = ir_strdup(name);
    if (!includeName) return false;

    IncludeRequestCacheEntry entry = {0};
    entry.parentFileIndex = parentFileIndex;
    entry.includeName = includeName;
    entry.isSystem = isSystem;
    entry.isIncludeNext = isIncludeNext;
    entry.fileIndex = fileIndex;
    entry.origin = origin;
    entry.originIndex = originIndex;
    resolver->requestCache[resolver->requestCacheCount++] = entry;
    return true;
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
    IncludeFile file;
    file.path = ir_strdup(path);
    file.contents = contents_owned;
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

    for (int depth = 0; depth < 24; ++depth) {
        char candidate[4096];
        if (ir_build_path(candidate, sizeof(candidate), dir, name)) {
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

        char* up = strrchr(dir, '/');
        if (!up) break;
        if (up == dir) break;
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
    for (size_t i = 0; i < resolver->requestCacheCount; ++i) {
        free(resolver->requestCache[i].includeName);
    }
    free(resolver->requestCache);
    for (size_t i = 0; i < resolver->count; ++i) {
        free(resolver->files[i].path);
        free(resolver->files[i].contents);
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
    free(resolver);
}

static const IncludeFile* ir_try_load(IncludeResolver* resolver,
                                      const char* path,
                                      IncludeSearchOrigin origin,
                                      size_t originIndex) {
    profiler_record_value("pp_count_resolver_try_load", 1);
    char* canonical = ir_canonicalize_path(path);
    if (!canonical) return NULL;

    long mtime = 0;
    if (!ir_path_exists(canonical, &mtime)) {
        free(canonical);
        return NULL;
    }

    const IncludeFile* cached = ir_lookup_by_canonical_path(resolver, canonical);
    if (cached && cached->mtime == mtime) {
        profiler_record_value("pp_count_resolver_cache_hit", 1);
        free(canonical);
        // cached origin is authoritative; ignore requested origin/index
        return cached;
    }
    profiler_record_value("pp_count_resolver_cache_miss", 1);

    char* data = ir_read_file(canonical);
    if (!data) {
        free(canonical);
        return NULL;
    }

    IncludeFile file;
    file.path = ir_strdup(path);
    if (!file.path) {
        free(data);
        free(canonical);
        return NULL;
    }
    file.contents = data;
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
        return NULL;
    }
    profiler_record_value("pp_count_unique_files_loaded", 1);

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

    // 2) Project include paths
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

    // 3) Project include paths
    size_t startIdx = 0;
    if (isIncludeNext && parentOrigin == INCLUDE_SEARCH_INCLUDE_PATH && parentIndex != (size_t)-1) {
        startIdx = parentIndex + 1;
    }
    for (size_t i = startIdx; i < resolver->includePathCount; ++i) {
        if (ir_build_path(candidate, sizeof(candidate), resolver->includePaths[i], name)) {
            const IncludeFile* file = ir_try_load(resolver,
                                                  candidate,
                                                  INCLUDE_SEARCH_INCLUDE_PATH,
                                                  i);
            if (file) {
                origin = INCLUDE_SEARCH_INCLUDE_PATH;
                originIndex = i;
                if (originOut) *originOut = origin;
                if (originIndexOut) *originIndexOut = originIndex;
                return file;
            }
        }
    }

    // 4) Fallback: raw name
    if (isSystem) {
        const IncludeFile* frameworkFile = ir_try_framework_header(resolver, name, originOut, originIndexOut);
        if (frameworkFile) {
            return frameworkFile;
        }
    }

    origin = INCLUDE_SEARCH_RAW;
    originIndex = (size_t)-1;
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

    if (isSystem) {
        const IncludeFile* virtualAudioToolbox = ir_try_virtual_audio_toolbox(resolver, name);
        if (virtualAudioToolbox) {
            if (originOut) *originOut = INCLUDE_SEARCH_RAW;
            if (originIndexOut) *originIndexOut = (size_t)-1;
            return virtualAudioToolbox;
        }
    }

    size_t parentFileIndex = (size_t)-1;
    bool canUseRequestCache = true;
    if (includingFile && (!isSystem || isIncludeNext)) {
        parentFileIndex = ir_lookup_exact_index(resolver, includingFile);
        if (parentFileIndex == (size_t)-1) {
            canUseRequestCache = false;
        }
    }

    if (canUseRequestCache) {
        size_t cacheIndex = ir_lookup_request_cache_index(resolver,
                                                          parentFileIndex,
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

    profiler_record_value("pp_count_resolver_front_cache_miss", 1);
    const IncludeFile* file = ir_search_and_load(resolver,
                                                 includingFile,
                                                 name,
                                                 isSystem,
                                                 isIncludeNext,
                                                 originOut,
                                                 originIndexOut);
    if (file && canUseRequestCache) {
        size_t fileIndex = (size_t)(file - resolver->files);
        IncludeSearchOrigin cachedOrigin = originOut ? *originOut : file->origin;
        size_t cachedOriginIndex = originIndexOut ? *originIndexOut : file->originIndex;
        (void)ir_cache_request_result(resolver,
                                      parentFileIndex,
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

void include_graph_init(IncludeGraph* graph) {
    if (!graph) return;
    graph->edges = NULL;
    graph->count = 0;
    graph->capacity = 0;
}

void include_graph_destroy(IncludeGraph* graph) {
    if (!graph) return;
    for (size_t i = 0; i < graph->count; ++i) {
        free(graph->edges[i].from);
        free(graph->edges[i].to);
    }
    free(graph->edges);
    graph->edges = NULL;
    graph->count = 0;
    graph->capacity = 0;
}

bool include_graph_add(IncludeGraph* graph, const char* from, const char* to) {
    if (!graph || !from || !to) return false;
    for (size_t i = 0; i < graph->count; ++i) {
        if (graph->edges[i].from && graph->edges[i].to &&
            strcmp(graph->edges[i].from, from) == 0 &&
            strcmp(graph->edges[i].to, to) == 0) {
            return true; // already recorded
        }
    }
    if (graph->count == graph->capacity) {
        size_t newCap = graph->capacity ? graph->capacity * 2 : 8;
        IncludeEdge* edges = realloc(graph->edges, newCap * sizeof(IncludeEdge));
        if (!edges) return false;
        graph->edges = edges;
        graph->capacity = newCap;
    }
    graph->edges[graph->count].from = ir_strdup(from);
    graph->edges[graph->count].to = ir_strdup(to);
    if (!graph->edges[graph->count].from || !graph->edges[graph->count].to) {
        free(graph->edges[graph->count].from);
        free(graph->edges[graph->count].to);
        return false;
    }
    graph->count++;
    return true;
}

bool include_graph_clone(IncludeGraph* dest, const IncludeGraph* src) {
    if (!dest) return false;
    include_graph_init(dest);
    if (!src) return true;
    for (size_t i = 0; i < src->count; ++i) {
        if (!include_graph_add(dest, src->edges[i].from, src->edges[i].to)) {
            include_graph_destroy(dest);
            return false;
        }
    }
    return true;
}

typedef struct IncludeGraphJsonBuilder {
    char* data;
    size_t len;
    size_t cap;
} IncludeGraphJsonBuilder;

static bool graph_json_reserve(IncludeGraphJsonBuilder* b, size_t extra) {
    if (!b) return false;
    if (extra > SIZE_MAX - b->len) return false;
    size_t need = b->len + extra;
    if (need <= b->cap) return true;
    size_t newCap = b->cap ? b->cap : 256u;
    while (newCap < need) {
        if (newCap > SIZE_MAX / 2u) {
            newCap = need;
            break;
        }
        newCap *= 2u;
    }
    char* grown = (char*)realloc(b->data, newCap);
    if (!grown) return false;
    b->data = grown;
    b->cap = newCap;
    return true;
}

static bool graph_json_append_raw(IncludeGraphJsonBuilder* b, const char* s) {
    size_t add = 0;
    if (!b || !s) return false;
    add = strlen(s);
    if (!graph_json_reserve(b, add)) return false;
    memcpy(b->data + b->len, s, add);
    b->len += add;
    return true;
}

static bool graph_json_append_char(IncludeGraphJsonBuilder* b, char c) {
    if (!graph_json_reserve(b, 1u)) return false;
    b->data[b->len++] = c;
    return true;
}

static bool graph_json_append_escaped(IncludeGraphJsonBuilder* b, const char* s) {
    if (!s) {
        return graph_json_append_raw(b, "\"\"");
    }
    if (!graph_json_append_char(b, '"')) return false;
    for (const unsigned char* p = (const unsigned char*)s; *p; ++p) {
        if (*p == '\\' || *p == '"') {
            if (!graph_json_append_char(b, '\\')) return false;
            if (!graph_json_append_char(b, (char)*p)) return false;
        } else if (*p == '\n') {
            if (!graph_json_append_raw(b, "\\n")) return false;
        } else if (*p == '\r') {
            if (!graph_json_append_raw(b, "\\r")) return false;
        } else if (*p == '\t') {
            if (!graph_json_append_raw(b, "\\t")) return false;
        } else {
            if (!graph_json_append_char(b, (char)*p)) return false;
        }
    }
    return graph_json_append_char(b, '"');
}

bool include_graph_write_json(const IncludeGraph* graph, const char* outPath) {
    IncludeGraphJsonBuilder b = {0};
    CoreResult write_result = core_result_ok();
    if (!graph || !outPath || outPath[0] == '\0') return false;

    if (!graph_json_append_raw(&b, "{\"edges\":[")) goto fail;
    for (size_t i = 0; i < graph->count; ++i) {
        if (i && !graph_json_append_char(&b, ',')) goto fail;
        if (!graph_json_append_raw(&b, "{\"from\":")) goto fail;
        if (!graph_json_append_escaped(&b, graph->edges[i].from)) goto fail;
        if (!graph_json_append_raw(&b, ",\"to\":")) goto fail;
        if (!graph_json_append_escaped(&b, graph->edges[i].to)) goto fail;
        if (!graph_json_append_char(&b, '}')) goto fail;
    }
    if (!graph_json_append_raw(&b, "]}\n")) goto fail;
    write_result = core_io_write_all(outPath, b.data, b.len);
    free(b.data);
    return write_result.code == CORE_OK;

fail:
    free(b.data);
    return false;
}
