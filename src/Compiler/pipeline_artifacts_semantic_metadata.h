// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "fisics_frontend.h"

struct SemanticModel;

char* pipeline_normalize_semantic_metadata_source_path(const char* path);

uint64_t pipeline_compute_exported_symbol_stable_id(const FisicsSymbol* symbol);

bool pipeline_collect_semantic_metadata_projection(
    const struct SemanticModel* model,
    const FisicsSymbol* symbols,
    size_t symbol_count,
    const char* fallback_file_path,
    FisicsFrontendSemanticMetadataRecord** out_records,
    size_t* out_record_count,
    FisicsFrontendSemanticMetadataEdge** out_edges,
    size_t* out_edge_count);

void pipeline_free_semantic_metadata_projection(
    FisicsFrontendSemanticMetadataRecord* records,
    size_t record_count,
    FisicsFrontendSemanticMetadataEdge* edges,
    size_t edge_count);
