// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "Extensions/Units/units_model.h"

/*
 * Registry integrity contract:
 *
 * - `name` is the canonical exported unit name. It must be singular,
 *   lowercase ASCII, and `snake_case`.
 * - `symbol` is the canonical short spelling. It must be globally unique
 *   unless a future validator explicitly allows a narrow exception.
 * - `aliases` are accepted source spellings only. They are never exported as
 *   canonical names and must not collide with any other canonical name,
 *   symbol, or alias in the registry.
 * - Each unit belongs to exactly one semantic family and must carry the
 *   family's canonical `Dim8` signature exactly.
 * - `scale_to_canonical` is relative to the canonical unit of that family.
 *   Canonical units must use `1.0`.
 * - `offset_to_canonical` is reserved for future offset-bearing families.
 *   The current registry is expected to stay at `0.0` for all entries.
 */
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

typedef struct {
    size_t family_count;
    size_t unit_count;
    size_t alias_count;
    size_t issue_count;
} FisicsUnitRegistryAudit;

bool fisics_unit_lookup(const char* name, const FisicsUnitDef** outUnit);
bool fisics_unit_lookup_family(FisicsDimFamily family, const FisicsUnitFamilyDef** outFamily);
size_t fisics_unit_family_count(void);
const FisicsUnitFamilyDef* fisics_unit_family_at(size_t index);
bool fisics_unit_registry_validate_table(const FisicsUnitFamilyDef* families,
                                         size_t family_count,
                                         FisicsUnitRegistryAudit* outAudit);
bool fisics_unit_registry_validate(FisicsUnitRegistryAudit* outAudit);
