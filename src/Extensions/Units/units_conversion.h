// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>

#include "Extensions/Units/units_registry.h"

bool fisics_unit_family_supports_explicit_conversion(FisicsDimFamily family);
bool fisics_unit_can_convert(const FisicsUnitDef* source,
                             const FisicsUnitDef* target,
                             const char** detailOut);
bool fisics_unit_to_canonical(double sourceValue,
                              const FisicsUnitDef* unit,
                              double* outCanonical);
bool fisics_unit_from_canonical(double canonicalValue,
                                const FisicsUnitDef* unit,
                                double* outTarget);
bool fisics_unit_convert_explicit(double sourceValue,
                                  const FisicsUnitDef* source,
                                  const FisicsUnitDef* target,
                                  double* outTarget,
                                  const char** detailOut);
