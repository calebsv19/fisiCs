#include "Preprocessor/include_resolver.h"
#include "core_io.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static char* ir_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* copy = malloc(len);
    if (copy) memcpy(copy, s, len);
    return copy;
}

static bool ir_path_exists(const char* path, long* mtimeOut) {
    struct stat st;
    if (stat(path, &st) != 0) return false;
    if (mtimeOut) *mtimeOut = (long)st.st_mtime;
    return true;
}

static char* ir_read_file(const char* path) {
    CoreBuffer file_data = {0};
    CoreResult read_result = core_io_read_all(path, &file_data);
    if (read_result.code != CORE_OK) return NULL;

    char* buffer = malloc(file_data.size + 1u);
    if (!buffer) {
        core_io_buffer_free(&file_data);
        return NULL;
    }
    if (file_data.size > 0u) {
        memcpy(buffer, file_data.data, file_data.size);
    }
    buffer[file_data.size] = '\0';
    core_io_buffer_free(&file_data);
    return buffer;
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

static const IncludeFile* ir_lookup(const IncludeResolver* resolver, const char* path) {
    if (!resolver || !path) return NULL;
    for (size_t i = 0; i < resolver->count; ++i) {
        if (strcmp(resolver->files[i].path, path) == 0) {
            return &resolver->files[i];
        }
    }
    return NULL;
}

const IncludeFile* include_resolver_lookup(const IncludeResolver* resolver, const char* path) {
    return ir_lookup(resolver, path);
}

bool include_resolver_set_root_buffer(IncludeResolver* resolver,
                                      const char* path,
                                      char* contents_owned,
                                      long mtime) {
    if (!resolver || !path || !contents_owned) return false;
    if (ir_lookup(resolver, path)) {
        free(contents_owned);
        return true;
    }
    IncludeFile file;
    file.path = ir_strdup(path);
    file.contents = contents_owned;
    file.mtime = mtime;
    file.pragmaOnce = false;
    file.includedOnce = false;
    file.origin = INCLUDE_SEARCH_RAW;
    file.originIndex = (size_t)-1;
    if (!file.path) {
        free(contents_owned);
        return false;
    }
    if (!ir_append_file(resolver, file)) {
        free(file.path);
        free(file.contents);
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
    for (size_t i = 0; i < resolver->count; ++i) {
        free(resolver->files[i].path);
        free(resolver->files[i].contents);
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
    long mtime = 0;
    if (!ir_path_exists(path, &mtime)) return NULL;

    const IncludeFile* cached = ir_lookup(resolver, path);
    if (cached && cached->mtime == mtime) {
        // cached origin is authoritative; ignore requested origin/index
        return cached;
    }

    char* data = ir_read_file(path);
    if (!data) return NULL;

    IncludeFile file;
    file.path = ir_strdup(path);
    file.contents = data;
    file.mtime = mtime;
    file.pragmaOnce = false;
    file.includedOnce = false;
    file.origin = origin;
    file.originIndex = originIndex;

    if (!ir_append_file(resolver, file)) {
        free(file.path);
        free(file.contents);
        return NULL;
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
        const IncludeFile* parent = ir_lookup(resolver, includingFile);
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

    // 3) Fallback: raw name
    origin = INCLUDE_SEARCH_RAW;
    originIndex = (size_t)-1;
    if (ir_try_load(resolver, name, origin, originIndex)) {
        if (originOut) *originOut = origin;
        if (originIndexOut) *originIndexOut = originIndex;
        return ir_lookup(resolver, name);
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
    const IncludeFile* cached = ir_lookup(resolver, name);
    if (cached) {
        if (originOut) *originOut = cached->origin;
        if (originIndexOut) *originIndexOut = cached->originIndex;
        return cached;
    }
    return ir_search_and_load(resolver,
                              includingFile,
                              name,
                              isSystem,
                              isIncludeNext,
                              originOut,
                              originIndexOut);
}

void include_resolver_mark_pragma_once(IncludeResolver* resolver, const char* resolvedPath) {
    if (!resolver || !resolvedPath) return;
    IncludeFile* file = (IncludeFile*)ir_lookup(resolver, resolvedPath);
    if (file) {
        file->pragmaOnce = true;
    }
}

void include_resolver_mark_included(IncludeResolver* resolver, const char* resolvedPath) {
    if (!resolver || !resolvedPath) return;
    IncludeFile* file = (IncludeFile*)ir_lookup(resolver, resolvedPath);
    if (file) {
        file->includedOnce = true;
    }
}

bool include_resolver_was_included(const IncludeResolver* resolver, const char* resolvedPath) {
    const IncludeFile* file = ir_lookup(resolver, resolvedPath);
    return file ? file->includedOnce : false;
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
