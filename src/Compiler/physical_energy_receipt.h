// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "Compiler/physical_receipt.h"
#include "Extensions/PhysicalSemantics/physical_energy_semantics.h"
void physical_energy_fact_write(FILE *, CompilerContext *, PhysicalFact);
bool physical_energy_fact_match(const FisicsJsonValue *,
                                const FisicsPhysicalPolicy *);
bool physical_energy_signature_match(const FisicsJsonValue *,
                                     const FisicsPhysicalPolicy *);
