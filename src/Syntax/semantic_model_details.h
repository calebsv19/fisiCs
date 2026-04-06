// SPDX-License-Identifier: Apache-2.0

#ifndef SEMANTIC_MODEL_DETAILS_H
#define SEMANTIC_MODEL_DETAILS_H

#include "semantic_model.h"

#ifdef __cplusplus
extern "C" {
#endif

size_t semanticModelGetStructCount(const SemanticModel* model);
const Symbol* semanticModelGetStructByIndex(const SemanticModel* model, size_t index);
const Symbol* semanticModelLookupStruct(const SemanticModel* model, const char* name);

#ifdef __cplusplus
}
#endif

#endif // SEMANTIC_MODEL_DETAILS_H
