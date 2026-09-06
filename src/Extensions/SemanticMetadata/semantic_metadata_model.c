// SPDX-License-Identifier: Apache-2.0

#include "Extensions/SemanticMetadata/semantic_metadata_model.h"

#include <stdlib.h>
#include <string.h>

#include "Compiler/compiler_context.h"
#include "Extensions/extension_hooks.h"

void fisics_semantic_metadata_records_destroy(FisicsSemanticMetadataRecord* records,
                                              size_t count) {
    if (!records) return;
    for (size_t i = 0; i < count; ++i) {
        free(records[i].symbolName);
        free(records[i].authoredId);
        free(records[i].domain);
        free(records[i].role);
    }
    free(records);
}

void fisics_semantic_metadata_edges_destroy(FisicsSemanticMetadataEdge* edges,
                                            size_t count) {
    if (!edges) return;
    for (size_t i = 0; i < count; ++i) {
        free(edges[i].sourceSymbolName);
        free(edges[i].targetId);
    }
    free(edges);
}

static int compare_record_ptrs(const void* lhs, const void* rhs) {
    const FisicsSemanticMetadataRecord* a = *(const FisicsSemanticMetadataRecord* const*)lhs;
    const FisicsSemanticMetadataRecord* b = *(const FisicsSemanticMetadataRecord* const*)rhs;
    const char* af = a->location.start.file ? a->location.start.file : "";
    const char* bf = b->location.start.file ? b->location.start.file : "";
    int fileCmp = strcmp(af, bf);
    if (fileCmp != 0) return fileCmp;
    if (a->location.start.line != b->location.start.line) {
        return a->location.start.line < b->location.start.line ? -1 : 1;
    }
    if (a->location.start.column != b->location.start.column) {
        return a->location.start.column < b->location.start.column ? -1 : 1;
    }
    return strcmp(a->symbolName ? a->symbolName : "", b->symbolName ? b->symbolName : "");
}

static const char* shown_value(const char* value, bool conflicted) {
    if (conflicted) return "<conflicted>";
    return value ? value : "<unset>";
}

static int compare_edge_ptrs(const void* lhs, const void* rhs) {
    const FisicsSemanticMetadataEdge* a =
        *(const FisicsSemanticMetadataEdge* const*)lhs;
    const FisicsSemanticMetadataEdge* b =
        *(const FisicsSemanticMetadataEdge* const*)rhs;
    const char* af = a->sourceLocation.start.file
                         ? a->sourceLocation.start.file
                         : "";
    const char* bf = b->sourceLocation.start.file
                         ? b->sourceLocation.start.file
                         : "";
    int cmp = strcmp(af, bf);
    if (cmp != 0) return cmp;
    if (a->sourceLocation.start.line != b->sourceLocation.start.line) {
        return a->sourceLocation.start.line < b->sourceLocation.start.line ? -1 : 1;
    }
    if (a->sourceLocation.start.column != b->sourceLocation.start.column) {
        return a->sourceLocation.start.column < b->sourceLocation.start.column ? -1 : 1;
    }
    cmp = strcmp(a->sourceSymbolName ? a->sourceSymbolName : "",
                 b->sourceSymbolName ? b->sourceSymbolName : "");
    if (cmp != 0) return cmp;
    if (a->kind != b->kind) return a->kind < b->kind ? -1 : 1;
    return strcmp(a->targetId ? a->targetId : "", b->targetId ? b->targetId : "");
}

static const FisicsSemanticMetadataRecord* record_for_symbol(
    const FisicsExtensionState* state,
    const struct Symbol* symbol) {
    if (!state || !symbol) return NULL;
    for (size_t i = 0; i < state->semanticMetadataRecordCount; ++i) {
        if (state->semanticMetadataRecords[i].symbol == symbol) {
            return &state->semanticMetadataRecords[i];
        }
    }
    return NULL;
}

