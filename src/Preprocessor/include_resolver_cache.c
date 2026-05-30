// SPDX-License-Identifier: Apache-2.0

#include "Preprocessor/include_resolver_internal.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

size_t ir_lookup_include_path_hint_index(const IncludeResolver* resolver,
                                         const char* name,
                                         bool isSystem,
                                         bool isIncludeNext) {
    if (!resolver || !name) return (size_t)-1;
    for (size_t i = 0; i < resolver->includePathHintCount; ++i) {
        const IncludePathHintEntry* entry = &resolver->includePathHints[i];
        if (entry->isSystem != isSystem) continue;
        if (entry->isIncludeNext != isIncludeNext) continue;
        if (strcmp(entry->includeName, name) != 0) continue;
        return i;
    }
    return (size_t)-1;
}

static const char* ir_lookup_include_path_exact_alias_root(const char* name) {
    if (!name) return NULL;
    if (strcmp(name, "kit_workspace_authoring_ui.h") == 0) {
        return "kit_workspace_authoring";
    }
    return NULL;
}

size_t ir_lookup_include_path_exact_alias_index(const IncludeResolver* resolver,
                                                const char* name,
                                                size_t startIdx) {
    if (!resolver || !name) return (size_t)-1;

    const char* aliasRoot = ir_lookup_include_path_exact_alias_root(name);
    if (!aliasRoot || !aliasRoot[0]) return (size_t)-1;

    size_t aliasLen = strlen(aliasRoot);
    for (size_t i = startIdx; i < resolver->includePathCount; ++i) {
        const char* includePath = resolver->includePaths[i];
        if (!includePath || !includePath[0]) continue;

        const char* includeMarker = strstr(includePath, "/include");
        const char* segmentEnd = includeMarker ? includeMarker : includePath + strlen(includePath);
        while (segmentEnd > includePath && segmentEnd[-1] == '/') {
            segmentEnd--;
        }
        const char* segmentStart = segmentEnd;
        while (segmentStart > includePath && segmentStart[-1] != '/') {
            segmentStart--;
        }

        size_t segmentLen = (size_t)(segmentEnd - segmentStart);
        if (segmentLen != aliasLen) continue;
        if (strncmp(segmentStart, aliasRoot, aliasLen) != 0) continue;
        return i;
    }

    return (size_t)-1;
}

size_t ir_lookup_include_path_stem_match_index(const IncludeResolver* resolver,
                                               const char* name,
                                               size_t startIdx) {
    if (!resolver || !name) return (size_t)-1;
    if (strchr(name, '/')) return (size_t)-1;

    const char* dot = strrchr(name, '.');
    size_t stemLen = dot ? (size_t)(dot - name) : strlen(name);
    if (stemLen == 0) return (size_t)-1;

    for (size_t i = startIdx; i < resolver->includePathCount; ++i) {
        const char* includePath = resolver->includePaths[i];
        if (!includePath || !includePath[0]) continue;

        const char* includeMarker = strstr(includePath, "/include");
        const char* segmentEnd = includeMarker ? includeMarker : includePath + strlen(includePath);
        while (segmentEnd > includePath && segmentEnd[-1] == '/') {
            segmentEnd--;
        }
        const char* segmentStart = segmentEnd;
        while (segmentStart > includePath && segmentStart[-1] != '/') {
            segmentStart--;
        }

        size_t segmentLen = (size_t)(segmentEnd - segmentStart);
        if (segmentLen != stemLen) continue;
        if (strncmp(segmentStart, name, stemLen) != 0) continue;
        return i;
    }

    return (size_t)-1;
}

bool ir_cache_include_path_hint(IncludeResolver* resolver,
                                const char* name,
                                bool isSystem,
                                bool isIncludeNext,
                                size_t includePathIndex) {
    if (!resolver || !name) return false;
    size_t existingIndex = ir_lookup_include_path_hint_index(resolver, name, isSystem, isIncludeNext);
    if (existingIndex != (size_t)-1) {
        resolver->includePathHints[existingIndex].includePathIndex = includePathIndex;
        return true;
    }

    if (resolver->includePathHintCount == resolver->includePathHintCapacity) {
        size_t newCapacity = resolver->includePathHintCapacity ? resolver->includePathHintCapacity * 2 : 32;
        IncludePathHintEntry* entries =
            realloc(resolver->includePathHints, newCapacity * sizeof(*entries));
        if (!entries) return false;
        resolver->includePathHints = entries;
        resolver->includePathHintCapacity = newCapacity;
    }

    char* includeName = ir_strdup(name);
    if (!includeName) return false;

    IncludePathHintEntry entry = {
        .includeName = includeName,
        .isSystem = isSystem,
        .isIncludeNext = isIncludeNext,
        .includePathIndex = includePathIndex,
    };
    resolver->includePathHints[resolver->includePathHintCount++] = entry;
    return true;
}

