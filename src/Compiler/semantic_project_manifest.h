// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>
#include <stddef.h>

#define FISICS_SEMANTIC_PROJECT_MANIFEST_MAX_BYTES (1024u * 1024u)
#define FISICS_SEMANTIC_PROJECT_MAX_TRANSLATION_UNITS 4096u
#define FISICS_SEMANTIC_PROJECT_MAX_EXTERNAL_TARGETS 65536u

typedef struct FisicsSemanticProjectManifestUnit {
    char* metadata_path;
} FisicsSemanticProjectManifestUnit;

typedef struct FisicsSemanticProjectManifest {
    char* project_id;
    char* manifest_path;
    FisicsSemanticProjectManifestUnit* translation_units;
    size_t translation_unit_count;
    char** allowed_external_targets;
    size_t allowed_external_target_count;
} FisicsSemanticProjectManifest;

typedef struct FisicsSemanticProjectDiagnostic {
    char message[512];
} FisicsSemanticProjectDiagnostic;

bool fisics_semantic_project_manifest_load(
    const char* path,
    FisicsSemanticProjectManifest* out,
    FisicsSemanticProjectDiagnostic* diagnostic);

void fisics_semantic_project_manifest_free(FisicsSemanticProjectManifest* manifest);
