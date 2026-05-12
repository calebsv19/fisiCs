// SPDX-License-Identifier: Apache-2.0

#include "Preprocessor/include_resolver.h"
#include "core_io.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static char* irg_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1u;
    char* copy = (char*)malloc(len);
    if (copy) memcpy(copy, s, len);
    return copy;
}

static uint64_t include_graph_hash_string(const char* s) {
    uint64_t hash = 1469598103934665603ull;
    if (!s) return hash;
    for (const unsigned char* p = (const unsigned char*)s; *p; ++p) {
        hash ^= (uint64_t)(*p);
        hash *= 1099511628211ull;
    }
    return hash;
}

static uint64_t include_graph_hash_edge(const char* from, const char* to) {
    uint64_t hash = include_graph_hash_string(from);
    hash ^= 0x9e3779b97f4a7c15ull;
    hash *= 1099511628211ull;
    hash ^= include_graph_hash_string(to);
    hash *= 1099511628211ull;
    return hash;
}

static bool include_graph_hash_reserve(IncludeGraph* graph, size_t minCapacity) {
    if (!graph) return false;
    size_t newCapacity = graph->hashCapacity ? graph->hashCapacity : 16u;
    while (newCapacity < minCapacity) {
        if (newCapacity > SIZE_MAX / 2u) {
            newCapacity = minCapacity;
            break;
        }
        newCapacity *= 2u;
    }
    size_t* slots = (size_t*)calloc(newCapacity, sizeof(size_t));
    if (!slots) return false;
    for (size_t i = 0; i < graph->count; ++i) {
        uint64_t hash = include_graph_hash_edge(graph->edges[i].from, graph->edges[i].to);
        size_t mask = newCapacity - 1u;
        size_t slot = (size_t)(hash & (uint64_t)mask);
        while (slots[slot] != 0u) {
            slot = (slot + 1u) & mask;
        }
        slots[slot] = i + 1u;
    }
    free(graph->hashSlots);
    graph->hashSlots = slots;
    graph->hashCapacity = newCapacity;
    return true;
}

static bool include_graph_hash_ensure_for_insert(IncludeGraph* graph) {
    if (!graph) return false;
    size_t minCapacity = graph->hashCapacity;
    if (minCapacity == 0u) {
        minCapacity = 16u;
    }
    while ((graph->count + 1u) * 10u >= minCapacity * 7u) {
        if (minCapacity > SIZE_MAX / 2u) {
            break;
        }
        minCapacity *= 2u;
    }
    if (graph->hashCapacity == minCapacity) return true;
    return include_graph_hash_reserve(graph, minCapacity);
}

static size_t include_graph_find_slot(const IncludeGraph* graph,
                                      const char* from,
                                      const char* to,
                                      bool* found) {
    if (found) *found = false;
    if (!graph || graph->hashCapacity == 0u || !graph->hashSlots) {
        return 0u;
    }
    size_t mask = graph->hashCapacity - 1u;
    uint64_t hash = include_graph_hash_edge(from, to);
    size_t slot = (size_t)(hash & (uint64_t)mask);
    while (graph->hashSlots[slot] != 0u) {
        size_t edgeIndex = graph->hashSlots[slot] - 1u;
        if (edgeIndex < graph->count &&
            graph->edges[edgeIndex].from && graph->edges[edgeIndex].to &&
            strcmp(graph->edges[edgeIndex].from, from) == 0 &&
            strcmp(graph->edges[edgeIndex].to, to) == 0) {
            if (found) *found = true;
            return slot;
        }
        slot = (slot + 1u) & mask;
    }
    return slot;
}

void include_graph_init(IncludeGraph* graph) {
    if (!graph) return;
    graph->edges = NULL;
    graph->count = 0;
    graph->capacity = 0;
    graph->hashSlots = NULL;
    graph->hashCapacity = 0;
}

void include_graph_destroy(IncludeGraph* graph) {
    if (!graph) return;
    for (size_t i = 0; i < graph->count; ++i) {
        free(graph->edges[i].from);
        free(graph->edges[i].to);
    }
    free(graph->edges);
    free(graph->hashSlots);
    graph->edges = NULL;
    graph->count = 0;
    graph->capacity = 0;
    graph->hashSlots = NULL;
    graph->hashCapacity = 0;
}

bool include_graph_add(IncludeGraph* graph, const char* from, const char* to) {
    if (!graph || !from || !to) return false;
    if (!include_graph_hash_ensure_for_insert(graph)) return false;
    bool found = false;
    size_t slot = include_graph_find_slot(graph, from, to, &found);
    if (found) return true;
    if (graph->count == graph->capacity) {
        size_t newCap = graph->capacity ? graph->capacity * 2u : 8u;
        IncludeEdge* edges = (IncludeEdge*)realloc(graph->edges, newCap * sizeof(IncludeEdge));
        if (!edges) return false;
        graph->edges = edges;
        graph->capacity = newCap;
    }
    graph->edges[graph->count].from = irg_strdup(from);
    graph->edges[graph->count].to = irg_strdup(to);
    if (!graph->edges[graph->count].from || !graph->edges[graph->count].to) {
        free(graph->edges[graph->count].from);
        free(graph->edges[graph->count].to);
        return false;
    }
    graph->hashSlots[slot] = graph->count + 1u;
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