static const char* edge_kind_name(FisicsSemanticMetadataEdgeKind kind) {
    return kind == FISICS_METADATA_EDGE_CONSUMES ? "CONSUMES" : "PRODUCES";
}

static const char* reference_state_name(FisicsSemanticMetadataReferenceState state) {
    return state == FISICS_METADATA_REFERENCE_RESOLVED ? "resolved" : "external";
}

static void print_edge_source_identity(FILE* out,
                                       const FisicsSemanticMetadataRecord* record,
                                       const FisicsSemanticMetadataEdge* edge) {
    if (record && record->authoredId && !record->idConflicted) {
        fputs(record->authoredId, out);
        return;
    }
    const SourceRange* location = record ? &record->location : &edge->sourceLocation;
    const char* file = location->start.file ? location->start.file : "<unknown>";
    fprintf(out, "symbol:%s:%s:%d:%d:%s",
            edge->sourceIsFunction ? "function" : "variable",
            file,
            location->start.line,
            location->start.column,
            edge->sourceSymbolName ? edge->sourceSymbolName : "<unnamed>");
}

void fisics_extension_dump_semantic_metadata(const CompilerContext* ctx, FILE* out) {
    if (!out) return;
    size_t count = (ctx && ctx->extensionState)
                       ? ctx->extensionState->semanticMetadataRecordCount
                       : 0;
    if (count == 0) return;
    fprintf(out, "SemanticMetadata: %zu record(s)\n", count);

    const FisicsSemanticMetadataRecord** ordered =
        (const FisicsSemanticMetadataRecord**)malloc(count * sizeof(*ordered));
    if (!ordered) {
        fprintf(out, "  <metadata ordering unavailable: out of memory>\n");
        return;
    }
    for (size_t i = 0; i < count; ++i) {
        ordered[i] = &ctx->extensionState->semanticMetadataRecords[i];
    }
    qsort(ordered, count, sizeof(*ordered), compare_record_ptrs);
    for (size_t i = 0; i < count; ++i) {
        const FisicsSemanticMetadataRecord* record = ordered[i];
        bool conflicted = record->idConflicted || record->domainConflicted || record->roleConflicted;
        fprintf(out,
                "  - %s: %s [id=%s, domain=%s, role=%s, %s]\n",
                record->isFunction ? "function" : "variable",
                record->symbolName ? record->symbolName : "<unnamed>",
                shown_value(record->authoredId, record->idConflicted),
                shown_value(record->domain, record->domainConflicted),
                shown_value(record->role, record->roleConflicted),
                conflicted ? "conflicted" : "resolved");
    }
    free(ordered);

    size_t edgeCount = ctx->extensionState->semanticMetadataEdgeCount;
    if (edgeCount == 0) return;
    fprintf(out, "SemanticMetadataGraph: %zu edge(s)\n", edgeCount);
    const FisicsSemanticMetadataEdge** edgeOrder =
        (const FisicsSemanticMetadataEdge**)malloc(edgeCount * sizeof(*edgeOrder));
    if (!edgeOrder) {
        fprintf(out, "  <graph ordering unavailable: out of memory>\n");
        return;
    }
    for (size_t i = 0; i < edgeCount; ++i) {
        edgeOrder[i] = &ctx->extensionState->semanticMetadataEdges[i];
    }
    qsort(edgeOrder, edgeCount, sizeof(*edgeOrder), compare_edge_ptrs);
    for (size_t i = 0; i < edgeCount; ++i) {
        const FisicsSemanticMetadataEdge* edge = edgeOrder[i];
        const FisicsSemanticMetadataRecord* record =
            record_for_symbol(ctx->extensionState, edge->sourceSymbol);
        fprintf(out, "  - %s: ", edge_kind_name(edge->kind));
        print_edge_source_identity(out, record, edge);
        fprintf(out, " -> %s [%s]\n",
                edge->targetId ? edge->targetId : "<unset>",
                reference_state_name(edge->resolution));
    }
    free(edgeOrder);
}
