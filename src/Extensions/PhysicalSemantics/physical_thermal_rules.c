// SPDX-License-Identifier: Apache-2.0
#include "Extensions/PhysicalSemantics/physical_operations.h"
#include <string.h>
/* V7 opt-in finite two-body thermal algebra. No absolute temperature, storage
 * origin, material law or automatic energy-form/process classification. */
static bool kind(const FisicsPhysicalPolicy *p, uint32_t k, const char *id) {
    static const struct {
        const char *id;
        int8_t dim[8];
    } kinds[] = {{"area", {2, 0, 0, 0, 0, 0, 0, 0}},
                 {"geometric_length", {1, 0, 0, 0, 0, 0, 0, 0}},
                 {"volume", {3, 0, 0, 0, 0, 0, 0, 0}},
                 {"volumetric_heat_capacity", {-1, 1, -2, 0, -1, 0, 0, 0}},
                 {"thermal_conductivity", {1, 1, -3, 0, -1, 0, 0, 0}},
                 {"area_specific_thermal_resistance", {0, -1, 3, 0, 1, 0, 0, 0}},
                 {"volumetric_enthalpy", {-1, 1, -2, 0, 0, 0, 0, 0}},
                 {"capacity_per_length", {1, 1, -2, 0, -1, 0, 0, 0}},
                 {"conductance_length", {3, 1, -3, 0, -1, 0, 0, 0}},
                 {"inverse_heat_capacity", {-2, -1, 2, 0, 1}},
                 {"power", {2, 1, -3}},
                 {"duration", {0, 0, 1}},
                 {"energy", {2, 1, -2}},
                 {"heat_capacity", {2, 1, -2, 0, -1}},
                 {"thermal_conductance", {2, 1, -3, 0, -1}},
                 {"temperature.difference", {0, 0, 0, 0, 1}},
                 {"temperature_rate", {0, 0, -1, 0, 1}},
                 {"inverse_duration", {0, 0, -1}},
                 {"ratio", {0}}};
    if (!k || k > p->kind_count)
        return false;
    FisicsPhysicalReferenceRole role =
        !strcmp(id, "temperature.difference") ? FISICS_PHYSICAL_DIFFERENCE : FISICS_PHYSICAL_LINEAR;
    if (p->models[k - 1].role != role)
        return false;
    for (size_t i = 0; i < sizeof(kinds) / sizeof(kinds[0]); i++)
        if (!strcmp(id, kinds[i].id))
            return !strncmp(p->kinds[k - 1].id, "quantity.", 9) &&
                   !strcmp(p->kinds[k - 1].id + 9, id) &&
                   !memcmp(p->kinds[k - 1].dimension, kinds[i].dim, 8);
    return false;
}
bool physical_thermal_contract(const FisicsPhysicalPolicy *p, FisicsUnitsMathOperation op,
                               uint32_t a, uint32_t b, uint32_t k) {
    if (!p || p->schema_version != 7)
        return false;
    static const struct {
        FisicsUnitsMathOperation op;
        const char *a, *b, *k;
    } rows[] = {
        {PHYSICAL_MULTIPLY, "temperature_rate", "duration", "temperature.difference"},
        {PHYSICAL_DIVIDE, "temperature.difference", "temperature.difference", "ratio"},
        {PHYSICAL_DIVIDE, "ratio", "inverse_duration", "duration"},
        {PHYSICAL_MULTIPLY, "heat_capacity", "temperature_rate", "power"},
        {PHYSICAL_MULTIPLY, "duration", "thermal_conductance", "heat_capacity"},
        {PHYSICAL_DIVIDE, "temperature_rate", "temperature_rate", "ratio"},
        {PHYSICAL_MULTIPLY, "volumetric_heat_capacity", "area", "capacity_per_length"},
        {PHYSICAL_MULTIPLY, "capacity_per_length", "geometric_length", "heat_capacity"},
        {PHYSICAL_DIVIDE, "geometric_length", "thermal_conductivity",
         "area_specific_thermal_resistance"},
        {PHYSICAL_DIVIDE, "area", "area_specific_thermal_resistance", "thermal_conductance"},
        {PHYSICAL_MULTIPLY, "area", "thermal_conductivity", "conductance_length"},
        {PHYSICAL_DIVIDE, "conductance_length", "geometric_length", "thermal_conductance"},
        {PHYSICAL_MULTIPLY, "temperature.difference", "volumetric_heat_capacity",
         "volumetric_enthalpy"},
        {PHYSICAL_MULTIPLY, "area", "geometric_length", "volume"},
        {PHYSICAL_MULTIPLY, "volume", "volumetric_enthalpy", "energy"},
        {PHYSICAL_DIVIDE, "energy", "volume", "volumetric_enthalpy"},
        {PHYSICAL_MULTIPLY, "volume", "volumetric_heat_capacity", "heat_capacity"},
        {PHYSICAL_DIVIDE, "ratio", "heat_capacity", "inverse_heat_capacity"},
        {PHYSICAL_MULTIPLY, "thermal_conductance", "inverse_heat_capacity", "inverse_duration"},
        {PHYSICAL_MULTIPLY, "power", "duration", "energy"},
        {PHYSICAL_DIVIDE, "energy", "duration", "power"},
        {PHYSICAL_MULTIPLY, "heat_capacity", "temperature.difference", "energy"},
        {PHYSICAL_DIVIDE, "energy", "heat_capacity", "temperature.difference"},
        {PHYSICAL_MULTIPLY, "thermal_conductance", "temperature.difference", "power"},
        {PHYSICAL_DIVIDE, "power", "heat_capacity", "temperature_rate"},
        {PHYSICAL_DIVIDE, "thermal_conductance", "heat_capacity", "inverse_duration"},
        {PHYSICAL_MULTIPLY, "inverse_duration", "duration", "ratio"},
        {PHYSICAL_DIVIDE, "heat_capacity", "heat_capacity", "ratio"}};
    for (size_t i = 0; i < sizeof(rows) / sizeof(rows[0]); i++) {
        if (op != rows[i].op || !kind(p, k, rows[i].k))
            continue;
        if (kind(p, a, rows[i].a) && kind(p, b, rows[i].b))
            return true;
        if (op == PHYSICAL_MULTIPLY && kind(p, b, rows[i].a) && kind(p, a, rows[i].b))
            return true;
    }
    return false;
}
