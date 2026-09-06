// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "fisics_frontend.h"

bool fisics_units_metadata_write_json_atomic(
    const char* output_path,
    const char* source_path,
    uint64_t source_hash,
    size_t source_length,
    bool overlay_enabled,
    const FisicsUnitsAttachment* attachments,
    size_t attachment_count);
