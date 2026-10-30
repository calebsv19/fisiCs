// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "Extensions/PhysicalSemantics/physical_policy.h"
uint64_t physical_policy_fingerprint(FisicsPhysicalPolicy *policy);
bool physical_policy_profiles(const FisicsJsonValue *root,
                              FisicsPhysicalPolicy *policy,
                              FisicsPhysicalLoadContext *load,
                              FisicsSemanticProjectDiagnostic *diagnostic);
