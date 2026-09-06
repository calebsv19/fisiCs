// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "Compiler/semantic_project_manifest.h"
#include "fisics_frontend.h"

typedef struct FisicsSemanticProjectTranslationUnit {
    size_t translation_unit_index;
    char* source_path;
    uint64_t source_hash;
    size_t source_length;
    char* metadata_path;
    char* producer_version;
    char* analysis_contract_version;
    size_t record_count;
    size_t edge_count;
} FisicsSemanticProjectTranslationUnit;

typedef struct FisicsSemanticProjectRecord {
    size_t record_index;
    size_t translation_unit_index;
    size_t local_record_index;
    FisicsFrontendSemanticMetadataRecord record;
} FisicsSemanticProjectRecord;

typedef struct FisicsSemanticProjectEdge {
    size_t translation_unit_index;
    size_t local_edge_index;
    FisicsFrontendSemanticMetadataEdge edge;
} FisicsSemanticProjectEdge;

typedef struct FisicsSemanticProjectGraph {
    char* project_id;
    char* manifest_path;
    FisicsSemanticProjectTranslationUnit* translation_units;
    size_t translation_unit_count;
    char** allowed_external_targets;
    size_t allowed_external_target_count;
    FisicsSemanticProjectRecord* records;
    size_t record_count;
    FisicsSemanticProjectEdge* edges;
    size_t edge_count;
} FisicsSemanticProjectGraph;

bool fisics_semantic_project_graph_build(
    const FisicsSemanticProjectManifest* manifest,
    FisicsSemanticProjectGraph* out,
    FisicsSemanticProjectDiagnostic* diagnostic);

void fisics_semantic_project_graph_free(FisicsSemanticProjectGraph* graph);
