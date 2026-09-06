// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>

#include "Compiler/semantic_project_graph.h"

bool fisics_semantic_project_graph_write_json_atomic(
    const char* output_path,
    const FisicsSemanticProjectGraph* graph,
    FisicsSemanticProjectDiagnostic* diagnostic);
