// SPDX-License-Identifier: Apache-2.0

#ifndef PREPROCESSOR_INCLUDE_RESOLVER_H
#define PREPROCESSOR_INCLUDE_RESOLVER_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    INCLUDE_SEARCH_SAME_DIR = 0,
    INCLUDE_SEARCH_INCLUDE_PATH = 1,
    INCLUDE_SEARCH_RAW = 2
} IncludeSearchOrigin;

typedef enum {
    INCLUDE_SUMMARY_PROBE_UNKNOWN = 0,
    INCLUDE_SUMMARY_PROBE_CANDIDATE = 1,
    INCLUDE_SUMMARY_PROBE_REJECT_NON_LITERAL_INCLUDE = 2,
    INCLUDE_SUMMARY_PROBE_REJECT_UNSUPPORTED_DIRECTIVE = 3
} IncludeSummaryProbeStatus;

typedef enum {
    INCLUDE_HEADER_BEHAVIOR_UNKNOWN = 0,
    INCLUDE_HEADER_BEHAVIOR_ROUTER_ONLY = 1,
    INCLUDE_HEADER_BEHAVIOR_ROUTER_RAW_TAIL = 2,
    INCLUDE_HEADER_BEHAVIOR_INCLUDE_DEFINE_SCAFFOLD = 3,
    INCLUDE_HEADER_BEHAVIOR_CONDITIONAL_SCAFFOLD = 4,
    INCLUDE_HEADER_BEHAVIOR_GENERAL_FALLBACK = 5
} IncludeHeaderBehaviorClass;

typedef struct {
    IncludeSummaryProbeStatus status;
    size_t directiveCount;
    size_t includeCount;
    size_t defineCount;
    size_t conditionalCount;
    IncludeHeaderBehaviorClass behaviorClass;
    bool routerOnly;
    bool routerRawTail;
    bool routerHasPragmaOnce;
} IncludeSummaryProbe;

typedef enum {
    INCLUDE_SUMMARY_ACTION_RAW_RANGE = 0,
    INCLUDE_SUMMARY_ACTION_DEFINE,
    INCLUDE_SUMMARY_ACTION_INCLUDE,
    INCLUDE_SUMMARY_ACTION_INCLUDE_NEXT,
    INCLUDE_SUMMARY_ACTION_IF,
    INCLUDE_SUMMARY_ACTION_IFDEF,
    INCLUDE_SUMMARY_ACTION_IFNDEF,
    INCLUDE_SUMMARY_ACTION_ELIF,
    INCLUDE_SUMMARY_ACTION_ELSE,
    INCLUDE_SUMMARY_ACTION_ENDIF,
    INCLUDE_SUMMARY_ACTION_PRAGMA
} IncludeSummaryActionKind;

typedef struct {
    IncludeSummaryActionKind kind;
    size_t start;
    size_t end;
} IncludeSummaryAction;

typedef struct {
    char* path;
    char* contents;
    char* cachedGuardName;
    char* canonicalPath;
    IncludeSummaryProbe summaryProbe;
    IncludeSummaryAction* summaryActions;
    size_t summaryActionCount;
    long mtime;
    bool pragmaOnce;
    bool includedOnce;
    IncludeSearchOrigin origin;
    size_t originIndex; // include path index if origin==INCLUDE_SEARCH_INCLUDE_PATH, else SIZE_MAX
} IncludeFile;

typedef struct {
    size_t parentFileIndex; // SIZE_MAX when there is no includer context.
    char* includeName;
    bool isSystem;
    bool isIncludeNext;
    size_t fileIndex;
    IncludeSearchOrigin origin;
    size_t originIndex;
} IncludeRequestCacheEntry;

typedef struct {
    IncludeFile* files;
    size_t count;
    size_t capacity;
    IncludeRequestCacheEntry* requestCache;
    size_t requestCacheCount;
    size_t requestCacheCapacity;
    char** includePaths;
    size_t includePathCount;
} IncludeResolver;

IncludeResolver* include_resolver_create(const char* const* includePaths, size_t pathCount);
void include_resolver_destroy(IncludeResolver* resolver);

// Resolves and loads an include. Returns NULL on failure.
// If found, returns a pointer owned by the resolver cache.
// Lookup a cached file by resolved path (exact match). Returns NULL if not found.
const IncludeFile* include_resolver_lookup(const IncludeResolver* resolver, const char* path);

const IncludeFile* include_resolver_load(IncludeResolver* resolver,
                                         const char* includingFile,
                                         const char* name,
                                         bool isSystem,
                                         bool isIncludeNext,
                                         IncludeSearchOrigin* originOut,
                                         size_t* originIndexOut);

// Allows injecting an already-loaded root file (for in-memory buffers). The resolver takes ownership.
bool include_resolver_set_root_buffer(IncludeResolver* resolver,
                                      const char* path,
                                      char* contents_owned,
                                      long mtime);

// Record a #pragma once hit for the given resolved path.
void include_resolver_mark_pragma_once(IncludeResolver* resolver, const char* resolvedPath);
void include_resolver_mark_included(IncludeResolver* resolver, const char* resolvedPath);
bool include_resolver_was_included(const IncludeResolver* resolver, const char* resolvedPath);
const char* include_resolver_get_cached_guard(const IncludeResolver* resolver, const char* resolvedPath);
void include_resolver_cache_guard(IncludeResolver* resolver, const char* resolvedPath, const char* guardName);
void include_resolver_cache_summary_probe(IncludeResolver* resolver,
                                          const char* resolvedPath,
                                          IncludeSummaryProbe probe);
void include_resolver_cache_summary_actions(IncludeResolver* resolver,
                                            const char* resolvedPath,
                                            const IncludeSummaryAction* actions,
                                            size_t actionCount);
const IncludeSummaryAction* include_resolver_get_cached_summary_actions(const IncludeResolver* resolver,
                                                                        const char* resolvedPath,
                                                                        size_t* actionCountOut);

// Dependency graph: pairs of edges (parent -> child).
typedef struct {
    char* from;
    char* to;
} IncludeEdge;

typedef struct {
    IncludeEdge* edges;
    size_t count;
    size_t capacity;
} IncludeGraph;

void include_graph_init(IncludeGraph* graph);
void include_graph_destroy(IncludeGraph* graph);
bool include_graph_add(IncludeGraph* graph, const char* from, const char* to);
bool include_graph_clone(IncludeGraph* dest, const IncludeGraph* src);
bool include_graph_write_json(const IncludeGraph* graph, const char* outPath);

#endif /* PREPROCESSOR_INCLUDE_RESOLVER_H */
