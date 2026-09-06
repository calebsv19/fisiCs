// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "Lexer/source_range.h"

struct CompilerContext;
struct Symbol;

typedef struct FisicsSemanticMetadataRecord {
    struct Symbol* symbol;
    char* symbolName;
    bool isFunction;
    SourceRange location;
    char* authoredId;
    char* domain;
    char* role;
    bool idConflicted;
    bool domainConflicted;
    bool roleConflicted;
} FisicsSemanticMetadataRecord;

typedef enum {
    FISICS_METADATA_EDGE_PRODUCES = 0,
    FISICS_METADATA_EDGE_CONSUMES
} FisicsSemanticMetadataEdgeKind;

typedef enum {
    FISICS_METADATA_REFERENCE_EXTERNAL = 0,
    FISICS_METADATA_REFERENCE_RESOLVED
} FisicsSemanticMetadataReferenceState;

typedef struct FisicsSemanticMetadataEdge {
    struct Symbol* sourceSymbol;
    char* sourceSymbolName;
    bool sourceIsFunction;
    SourceRange sourceLocation;
    FisicsSemanticMetadataEdgeKind kind;
    char* targetId;
    FisicsSemanticMetadataReferenceState resolution;
    SourceRange location;
} FisicsSemanticMetadataEdge;

void fisics_semantic_metadata_records_destroy(FisicsSemanticMetadataRecord* records,
                                              size_t count);
void fisics_semantic_metadata_edges_destroy(FisicsSemanticMetadataEdge* edges,
                                            size_t count);
void fisics_extension_dump_semantic_metadata(const struct CompilerContext* ctx,
                                             FILE* out);
