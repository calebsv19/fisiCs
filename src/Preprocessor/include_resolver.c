#include "Preprocessor/include_resolver.h"

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
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long len = ftell(f);
    if (len < 0) { fclose(f); return NULL; }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return NULL; }
    char* buffer = malloc((size_t)len + 1);
    if (!buffer) { fclose(f); return NULL; }
    size_t read = fread(buffer, 1, (size_t)len, f);
    fclose(f);
    if (read != (size_t)len) { free(buffer); return NULL; }
    buffer[len] = '\0';
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

static const IncludeFile* ir_try_load(IncludeResolver* resolver, const char* path) {
    long mtime = 0;
    if (!ir_path_exists(path, &mtime)) return NULL;

    const IncludeFile* cached = ir_lookup(resolver, path);
    if (cached && cached->mtime == mtime) {
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
                                             bool isSystem) {
    char candidate[4096];
    // 1) If quoted include and includingFile provided, search its directory.
    if (!isSystem && includingFile) {
        const char* slash = strrchr(includingFile, '/');
        if (slash) {
            size_t dirLen = (size_t)(slash - includingFile);
            char dir[4096];
            if (dirLen < sizeof(dir)) {
                memcpy(dir, includingFile, dirLen);
                dir[dirLen] = '\0';
                if (ir_build_path(candidate, sizeof(candidate), dir, name)) {
                    const IncludeFile* file = ir_try_load(resolver, candidate);
                    if (file) return file;
                }
            }
        }
    }

    // 2) Project include paths
    for (size_t i = 0; i < resolver->includePathCount; ++i) {
        if (ir_build_path(candidate, sizeof(candidate), resolver->includePaths[i], name)) {
            const IncludeFile* file = ir_try_load(resolver, candidate);
            if (file) return file;
        }
    }

    // 3) Fallback: raw name
    if (ir_try_load(resolver, name)) {
        return ir_lookup(resolver, name);
    }
    return NULL;
}

const IncludeFile* include_resolver_load(IncludeResolver* resolver,
                                         const char* includingFile,
                                         const char* name,
                                         bool isSystem) {
    if (!resolver || !name) return NULL;
    const IncludeFile* cached = ir_lookup(resolver, name);
    if (cached) return cached;
    return ir_search_and_load(resolver, includingFile, name, isSystem);
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

static void fprint_json_escaped(FILE* f, const char* s) {
    if (!s) {
        fputs("\"\"", f);
        return;
    }
    fputc('"', f);
    for (const unsigned char* p = (const unsigned char*)s; *p; ++p) {
        if (*p == '\\' || *p == '"') {
            fputc('\\', f);
            fputc(*p, f);
        } else if (*p == '\n') {
            fputs("\\n", f);
        } else if (*p == '\r') {
            fputs("\\r", f);
        } else if (*p == '\t') {
            fputs("\\t", f);
        } else {
            fputc(*p, f);
        }
    }
    fputc('"', f);
}

bool include_graph_write_json(const IncludeGraph* graph, const char* outPath) {
    if (!graph || !outPath || outPath[0] == '\0') return false;
    FILE* f = fopen(outPath, "w");
    if (!f) return false;
    fputs("{\"edges\":[", f);
    for (size_t i = 0; i < graph->count; ++i) {
        if (i) fputc(',', f);
        fputs("{\"from\":", f);
        fprint_json_escaped(f, graph->edges[i].from);
        fputs(",\"to\":", f);
        fprint_json_escaped(f, graph->edges[i].to);
        fputc('}', f);
    }
    fputs("]}\n", f);
    fclose(f);
    return true;
}
