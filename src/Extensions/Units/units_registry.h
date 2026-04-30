// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "Extensions/Units/units_model.h"

typedef struct {
    const char* name;
    const char* symbol;
    FisicsDimFamily family;
    FisicsDim8 dim;
    double scale_to_canonical;
    double offset_to_canonical;
    const char* const* aliases;
    size_t alias_count;
} FisicsUnitDef;

typedef struct {
    FisicsDimFamily family;
    const char* family_name;
    const FisicsUnitDef* units;
    size_t unit_count;
} FisicsUnitFamilyDef;

bool fisics_unit_lookup(const char* name, const FisicsUnitDef** outUnit);
bool fisics_unit_lookup_family(FisicsDimFamily family, const FisicsUnitFamilyDef** outFamily);
size_t fisics_unit_family_count(void);
const FisicsUnitFamilyDef* fisics_unit_family_at(size_t index);
