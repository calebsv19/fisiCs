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

typedef struct {
    char* path;
    char* contents;
    long mtime;
    bool pragmaOnce;
    bool includedOnce;
    IncludeSearchOrigin origin;
    size_t originIndex; // include path index if origin==INCLUDE_SEARCH_INCLUDE_PATH, else SIZE_MAX
} IncludeFile;

typedef struct {
    IncludeFile* files;
    size_t count;
    size_t capacity;
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
