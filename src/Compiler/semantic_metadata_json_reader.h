// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "Compiler/semantic_project_manifest.h"
#include "fisics_frontend.h"

#define FISICS_SEMANTIC_METADATA_ARTIFACT_MAX_BYTES (64u * 1024u * 1024u)
#define FISICS_SEMANTIC_PROJECT_MAX_TOTAL_ARTIFACT_BYTES (256u * 1024u * 1024u)
#define FISICS_SEMANTIC_PROJECT_MAX_RECORDS 65536u
#define FISICS_SEMANTIC_PROJECT_MAX_EDGES 262144u

typedef struct FisicsSemanticMetadataArtifact {
    char* metadata_path;
    char* producer_version;
    char* analysis_contract_version;
    char* source_path;
    uint64_t source_hash;
    size_t source_length;
    FisicsFrontendSemanticMetadataRecord* records;
    size_t record_count;
    FisicsFrontendSemanticMetadataEdge* edges;
    size_t edge_count;
} FisicsSemanticMetadataArtifact;

bool fisics_semantic_metadata_artifact_load(
    const char* path,
    FisicsSemanticMetadataArtifact* out,
    size_t* out_artifact_bytes,
    FisicsSemanticProjectDiagnostic* diagnostic);

void fisics_semantic_metadata_artifact_free(FisicsSemanticMetadataArtifact* artifact);
