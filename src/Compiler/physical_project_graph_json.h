// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>

#include "Compiler/physical_project_graph.h"

bool fisics_physical_project_graph_write_json_atomic(
    const char* output_path,
    const FisicsPhysicalProjectManifest* manifest,
    const FisicsPhysicalProjectGraph* graph,
    FisicsSemanticProjectDiagnostic* diagnostic);
