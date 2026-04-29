// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>
#include <stdint.h>

enum {
    FISICS_DIM_COUNT = 8
};

typedef enum {
    FISICS_DIM_LENGTH = 0,
    FISICS_DIM_MASS = 1,
    FISICS_DIM_TIME = 2,
    FISICS_DIM_CURRENT = 3,
    FISICS_DIM_TEMPERATURE = 4,
    FISICS_DIM_AMOUNT = 5,
    FISICS_DIM_LUMINOUS = 6,
    FISICS_DIM_CUSTOM = 7
} FisicsDimSlot;

typedef struct {
    int8_t e[FISICS_DIM_COUNT];
} FisicsDim8;

FisicsDim8 fisics_dim_zero(void);
bool fisics_dim_equal(FisicsDim8 a, FisicsDim8 b);
bool fisics_dim_add(FisicsDim8 a, FisicsDim8 b, FisicsDim8* outDim);
bool fisics_dim_sub(FisicsDim8 a, FisicsDim8 b, FisicsDim8* outDim);
bool fisics_dim_scale(FisicsDim8 dim, int power, FisicsDim8* outDim);
bool fisics_dim_is_dimensionless(FisicsDim8 dim);
char* fisics_dim_to_string(FisicsDim8 dim);

FisicsDim8 fisics_dim_length(void);
FisicsDim8 fisics_dim_mass(void);
FisicsDim8 fisics_dim_time(void);
FisicsDim8 fisics_dim_current(void);
FisicsDim8 fisics_dim_temperature(void);
FisicsDim8 fisics_dim_amount(void);
FisicsDim8 fisics_dim_luminous(void);
FisicsDim8 fisics_dim_custom(void);

FisicsDim8 fisics_dim_velocity(void);
FisicsDim8 fisics_dim_acceleration(void);
FisicsDim8 fisics_dim_force(void);
FisicsDim8 fisics_dim_energy(void);
FisicsDim8 fisics_dim_power(void);
FisicsDim8 fisics_dim_pressure(void);
FisicsDim8 fisics_dim_charge(void);
FisicsDim8 fisics_dim_voltage(void);
FisicsDim8 fisics_dim_resistance(void);
