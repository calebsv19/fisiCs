// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "Compiler/semantic_project_manifest.h"

#define FISICS_PHYSICAL_PROJECT_MANIFEST_MAX_BYTES (1024u * 1024u)
#define FISICS_PHYSICAL_PROJECT_MAX_UNITS_RECEIPTS 4096u
#define FISICS_PHYSICAL_PROJECT_MAX_CATALOGS 1024u

typedef enum FisicsPhysicalCoveragePolicy {
    FISICS_PHYSICAL_COVERAGE_ALLOW_PARTIAL = 0,
    FISICS_PHYSICAL_COVERAGE_RELATIONSHIP_TARGETS,
    FISICS_PHYSICAL_COVERAGE_ALL_AUTHORED
} FisicsPhysicalCoveragePolicy;

typedef enum FisicsPhysicalEvidencePolicy {
    FISICS_PHYSICAL_EVIDENCE_REPORT_UNVERIFIED = 0,
    FISICS_PHYSICAL_EVIDENCE_REQUIRE_SOURCE
} FisicsPhysicalEvidencePolicy;

typedef struct FisicsPhysicalProjectManifest {
    char* project_id;
    char* manifest_path;
    char* semantic_graph_path;
    char** units_metadata_paths;
    size_t units_metadata_path_count;
    char** catalog_paths;
    size_t catalog_path_count;
    FisicsPhysicalCoveragePolicy coverage_policy;
    FisicsPhysicalEvidencePolicy evidence_policy;
} FisicsPhysicalProjectManifest;

bool fisics_physical_project_manifest_load(
    const char* path,
    FisicsPhysicalProjectManifest* out,
    FisicsSemanticProjectDiagnostic* diagnostic);

void fisics_physical_project_manifest_free(FisicsPhysicalProjectManifest* manifest);
