#include <stdio.h>

#include "Extensions/Units/units_model.h"
#include "Extensions/Units/units_registry.h"

static int expect_valid(const char* label,
                        const FisicsUnitFamilyDef* families,
                        size_t family_count) {
    FisicsUnitRegistryAudit audit = {0};
    if (!fisics_unit_registry_validate_table(families, family_count, &audit)) {
        fprintf(stderr, "%s unexpectedly failed validation with %zu issue(s)\n",
                label, audit.issue_count);
        return 1;
    }
    if (audit.issue_count != 0) {
        fprintf(stderr, "%s unexpectedly reported %zu issue(s)\n",
                label, audit.issue_count);
        return 1;
    }
    return 0;
}

static int expect_invalid(const char* label,
                          const FisicsUnitFamilyDef* families,
                          size_t family_count) {
    FisicsUnitRegistryAudit audit = {0};
    if (fisics_unit_registry_validate_table(families, family_count, &audit)) {
        fprintf(stderr, "%s unexpectedly validated cleanly\n", label);
        return 1;
    }
    if (audit.issue_count == 0) {
        fprintf(stderr, "%s failed without reporting a concrete issue\n", label);
        return 1;
    }
    return 0;
}

int main(void) {
    static const char* const kMeterAliases[] = {"meters"};
    static const char* const kSecondAliases[] = {"seconds"};
    FisicsDim8 length = fisics_dim_length();
    FisicsDim8 time = fisics_dim_time();

    FisicsUnitDef valid_length_units[] = {
        {"meter", "m", FISICS_DIM_FAMILY_LENGTH, length, 1.0, 0.0, kMeterAliases, 1},
        {"foot", "ft", FISICS_DIM_FAMILY_LENGTH, length, 0.3048, 0.0, NULL, 0}
    };
    FisicsUnitFamilyDef valid_length_family[] = {
        {FISICS_DIM_FAMILY_LENGTH, "length", valid_length_units, 2}
    };
    if (expect_valid("valid-length-family", valid_length_family, 1)) return 1;

    FisicsUnitDef duplicate_name_units[] = {
        {"meter", "m", FISICS_DIM_FAMILY_LENGTH, length, 1.0, 0.0, NULL, 0},
        {"meter", "ft", FISICS_DIM_FAMILY_LENGTH, length, 0.3048, 0.0, NULL, 0}
    };
    FisicsUnitFamilyDef duplicate_name_family[] = {
        {FISICS_DIM_FAMILY_LENGTH, "length", duplicate_name_units, 2}
    };
    if (expect_invalid("duplicate-canonical-name", duplicate_name_family, 1)) return 1;

    static const char* const kAliasCollisionAliases[] = {"s"};
    FisicsUnitDef alias_collision_units[] = {
        {"meter", "m", FISICS_DIM_FAMILY_LENGTH, length, 1.0, 0.0, NULL, 0}
    };
    FisicsUnitDef alias_collision_time_units[] = {
        {"second", "s", FISICS_DIM_FAMILY_TIME, time, 1.0, 0.0, kSecondAliases, 1},
        {"minute", "min", FISICS_DIM_FAMILY_TIME, time, 60.0, 0.0, kAliasCollisionAliases, 1}
    };
    FisicsUnitFamilyDef alias_collision_families[] = {
        {FISICS_DIM_FAMILY_LENGTH, "length", alias_collision_units, 1},
        {FISICS_DIM_FAMILY_TIME, "time", alias_collision_time_units, 2}
    };
    if (expect_invalid("alias-collides-with-canonical-symbol", alias_collision_families, 2)) return 1;

    FisicsUnitDef duplicate_symbol_units[] = {
        {"meter", "m", FISICS_DIM_FAMILY_LENGTH, length, 1.0, 0.0, NULL, 0},
        {"mile", "m", FISICS_DIM_FAMILY_LENGTH, length, 1609.344, 0.0, NULL, 0}
    };
    FisicsUnitFamilyDef duplicate_symbol_family[] = {
        {FISICS_DIM_FAMILY_LENGTH, "length", duplicate_symbol_units, 2}
    };
    if (expect_invalid("duplicate-symbol", duplicate_symbol_family, 1)) return 1;

    FisicsUnitDef wrong_dim_units[] = {
        {"meter", "m", FISICS_DIM_FAMILY_LENGTH, time, 1.0, 0.0, NULL, 0},
        {"foot", "ft", FISICS_DIM_FAMILY_LENGTH, length, 0.3048, 0.0, NULL, 0}
    };
    FisicsUnitFamilyDef wrong_dim_family[] = {
        {FISICS_DIM_FAMILY_LENGTH, "length", wrong_dim_units, 2}
    };
    if (expect_invalid("family-dimension-mismatch", wrong_dim_family, 1)) return 1;

    FisicsUnitDef wrong_family_units[] = {
        {"meter", "m", FISICS_DIM_FAMILY_TIME, length, 1.0, 0.0, NULL, 0},
        {"foot", "ft", FISICS_DIM_FAMILY_LENGTH, length, 0.3048, 0.0, NULL, 0}
    };
    FisicsUnitFamilyDef wrong_family_table[] = {
        {FISICS_DIM_FAMILY_LENGTH, "length", wrong_family_units, 2}
    };
    if (expect_invalid("unit-family-field-mismatch", wrong_family_table, 1)) return 1;

    FisicsUnitDef invalid_scale_units[] = {
        {"meter", "m", FISICS_DIM_FAMILY_LENGTH, length, 1.0, 0.0, NULL, 0},
        {"foot", "ft", FISICS_DIM_FAMILY_LENGTH, length, 0.0, 0.0, NULL, 0}
    };
    FisicsUnitFamilyDef invalid_scale_family[] = {
        {FISICS_DIM_FAMILY_LENGTH, "length", invalid_scale_units, 2}
    };
    if (expect_invalid("nonpositive-scale", invalid_scale_family, 1)) return 1;

    FisicsUnitDef invalid_offset_units[] = {
        {"meter", "m", FISICS_DIM_FAMILY_LENGTH, length, 1.0, 1.0, NULL, 0},
        {"foot", "ft", FISICS_DIM_FAMILY_LENGTH, length, 0.3048, 0.0, NULL, 0}
    };
    FisicsUnitFamilyDef invalid_offset_family[] = {
        {FISICS_DIM_FAMILY_LENGTH, "length", invalid_offset_units, 2}
    };
    if (expect_invalid("nonzero-offset", invalid_offset_family, 1)) return 1;

    FisicsUnitDef wrong_family_name_units[] = {
        {"meter", "m", FISICS_DIM_FAMILY_LENGTH, length, 1.0, 0.0, NULL, 0},
        {"foot", "ft", FISICS_DIM_FAMILY_LENGTH, length, 0.3048, 0.0, NULL, 0}
    };
    FisicsUnitFamilyDef wrong_family_name_table[] = {
        {FISICS_DIM_FAMILY_LENGTH, "distance", wrong_family_name_units, 2}
    };
    if (expect_invalid("family-name-mismatch", wrong_family_name_table, 1)) return 1;

    FisicsUnitDef missing_canonical_units[] = {
        {"foot", "ft", FISICS_DIM_FAMILY_LENGTH, length, 0.3048, 0.0, NULL, 0},
        {"yard", "yd", FISICS_DIM_FAMILY_LENGTH, length, 0.9144, 0.0, NULL, 0}
    };
    FisicsUnitFamilyDef missing_canonical_table[] = {
        {FISICS_DIM_FAMILY_LENGTH, "length", missing_canonical_units, 2}
    };
    if (expect_invalid("missing-canonical-base", missing_canonical_table, 1)) return 1;

    return 0;
}
