// SPDX-License-Identifier: Apache-2.0

#include "Extensions/Units/units_model.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
