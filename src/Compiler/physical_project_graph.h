// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "Compiler/physical_project_manifest.h"
#include "Compiler/physical_contract_catalog.h"

typedef struct FisicsPhysicalSourceReceipt {
    char* artifact_path;
    char* source_path;
    uint64_t source_hash;
    size_t source_length;
    uint64_t registry_fingerprint;
} FisicsPhysicalSourceReceipt;

typedef struct FisicsPhysicalUnitsBinding {
    char* binding_id;
    char* source_path;
    char* symbol_stable_id;
    char* symbol_name;
    int start_line, start_column, end_line, end_column;
    int8_t dimension[8];
    char* dimension_text;
    char* unit_name;
    char* unit_symbol;
    char* unit_family;
    char* scale_binary64;
    char* offset_binary64;
    size_t record_index;
    bool has_record_index;
} FisicsPhysicalUnitsBinding;

typedef struct FisicsPhysicalSemanticRecord {
    char* source_identity;
    char* authored_id;
    char* symbol_stable_id;
    char* symbol_name;
    char* source_path;
    int start_line, start_column, end_line, end_column;
} FisicsPhysicalSemanticRecord;

typedef struct FisicsPhysicalSemanticEdge {
    size_t source_record_index;
    char* kind;
    char* target_id;
    size_t target_contract_index;
    bool has_target_contract_index;
} FisicsPhysicalSemanticEdge;

typedef struct FisicsPhysicalProjectGraph {
    char* project_id;
    char* manifest_path;
    char* semantic_graph_path;
    char* semantic_project_id;
    uint64_t registry_fingerprint;
    FisicsPhysicalSourceReceipt* receipts;
    size_t receipt_count;
    FisicsPhysicalUnitsBinding* bindings;
    size_t binding_count;
    FisicsPhysicalQuantityKind* quantity_kinds;
    size_t quantity_kind_count;
    FisicsPhysicalContract* contracts;
    size_t contract_count;
    FisicsPhysicalSemanticRecord* semantic_records;
    size_t semantic_record_count;
    FisicsPhysicalSemanticEdge* semantic_edges;
    size_t semantic_edge_count;
    size_t uncovered_record_count;
    size_t uncovered_target_count;
    size_t unverified_contract_count;
    size_t external_contract_count;
} FisicsPhysicalProjectGraph;

bool fisics_physical_project_graph_build(
    const FisicsPhysicalProjectManifest* manifest,
    FisicsPhysicalProjectGraph* out,
    FisicsSemanticProjectDiagnostic* diagnostic);

void fisics_physical_project_graph_free(FisicsPhysicalProjectGraph* graph);
