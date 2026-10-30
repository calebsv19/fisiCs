// SPDX-License-Identifier: Apache-2.0
#include "Extensions/PhysicalSemantics/physical_operations.h"
#include <string.h>
bool physical_cosmic_math_operation(FisicsUnitsMathOperation op) {
    return op == FISICS_UNITS_MATH_SQRT || op == FISICS_UNITS_MATH_HYPOT || op == FISICS_UNITS_MATH_ATAN2;
}
static bool kind(const FisicsPhysicalPolicy *p, uint32_t k, const char *id, const int8_t dim[8]) {
    if (!k || k > p->kind_count) return false;
    FisicsPhysicalReferenceRole role = !strcmp(id,"quantity.displacement") ? FISICS_PHYSICAL_DIFFERENCE : FISICS_PHYSICAL_LINEAR;
    return !strcmp(p->kinds[k-1].id,id) && !memcmp(p->kinds[k-1].dimension,dim,8) && p->models[k-1].role == role;
}
bool physical_cosmic_math_contract(const FisicsPhysicalPolicy *p, FisicsUnitsMathOperation op, uint32_t a, uint32_t b, uint32_t k) {
    if (!p || p->schema_version != 7) return false;
    static const int8_t length[8]={1}, area[8]={2}, velocity[8]={1,0,-1}, speed2[8]={2,0,-2}, acceleration[8]={1,0,-2}, acceleration2[8]={2,0,-4}, zero[8]={0}, inverse_velocity[8]={-1,0,1}, inverse_speed_squared[8]={-2,0,2};
    if (op == FISICS_UNITS_MATH_SQRT && !b)
        return (kind(p,a,"quantity.inverse_speed_squared",inverse_speed_squared) && kind(p,k,"quantity.inverse_velocity",inverse_velocity)) ||
               (kind(p,a,"quantity.area",area) && kind(p,k,"quantity.geometric_length",length)) ||
               (kind(p,a,"quantity.speed_squared",speed2) && kind(p,k,"quantity.velocity",velocity)) ||
               (kind(p,a,"quantity.acceleration_squared",acceleration2) && kind(p,k,"quantity.acceleration",acceleration));
    if (a != b) return false;
    if (op == FISICS_UNITS_MATH_HYPOT)
        return (kind(p,a,"quantity.ratio",zero) && a==k) ||
               (kind(p,a,"quantity.displacement",length) && kind(p,k,"quantity.geometric_length",length)) ||
               (kind(p,a,"quantity.velocity",velocity) && a==k) ||
               (kind(p,a,"quantity.acceleration",acceleration) && a==k);
    return op == FISICS_UNITS_MATH_ATAN2 && (kind(p,a,"quantity.geometric_length",length) || kind(p,a,"quantity.ratio",zero)) && kind(p,k,"quantity.angle",zero);
}
