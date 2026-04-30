// SPDX-License-Identifier: Apache-2.0

#include "Extensions/Units/units_model.h"
#include "Extensions/Units/units_registry.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char* name;
    FisicsDimAtomKind kind;
    FisicsDimFamily family;
    FisicsDim8 dim;
} DimAtomEntry;

#define DIM_INIT(...) ((FisicsDim8){{__VA_ARGS__}})

static bool checked_cast_i8(int value, int8_t* out) {
    if (!out) return false;
    if (value < INT8_MIN || value > INT8_MAX) {
        return false;
    }
    *out = (int8_t)value;
    return true;
}

static FisicsDim8 dim_with_slot(FisicsDimSlot slot) {
    FisicsDim8 dim = {{0}};
    dim.e[slot] = 1;
    return dim;
}

static const char* family_name(FisicsDimFamily family) {
    switch (family) {
        case FISICS_DIM_FAMILY_DIMENSIONLESS: return "dimensionless";
        case FISICS_DIM_FAMILY_LENGTH: return "length";
        case FISICS_DIM_FAMILY_MASS: return "mass";
        case FISICS_DIM_FAMILY_TIME: return "time";
        case FISICS_DIM_FAMILY_CURRENT: return "current";
        case FISICS_DIM_FAMILY_TEMPERATURE: return "temperature";
        case FISICS_DIM_FAMILY_AMOUNT: return "amount";
        case FISICS_DIM_FAMILY_LUMINOUS: return "luminous";
        case FISICS_DIM_FAMILY_CUSTOM: return "custom";
        case FISICS_DIM_FAMILY_VELOCITY: return "velocity";
        case FISICS_DIM_FAMILY_ACCELERATION: return "acceleration";
        case FISICS_DIM_FAMILY_FORCE: return "force";
        case FISICS_DIM_FAMILY_ENERGY: return "energy";
        case FISICS_DIM_FAMILY_POWER: return "power";
        case FISICS_DIM_FAMILY_PRESSURE: return "pressure";
        case FISICS_DIM_FAMILY_CHARGE: return "charge";
        case FISICS_DIM_FAMILY_VOLTAGE: return "voltage";
        case FISICS_DIM_FAMILY_RESISTANCE: return "resistance";
        default: return "unknown";
    }
}

