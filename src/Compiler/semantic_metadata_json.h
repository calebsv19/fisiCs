// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "fisics_frontend.h"

bool fisics_semantic_metadata_write_json_atomic(
    const char* output_path,
    const char* source_path,
    uint64_t source_hash,
    size_t source_length,
    const FisicsFrontendSemanticMetadataRecord* records,
    size_t record_count,
    const FisicsFrontendSemanticMetadataEdge* edges,
    size_t edge_count);
