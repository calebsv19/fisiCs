// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "Extensions/PhysicalSemantics/physical_policy.h"
#include <stdio.h>
typedef struct PhysicalEnergyTransfer {
  char *id, *process;
  uint32_t source, target;
} PhysicalEnergyTransfer;
#define PHYSICAL_MAX_ENERGY_FORMS 128u
#define PHYSICAL_MAX_ENERGY_TRANSFERS 256u
uint32_t physical_energy_form_find(const FisicsPhysicalPolicy *, const char *);
uint32_t physical_energy_transfer_find(const FisicsPhysicalPolicy *,
                                       const char *);
bool physical_energy_load(const FisicsJsonValue *, FisicsPhysicalPolicy *,
                          FisicsPhysicalLoadContext *,
                          FisicsSemanticProjectDiagnostic *);
void physical_energy_free(FisicsPhysicalPolicy *);
void physical_energy_write(FILE *, const FisicsPhysicalPolicy *);
bool physical_energy_match(const FisicsJsonValue *,
                           const FisicsPhysicalPolicy *);