static const DimAtomEntry kDimAtoms[] = {
    { "1", FISICS_DIM_ATOM_DIMENSION, FISICS_DIM_FAMILY_DIMENSIONLESS, DIM_INIT(0, 0, 0, 0, 0, 0, 0, 0) },
    { "dimensionless", FISICS_DIM_ATOM_DIMENSION, FISICS_DIM_FAMILY_DIMENSIONLESS, DIM_INIT(0, 0, 0, 0, 0, 0, 0, 0) },
    { "scalar", FISICS_DIM_ATOM_DIMENSION, FISICS_DIM_FAMILY_DIMENSIONLESS, DIM_INIT(0, 0, 0, 0, 0, 0, 0, 0) },

    { "m", FISICS_DIM_ATOM_DIMENSION, FISICS_DIM_FAMILY_LENGTH, DIM_INIT(1, 0, 0, 0, 0, 0, 0, 0) },
    { "length", FISICS_DIM_ATOM_DIMENSION, FISICS_DIM_FAMILY_LENGTH, DIM_INIT(1, 0, 0, 0, 0, 0, 0, 0) },
    { "distance", FISICS_DIM_ATOM_DIMENSION, FISICS_DIM_FAMILY_LENGTH, DIM_INIT(1, 0, 0, 0, 0, 0, 0, 0) },
    { "displacement", FISICS_DIM_ATOM_DIMENSION, FISICS_DIM_FAMILY_LENGTH, DIM_INIT(1, 0, 0, 0, 0, 0, 0, 0) },

    { "kg", FISICS_DIM_ATOM_DIMENSION, FISICS_DIM_FAMILY_MASS, DIM_INIT(0, 1, 0, 0, 0, 0, 0, 0) },
    { "mass", FISICS_DIM_ATOM_DIMENSION, FISICS_DIM_FAMILY_MASS, DIM_INIT(0, 1, 0, 0, 0, 0, 0, 0) },

    { "s", FISICS_DIM_ATOM_DIMENSION, FISICS_DIM_FAMILY_TIME, DIM_INIT(0, 0, 1, 0, 0, 0, 0, 0) },
    { "time", FISICS_DIM_ATOM_DIMENSION, FISICS_DIM_FAMILY_TIME, DIM_INIT(0, 0, 1, 0, 0, 0, 0, 0) },
    { "duration", FISICS_DIM_ATOM_DIMENSION, FISICS_DIM_FAMILY_TIME, DIM_INIT(0, 0, 1, 0, 0, 0, 0, 0) },

    { "A", FISICS_DIM_ATOM_DIMENSION, FISICS_DIM_FAMILY_CURRENT, DIM_INIT(0, 0, 0, 1, 0, 0, 0, 0) },
    { "current", FISICS_DIM_ATOM_DIMENSION, FISICS_DIM_FAMILY_CURRENT, DIM_INIT(0, 0, 0, 1, 0, 0, 0, 0) },
    { "electric_current", FISICS_DIM_ATOM_DIMENSION, FISICS_DIM_FAMILY_CURRENT, DIM_INIT(0, 0, 0, 1, 0, 0, 0, 0) },

    { "K", FISICS_DIM_ATOM_DIMENSION, FISICS_DIM_FAMILY_TEMPERATURE, DIM_INIT(0, 0, 0, 0, 1, 0, 0, 0) },
    { "temperature", FISICS_DIM_ATOM_DIMENSION, FISICS_DIM_FAMILY_TEMPERATURE, DIM_INIT(0, 0, 0, 0, 1, 0, 0, 0) },

    { "mol", FISICS_DIM_ATOM_DIMENSION, FISICS_DIM_FAMILY_AMOUNT, DIM_INIT(0, 0, 0, 0, 0, 1, 0, 0) },
    { "amount", FISICS_DIM_ATOM_DIMENSION, FISICS_DIM_FAMILY_AMOUNT, DIM_INIT(0, 0, 0, 0, 0, 1, 0, 0) },
    { "amount_of_substance", FISICS_DIM_ATOM_DIMENSION, FISICS_DIM_FAMILY_AMOUNT, DIM_INIT(0, 0, 0, 0, 0, 1, 0, 0) },

    { "cd", FISICS_DIM_ATOM_DIMENSION, FISICS_DIM_FAMILY_LUMINOUS, DIM_INIT(0, 0, 0, 0, 0, 0, 1, 0) },
    { "luminous", FISICS_DIM_ATOM_DIMENSION, FISICS_DIM_FAMILY_LUMINOUS, DIM_INIT(0, 0, 0, 0, 0, 0, 1, 0) },
    { "luminous_intensity", FISICS_DIM_ATOM_DIMENSION, FISICS_DIM_FAMILY_LUMINOUS, DIM_INIT(0, 0, 0, 0, 0, 0, 1, 0) },

    { "X", FISICS_DIM_ATOM_DIMENSION, FISICS_DIM_FAMILY_CUSTOM, DIM_INIT(0, 0, 0, 0, 0, 0, 0, 1) },
    { "custom", FISICS_DIM_ATOM_DIMENSION, FISICS_DIM_FAMILY_CUSTOM, DIM_INIT(0, 0, 0, 0, 0, 0, 0, 1) },
    { "custom_dimension", FISICS_DIM_ATOM_DIMENSION, FISICS_DIM_FAMILY_CUSTOM, DIM_INIT(0, 0, 0, 0, 0, 0, 0, 1) },

    { "velocity", FISICS_DIM_ATOM_DIMENSION, FISICS_DIM_FAMILY_VELOCITY, DIM_INIT(1, 0, -1, 0, 0, 0, 0, 0) },
    { "speed", FISICS_DIM_ATOM_DIMENSION, FISICS_DIM_FAMILY_VELOCITY, DIM_INIT(1, 0, -1, 0, 0, 0, 0, 0) },

    { "acceleration", FISICS_DIM_ATOM_DIMENSION, FISICS_DIM_FAMILY_ACCELERATION, DIM_INIT(1, 0, -2, 0, 0, 0, 0, 0) },

    { "force", FISICS_DIM_ATOM_DIMENSION, FISICS_DIM_FAMILY_FORCE, DIM_INIT(1, 1, -2, 0, 0, 0, 0, 0) },

    { "energy", FISICS_DIM_ATOM_DIMENSION, FISICS_DIM_FAMILY_ENERGY, DIM_INIT(2, 1, -2, 0, 0, 0, 0, 0) },
    { "work", FISICS_DIM_ATOM_DIMENSION, FISICS_DIM_FAMILY_ENERGY, DIM_INIT(2, 1, -2, 0, 0, 0, 0, 0) },
    { "kinetic_energy", FISICS_DIM_ATOM_DIMENSION, FISICS_DIM_FAMILY_ENERGY, DIM_INIT(2, 1, -2, 0, 0, 0, 0, 0) },
    { "potential_energy", FISICS_DIM_ATOM_DIMENSION, FISICS_DIM_FAMILY_ENERGY, DIM_INIT(2, 1, -2, 0, 0, 0, 0, 0) },

    { "power", FISICS_DIM_ATOM_DIMENSION, FISICS_DIM_FAMILY_POWER, DIM_INIT(2, 1, -3, 0, 0, 0, 0, 0) },

    { "pressure", FISICS_DIM_ATOM_DIMENSION, FISICS_DIM_FAMILY_PRESSURE, DIM_INIT(-1, 1, -2, 0, 0, 0, 0, 0) },

    { "charge", FISICS_DIM_ATOM_DIMENSION, FISICS_DIM_FAMILY_CHARGE, DIM_INIT(0, 0, 1, 1, 0, 0, 0, 0) },

    { "voltage", FISICS_DIM_ATOM_DIMENSION, FISICS_DIM_FAMILY_VOLTAGE, DIM_INIT(2, 1, -3, -1, 0, 0, 0, 0) },
    { "electric_potential", FISICS_DIM_ATOM_DIMENSION, FISICS_DIM_FAMILY_VOLTAGE, DIM_INIT(2, 1, -3, -1, 0, 0, 0, 0) },

    { "resistance", FISICS_DIM_ATOM_DIMENSION, FISICS_DIM_FAMILY_RESISTANCE, DIM_INIT(2, 1, -3, -2, 0, 0, 0, 0) },
};