bool ir_build_parent_dir(char* buffer, size_t bufSize, const char* path) {
    if (!buffer || bufSize == 0 || !path) return false;
    const char* slash = strrchr(path, '/');
    if (!slash) return false;
    size_t dirLen = (size_t)(slash - path);
    if (dirLen == 0 || dirLen >= bufSize) return false;
    memcpy(buffer, path, dirLen);
    buffer[dirLen] = '\0';
    return true;
}

static bool ir_request_cache_uses_parent_dir(bool isSystem, bool isIncludeNext) {
    return !isSystem && !isIncludeNext;
}

static uint64_t ir_hash_bytes(uint64_t seed, const char* value) {
    uint64_t hash = seed;
    if (!value) return hash;
    for (const unsigned char* p = (const unsigned char*)value; *p; ++p) {
        hash ^= (uint64_t)(*p);
        hash *= 1099511628211ull;
    }
    return hash;
}

static uint64_t ir_hash_size_t(uint64_t seed, size_t value) {
    uint64_t hash = seed;
    for (size_t i = 0; i < sizeof(size_t); ++i) {
        hash ^= (uint64_t)((value >> (i * 8u)) & 0xffu);
        hash *= 1099511628211ull;
    }
    return hash;
}

static uint64_t ir_request_cache_hash_key(size_t parentFileIndex,
                                          const char* parentDir,
                                          const char* name,
                                          bool isSystem,
                                          bool isIncludeNext) {
    uint64_t hash = 1469598103934665603ull;
    hash = ir_hash_size_t(hash, (size_t)isSystem);
    hash = ir_hash_size_t(hash, (size_t)isIncludeNext);
    if (ir_request_cache_uses_parent_dir(isSystem, isIncludeNext)) {
        hash = ir_hash_bytes(hash, parentDir);
    } else {
        hash = ir_hash_size_t(hash, parentFileIndex);
    }
    return ir_hash_bytes(hash, name);
}

static bool ir_request_cache_hash_reserve(IncludeResolver* resolver, size_t minCapacity) {
    if (!resolver) return false;
    size_t newCapacity = resolver->requestCacheHashCapacity ? resolver->requestCacheHashCapacity : 16u;
    while (newCapacity < minCapacity) {
        if (newCapacity > SIZE_MAX / 2u) {
            newCapacity = minCapacity;
            break;
        }
        newCapacity *= 2u;
    }
    size_t* slots = calloc(newCapacity, sizeof(size_t));
    if (!slots) return false;
    for (size_t i = 0; i < resolver->requestCacheCount; ++i) {
        const IncludeRequestCacheEntry* entry = &resolver->requestCache[i];
        uint64_t hash = ir_request_cache_hash_key(entry->parentFileIndex,
                                                  entry->parentDir,
                                                  entry->includeName,
                                                  entry->isSystem,
                                                  entry->isIncludeNext);
        size_t mask = newCapacity - 1u;
        size_t slot = (size_t)(hash & (uint64_t)mask);
        while (slots[slot] != 0u) {
            slot = (slot + 1u) & mask;
        }
        slots[slot] = i + 1u;
    }
    free(resolver->requestCacheHashSlots);
    resolver->requestCacheHashSlots = slots;
    resolver->requestCacheHashCapacity = newCapacity;
    return true;
}

static bool ir_request_cache_hash_ensure_for_insert(IncludeResolver* resolver) {
    if (!resolver) return false;
    size_t minCapacity = resolver->requestCacheHashCapacity;
    if (minCapacity == 0u) {
        minCapacity = 16u;
    }
    while ((resolver->requestCacheCount + 1u) * 10u >= minCapacity * 7u) {
        if (minCapacity > SIZE_MAX / 2u) break;
        minCapacity *= 2u;
    }
    if (resolver->requestCacheHashCapacity == minCapacity) return true;
    return ir_request_cache_hash_reserve(resolver, minCapacity);
}

