// SPDX-License-Identifier: Apache-2.0

#include "Extensions/Units/units_conversion.h"

#include "Extensions/Units/units_model.h"

static bool unit_has_nonzero_scale(const FisicsUnitDef* unit) {
    return unit && unit->scale_to_canonical != 0.0;
}

bool fisics_unit_family_supports_explicit_conversion(FisicsDimFamily family) {
    switch (family) {
        case FISICS_DIM_FAMILY_LENGTH:
        case FISICS_DIM_FAMILY_MASS:
        case FISICS_DIM_FAMILY_TIME:
        case FISICS_DIM_FAMILY_CURRENT:
        case FISICS_DIM_FAMILY_AMOUNT:
        case FISICS_DIM_FAMILY_LUMINOUS:
        case FISICS_DIM_FAMILY_VELOCITY:
        case FISICS_DIM_FAMILY_ACCELERATION:
        case FISICS_DIM_FAMILY_FORCE:
        case FISICS_DIM_FAMILY_ENERGY:
        case FISICS_DIM_FAMILY_POWER:
        case FISICS_DIM_FAMILY_PRESSURE:
        case FISICS_DIM_FAMILY_CHARGE:
        case FISICS_DIM_FAMILY_VOLTAGE:
        case FISICS_DIM_FAMILY_RESISTANCE:
            return true;
        case FISICS_DIM_FAMILY_DIMENSIONLESS:
        case FISICS_DIM_FAMILY_TEMPERATURE:
        case FISICS_DIM_FAMILY_CUSTOM:
        default:
            return false;
    }
}

bool fisics_unit_can_convert(const FisicsUnitDef* source,
                             const FisicsUnitDef* target,
                             const char** detailOut) {
    if (detailOut) *detailOut = NULL;
    if (!source || !target) {
        if (detailOut) *detailOut = "explicit conversion requires resolved source and target concrete units";
        return false;
    }
    if (source->family != target->family) {
        if (detailOut) *detailOut = "explicit conversion requires source and target units from the same family";
        return false;
    }
    if (!fisics_dim_equal(source->dim, target->dim)) {
        if (detailOut) *detailOut = "explicit conversion requires source and target units with identical dimensions";
        return false;
    }
    if (!fisics_unit_family_supports_explicit_conversion(source->family)) {
        if (detailOut) *detailOut = "explicit conversion is currently enabled only for multiplicative unit families";
        return false;
    }
    if (!unit_has_nonzero_scale(source) || !unit_has_nonzero_scale(target)) {
        if (detailOut) *detailOut = "explicit conversion requires nonzero canonical scales for both units";
        return false;
    }
    return true;
}

bool fisics_unit_to_canonical(double sourceValue,
                              const FisicsUnitDef* unit,
                              double* outCanonical) {
    if (!unit || !outCanonical || !unit_has_nonzero_scale(unit)) return false;
    *outCanonical = sourceValue * unit->scale_to_canonical + unit->offset_to_canonical;
    return true;
}

bool fisics_unit_from_canonical(double canonicalValue,
                                const FisicsUnitDef* unit,
                                double* outTarget) {
    if (!unit || !outTarget || !unit_has_nonzero_scale(unit)) return false;
    *outTarget = (canonicalValue - unit->offset_to_canonical) / unit->scale_to_canonical;
    return true;
}

bool fisics_unit_convert_explicit(double sourceValue,
                                  const FisicsUnitDef* source,
                                  const FisicsUnitDef* target,
                                  double* outTarget,
                                  const char** detailOut) {
    if (detailOut) *detailOut = NULL;
    if (!outTarget) {
        if (detailOut) *detailOut = "explicit conversion requires a non-null destination";
        return false;
    }
    if (!fisics_unit_can_convert(source, target, detailOut)) {
        return false;
    }

    double canonical = 0.0;
    if (!fisics_unit_to_canonical(sourceValue, source, &canonical)) {
        if (detailOut) *detailOut = "failed to convert source value to canonical family space";
        return false;
    }
    if (!fisics_unit_from_canonical(canonical, target, outTarget)) {
        if (detailOut) *detailOut = "failed to convert canonical family value to target unit";
        return false;
    }
    return true;
}