FisicsDim8 fisics_dim_zero(void) {
    FisicsDim8 dim = {{0}};
    return dim;
}

bool fisics_dim_equal(FisicsDim8 a, FisicsDim8 b) {
    for (int i = 0; i < FISICS_DIM_COUNT; ++i) {
        if (a.e[i] != b.e[i]) return false;
    }
    return true;
}

bool fisics_dim_add(FisicsDim8 a, FisicsDim8 b, FisicsDim8* outDim) {
    if (!outDim) return false;
    FisicsDim8 out = {{0}};
    for (int i = 0; i < FISICS_DIM_COUNT; ++i) {
        if (!checked_cast_i8((int)a.e[i] + (int)b.e[i], &out.e[i])) {
            return false;
        }
    }
    *outDim = out;
    return true;
}

bool fisics_dim_sub(FisicsDim8 a, FisicsDim8 b, FisicsDim8* outDim) {
    if (!outDim) return false;
    FisicsDim8 out = {{0}};
    for (int i = 0; i < FISICS_DIM_COUNT; ++i) {
        if (!checked_cast_i8((int)a.e[i] - (int)b.e[i], &out.e[i])) {
            return false;
        }
    }
    *outDim = out;
    return true;
}

bool fisics_dim_scale(FisicsDim8 dim, int power, FisicsDim8* outDim) {
    if (!outDim) return false;
    FisicsDim8 out = {{0}};
    for (int i = 0; i < FISICS_DIM_COUNT; ++i) {
        if (!checked_cast_i8((int)dim.e[i] * power, &out.e[i])) {
            return false;
        }
    }
    *outDim = out;
    return true;
}

bool fisics_dim_is_dimensionless(FisicsDim8 dim) {
    for (int i = 0; i < FISICS_DIM_COUNT; ++i) {
        if (dim.e[i] != 0) return false;
    }
    return true;
}

char* fisics_dim_to_string(FisicsDim8 dim) {
    static const char* baseNames[FISICS_DIM_COUNT] = {
        "m", "kg", "s", "A", "K", "mol", "cd", "X"
    };
    char buffer[256];
    size_t used = 0;
    bool hasNum = false;
    bool hasDen = false;

    for (int pass = 0; pass < 2; ++pass) {
        for (int i = 0; i < FISICS_DIM_COUNT; ++i) {
            int exp = (int)dim.e[i];
            if ((pass == 0 && exp <= 0) || (pass == 1 && exp >= 0)) {
                continue;
            }
            int mag = exp > 0 ? exp : -exp;
            if (pass == 0) {
                if (hasNum) buffer[used++] = '*';
                hasNum = true;
            } else {
                if (!hasDen) {
                    buffer[used++] = '/';
                    hasDen = true;
                } else {
                    buffer[used++] = '*';
                }
            }
            used += (size_t)snprintf(buffer + used, sizeof(buffer) - used, "%s", baseNames[i]);
            if (mag != 1) {
                used += (size_t)snprintf(buffer + used, sizeof(buffer) - used, "^%d", mag);
            }
        }
    }

    if (!hasNum && !hasDen) {
        strcpy(buffer, "1");
    } else if (!hasNum && hasDen) {
        char shifted[256];
        snprintf(shifted, sizeof(shifted), "1%s", buffer);
        strcpy(buffer, shifted);
    } else {
        buffer[used] = '\0';
    }
    return strdup(buffer);
}

