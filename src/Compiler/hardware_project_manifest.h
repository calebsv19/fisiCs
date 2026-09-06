// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "Compiler/semantic_project_manifest.h"

#define FISICS_HARDWARE_PROJECT_MANIFEST_MAX_BYTES (1024u * 1024u)
#define FISICS_HARDWARE_PROJECT_MAX_CATALOGS 256u

typedef enum FisicsHardwarePhysicalEvidencePolicy {
  FISICS_HARDWARE_EVIDENCE_REQUIRE_SOURCE = 0,
  FISICS_HARDWARE_EVIDENCE_ALLOW_DECLARED
} FisicsHardwarePhysicalEvidencePolicy;

typedef enum FisicsHardwareRequiredPortPolicy {
  FISICS_HARDWARE_PORTS_REQUIRE_ALL = 0,
  FISICS_HARDWARE_PORTS_REPORT_UNBOUND
} FisicsHardwareRequiredPortPolicy;

typedef struct FisicsHardwareProjectManifest {
  char *project_id;
  char *manifest_path;
  char *physical_graph_path;
  char **catalog_paths;
  size_t catalog_path_count;
  char *topology_path;
  FisicsHardwarePhysicalEvidencePolicy evidence_policy;
  FisicsHardwareRequiredPortPolicy port_policy;
} FisicsHardwareProjectManifest;

bool fisics_hardware_project_manifest_load(
    const char *path, FisicsHardwareProjectManifest *out,
    FisicsSemanticProjectDiagnostic *diagnostic);

void fisics_hardware_project_manifest_free(
    FisicsHardwareProjectManifest *manifest);
