// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>

#include "Compiler/hardware_project_graph.h"

bool fisics_hardware_project_graph_write_json_atomic(
    const char *output_path, const FisicsHardwareProjectManifest *manifest,
    const FisicsHardwareProjectGraph *graph,
    FisicsSemanticProjectDiagnostic *diagnostic);