FisicsDim8 fisics_dim_length(void) { return dim_with_slot(FISICS_DIM_LENGTH); }
FisicsDim8 fisics_dim_mass(void) { return dim_with_slot(FISICS_DIM_MASS); }
FisicsDim8 fisics_dim_time(void) { return dim_with_slot(FISICS_DIM_TIME); }
FisicsDim8 fisics_dim_current(void) { return dim_with_slot(FISICS_DIM_CURRENT); }
FisicsDim8 fisics_dim_temperature(void) { return dim_with_slot(FISICS_DIM_TEMPERATURE); }
FisicsDim8 fisics_dim_amount(void) { return dim_with_slot(FISICS_DIM_AMOUNT); }
FisicsDim8 fisics_dim_luminous(void) { return dim_with_slot(FISICS_DIM_LUMINOUS); }
FisicsDim8 fisics_dim_custom(void) { return dim_with_slot(FISICS_DIM_CUSTOM); }

FisicsDim8 fisics_dim_velocity(void) {
    FisicsDim8 dim = {{1, 0, -1, 0, 0, 0, 0, 0}};
    return dim;
}

FisicsDim8 fisics_dim_acceleration(void) {
    FisicsDim8 dim = {{1, 0, -2, 0, 0, 0, 0, 0}};
    return dim;
}

FisicsDim8 fisics_dim_force(void) {
    FisicsDim8 dim = {{1, 1, -2, 0, 0, 0, 0, 0}};
    return dim;
}

FisicsDim8 fisics_dim_energy(void) {
    FisicsDim8 dim = {{2, 1, -2, 0, 0, 0, 0, 0}};
    return dim;
}

FisicsDim8 fisics_dim_power(void) {
    FisicsDim8 dim = {{2, 1, -3, 0, 0, 0, 0, 0}};
    return dim;
}

FisicsDim8 fisics_dim_pressure(void) {
    FisicsDim8 dim = {{-1, 1, -2, 0, 0, 0, 0, 0}};
    return dim;
}

FisicsDim8 fisics_dim_charge(void) {
    FisicsDim8 dim = {{0, 0, 1, 1, 0, 0, 0, 0}};
    return dim;
}

FisicsDim8 fisics_dim_voltage(void) {
    FisicsDim8 dim = {{2, 1, -3, -1, 0, 0, 0, 0}};
    return dim;
}

FisicsDim8 fisics_dim_resistance(void) {
    FisicsDim8 dim = {{2, 1, -3, -2, 0, 0, 0, 0}};
    return dim;
}

const char* fisics_dim_family_name(FisicsDimFamily family) {
    return family_name(family);
}

bool fisics_dim_lookup_atom(const char* name, FisicsDimAtomInfo* outInfo) {
    if (!name || !outInfo) return false;
    memset(outInfo, 0, sizeof(*outInfo));
    outInfo->kind = FISICS_DIM_ATOM_UNKNOWN;

    for (size_t i = 0; i < sizeof(kDimAtoms) / sizeof(kDimAtoms[0]); ++i) {
        if (strcmp(kDimAtoms[i].name, name) != 0) continue;
        outInfo->kind = kDimAtoms[i].kind;
        outInfo->family = kDimAtoms[i].family;
        outInfo->family_name = family_name(kDimAtoms[i].family);
        outInfo->dim = kDimAtoms[i].dim;
        return true;
    }

    const FisicsUnitDef* unit = NULL;
    if (fisics_unit_lookup(name, &unit) && unit) {
        outInfo->kind = FISICS_DIM_ATOM_DEFERRED_UNIT;
        outInfo->family = unit->family;
        outInfo->family_name = family_name(unit->family);
        outInfo->dim = fisics_dim_zero();
        return true;
    }
    return false;
}
