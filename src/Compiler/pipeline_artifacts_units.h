// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "Syntax/semantic_model.h"
#include "fisics_frontend.h"

bool pipeline_collect_units_attachments(const SemanticModel* model,
                                        const FisicsSymbol* symbols,
                                        size_t symbol_count,
                                        FisicsUnitsAttachment** out_attachments,
                                        size_t* out_count);