static size_t ir_request_cache_find_slot(const IncludeResolver* resolver,
                                         size_t parentFileIndex,
                                         const char* parentDir,
                                         const char* name,
                                         bool isSystem,
                                         bool isIncludeNext,
                                         bool* found) {
    if (found) *found = false;
    if (!resolver || !name || resolver->requestCacheHashCapacity == 0u ||
        !resolver->requestCacheHashSlots) {
        return 0u;
    }
    bool useParentDir = ir_request_cache_uses_parent_dir(isSystem, isIncludeNext);
    uint64_t hash = ir_request_cache_hash_key(parentFileIndex, parentDir, name, isSystem, isIncludeNext);
    size_t mask = resolver->requestCacheHashCapacity - 1u;
    size_t slot = (size_t)(hash & (uint64_t)mask);
    while (resolver->requestCacheHashSlots[slot] != 0u) {
        size_t entryIndex = resolver->requestCacheHashSlots[slot] - 1u;
        if (entryIndex < resolver->requestCacheCount) {
            const IncludeRequestCacheEntry* entry = &resolver->requestCache[entryIndex];
            bool parentMatches = false;
            if (useParentDir) {
                parentMatches = parentDir && entry->parentDir && strcmp(entry->parentDir, parentDir) == 0;
            } else {
                parentMatches = entry->parentFileIndex == parentFileIndex;
            }
            if (parentMatches &&
                entry->isSystem == isSystem &&
                entry->isIncludeNext == isIncludeNext &&
                strcmp(entry->includeName, name) == 0) {
                if (found) *found = true;
                return slot;
            }
        }
        slot = (slot + 1u) & mask;
    }
    return slot;
}

size_t ir_lookup_request_cache_index(const IncludeResolver* resolver,
                                     size_t parentFileIndex,
                                     const char* parentDir,
                                     const char* name,
                                     bool isSystem,
                                     bool isIncludeNext) {
    if (!resolver || !name) return (size_t)-1;
    bool found = false;
    size_t slot = ir_request_cache_find_slot(resolver,
                                             parentFileIndex,
                                             parentDir,
                                             name,
                                             isSystem,
                                             isIncludeNext,
                                             &found);
    if (found) {
        return resolver->requestCacheHashSlots[slot] - 1u;
    }
    return (size_t)-1;
}

bool ir_cache_request_result(IncludeResolver* resolver,
                             size_t parentFileIndex,
                             const char* parentDir,
                             const char* name,
                             bool isSystem,
                             bool isIncludeNext,
                             size_t fileIndex,
                             IncludeSearchOrigin origin,
                             size_t originIndex) {
    if (!resolver || !name || fileIndex >= resolver->count) return false;
    bool useParentDir = ir_request_cache_uses_parent_dir(isSystem, isIncludeNext);

    size_t existingIndex = ir_lookup_request_cache_index(resolver,
                                                         parentFileIndex,
                                                         parentDir,
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

    if (!ir_request_cache_hash_ensure_for_insert(resolver)) {
        return false;
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
    char* parentDirCopy = NULL;
    if (useParentDir) {
        if (!parentDir || !parentDir[0]) {
            free(includeName);
            return false;
        }
        parentDirCopy = ir_strdup(parentDir);
        if (!parentDirCopy) {
            free(includeName);
            return false;
        }
    }

    IncludeRequestCacheEntry entry = {0};
    entry.parentFileIndex = parentFileIndex;
    entry.parentDir = parentDirCopy;
    entry.includeName = includeName;
    entry.isSystem = isSystem;
    entry.isIncludeNext = isIncludeNext;
    entry.fileIndex = fileIndex;
    entry.origin = origin;
    entry.originIndex = originIndex;
    size_t slot = ir_request_cache_find_slot(resolver,
                                             parentFileIndex,
                                             useParentDir ? parentDir : NULL,
                                             name,
                                             isSystem,
                                             isIncludeNext,
                                             NULL);
    resolver->requestCache[resolver->requestCacheCount] = entry;
    if (resolver->requestCacheHashSlots && resolver->requestCacheHashCapacity > 0u) {
        resolver->requestCacheHashSlots[slot] = resolver->requestCacheCount + 1u;
    }
    resolver->requestCacheCount++;
    return true;
}
