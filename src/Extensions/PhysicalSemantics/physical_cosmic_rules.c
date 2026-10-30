// SPDX-License-Identifier: Apache-2.0
#include "Extensions/PhysicalSemantics/physical_operations.h"
#include <string.h>
/* V7 finite Cosmic scalar primitives. A tuple does not establish vector shape,
 * frame, a gravity model, energy form, or a conserved/closed system. */
bool physical_cosmic_contract(const FisicsPhysicalPolicy *p, FisicsUnitsMathOperation op,
                             uint32_t a, uint32_t b, uint32_t k) {
    if (!p || p->schema_version != 7) return false;
    static const struct { const char *id; int8_t dim[8]; } kinds[] = {
        {"quantity.gravitational_constant", {3,-1,-2,0,0,0,0,0}},
        {"quantity.gravitational_parameter", {3,0,-2,0,0,0,0,0}},
        {"quantity.gravity_pair_strength", {3,1,-2,0,0,0,0,0}},
        {"quantity.translational_stiffness", {0,1,-2,0,0,0,0,0}},
        {"quantity.translational_damping", {0,1,-1,0,0,0,0,0}},
        {"quantity.dissipative_power", {2,1,-3,0,0,0,0,0}},
        {"quantity.mass", {0,1,0,0,0,0,0,0}},
        {"quantity.area", {2,0,0,0,0,0,0,0}},
        {"quantity.geometric_length", {1,0,0,0,0,0,0,0}},
        {"quantity.displacement", {1,0,0,0,0,0,0,0}},
        {"quantity.velocity", {1,0,-1,0,0,0,0,0}},
        {"quantity.speed_squared", {2,0,-2,0,0,0,0,0}},
        {"quantity.specific_energy", {2,0,-2,0,0,0,0,0}},
        {"quantity.specific_external_potential", {2,0,-2,0,0,0,0,0}},
        {"quantity.external_potential_energy", {2,1,-2,0,0,0,0,0}},
        {"quantity.acceleration", {1,0,-2,0,0,0,0,0}},
        {"quantity.force", {1,1,-2,0,0,0,0,0}},
        {"quantity.momentum", {1,1,-1,0,0,0,0,0}},
        {"quantity.orbital_angular_momentum", {2,1,-1,0,0,0,0,0}},
        {"quantity.rotational_momentum", {2,1,-1,0,0,0,0,0}},
        {"quantity.moment_of_inertia", {2,1,0,0,0,0,0,0}},
        {"quantity.angular_rate", {0,0,-1,0,0,0,0,0}},
        {"quantity.angle", {0,0,0,0,0,0,0,0}},
        {"quantity.energy", {2,1,-2,0,0,0,0,0}},
        {"quantity.duration", {0,0,1,0,0,0,0,0}},
        {"quantity.inverse_length", {-1,0,0,0,0,0,0,0}},
        {"quantity.ratio", {0,0,0,0,0,0,0,0}},
        {"quantity.acceleration_squared", {2,0,-4,0,0,0,0,0}},
        {"quantity.specific_angular_momentum", {2,0,-1,0,0,0,0,0}},
        {"quantity.kinetic_specific_factor", {0,0,0,0,0,0,0,0}},
        {"quantity.inverse_velocity", {-1,0,1,0,0,0,0,0}},
        {"quantity.inverse_speed_squared", {-2,0,2,0,0,0,0,0}},

    };
    uint32_t ids[3]={a,b,k};const char *names[3]={0};
    for(unsigned n=0;n<3;n++) {
        uint32_t id=ids[n];if(!id || id>p->kind_count) return false;
        for(size_t i=0;i<sizeof(kinds)/sizeof(kinds[0]);i++)
            if(!strcmp(p->kinds[id-1].id,kinds[i].id) && !memcmp(p->kinds[id-1].dimension,kinds[i].dim,8)) names[n]=kinds[i].id;
        if(!names[n]) return false;
        FisicsPhysicalReferenceRole role=!strcmp(names[n],"quantity.displacement")?FISICS_PHYSICAL_DIFFERENCE:FISICS_PHYSICAL_LINEAR;
        if(p->models[id-1].role!=role) return false;
    }
    static const struct { FisicsUnitsMathOperation op; const char *a,*b,*result; } rows[] = {
        {PHYSICAL_MULTIPLY,"quantity.translational_stiffness","quantity.displacement","quantity.force"},
        {PHYSICAL_MULTIPLY,"quantity.translational_damping","quantity.velocity","quantity.force"},
        {PHYSICAL_MULTIPLY,"quantity.force","quantity.displacement","quantity.energy"},
        {PHYSICAL_MULTIPLY,"quantity.force","quantity.velocity","quantity.dissipative_power"},
        {PHYSICAL_MULTIPLY,"quantity.dissipative_power","quantity.duration","quantity.energy"},
        {PHYSICAL_DIVIDE,"quantity.moment_of_inertia","quantity.moment_of_inertia","quantity.ratio"},
        {PHYSICAL_DIVIDE,"quantity.gravitational_parameter","quantity.geometric_length","quantity.specific_external_potential"},
        {PHYSICAL_DIVIDE,"quantity.specific_external_potential","quantity.geometric_length","quantity.acceleration"},
        {PHYSICAL_MULTIPLY,"quantity.mass","quantity.specific_external_potential","quantity.external_potential_energy"},
        {PHYSICAL_MULTIPLY,"quantity.speed_squared","quantity.kinetic_specific_factor","quantity.specific_energy"},
        {PHYSICAL_MULTIPLY,"quantity.geometric_length","quantity.velocity","quantity.specific_angular_momentum"},
        {PHYSICAL_DIVIDE,"quantity.specific_angular_momentum","quantity.gravitational_parameter","quantity.inverse_velocity"},
        {PHYSICAL_MULTIPLY,"quantity.velocity","quantity.inverse_velocity","quantity.ratio"},
        {PHYSICAL_MULTIPLY,"quantity.inverse_velocity","quantity.specific_angular_momentum","quantity.geometric_length"},
        {PHYSICAL_DIVIDE,"quantity.gravitational_parameter","quantity.specific_energy","quantity.geometric_length"},
        {PHYSICAL_DIVIDE,"quantity.geometric_length","quantity.gravitational_parameter","quantity.inverse_speed_squared"},
        {PHYSICAL_MULTIPLY,"quantity.geometric_length","quantity.inverse_velocity","quantity.duration"},
        {PHYSICAL_DIVIDE,"quantity.geometric_length","quantity.velocity","quantity.duration"},
        {PHYSICAL_DIVIDE,"quantity.geometric_length","quantity.geometric_length","quantity.ratio"},
        {PHYSICAL_DIVIDE,"quantity.velocity","quantity.velocity","quantity.ratio"},
        {PHYSICAL_MULTIPLY,"quantity.geometric_length","quantity.geometric_length","quantity.area"},
        {PHYSICAL_MULTIPLY,"quantity.acceleration","quantity.acceleration","quantity.acceleration_squared"},
        {PHYSICAL_DIVIDE,"quantity.acceleration","quantity.acceleration","quantity.ratio"},
        {PHYSICAL_DIVIDE,"quantity.displacement","quantity.geometric_length","quantity.ratio"},
        {PHYSICAL_DIVIDE,"quantity.mass","quantity.mass","quantity.ratio"},
        {PHYSICAL_DIVIDE,"quantity.ratio","quantity.geometric_length","quantity.inverse_length"},
        {PHYSICAL_DIVIDE,"quantity.angle","quantity.angle","quantity.ratio"},
        {PHYSICAL_MULTIPLY,"quantity.displacement","quantity.velocity","quantity.specific_angular_momentum"},
        {PHYSICAL_MULTIPLY,"quantity.mass","quantity.specific_angular_momentum","quantity.orbital_angular_momentum"},
        {PHYSICAL_MULTIPLY,"quantity.momentum","quantity.velocity","quantity.energy"},
        {PHYSICAL_MULTIPLY,"quantity.gravitational_constant","quantity.mass","quantity.gravitational_parameter"},
        {PHYSICAL_MULTIPLY,"quantity.gravitational_parameter","quantity.mass","quantity.gravity_pair_strength"},
        {PHYSICAL_DIVIDE,"quantity.gravity_pair_strength","quantity.area","quantity.force"},
        {PHYSICAL_DIVIDE,"quantity.force","quantity.mass","quantity.acceleration"},
        {PHYSICAL_MULTIPLY,"quantity.mass","quantity.acceleration","quantity.force"},
        {PHYSICAL_MULTIPLY,"quantity.mass","quantity.velocity","quantity.momentum"},
        {PHYSICAL_DIVIDE,"quantity.momentum","quantity.mass","quantity.velocity"},
        {PHYSICAL_MULTIPLY,"quantity.velocity","quantity.velocity","quantity.speed_squared"},
        {PHYSICAL_MULTIPLY,"quantity.mass","quantity.speed_squared","quantity.energy"},
        {PHYSICAL_DIVIDE,"quantity.energy","quantity.mass","quantity.specific_energy"},
        {PHYSICAL_MULTIPLY,"quantity.mass","quantity.specific_energy","quantity.energy"},
        {PHYSICAL_DIVIDE,"quantity.gravitational_parameter","quantity.geometric_length","quantity.specific_energy"},
        {PHYSICAL_DIVIDE,"quantity.gravitational_parameter","quantity.area","quantity.acceleration"},
        {PHYSICAL_MULTIPLY,"quantity.displacement","quantity.displacement","quantity.area"},
        {PHYSICAL_MULTIPLY,"quantity.mass","quantity.area","quantity.moment_of_inertia"},
        {PHYSICAL_MULTIPLY,"quantity.displacement","quantity.momentum","quantity.orbital_angular_momentum"},
        {PHYSICAL_MULTIPLY,"quantity.moment_of_inertia","quantity.angular_rate","quantity.orbital_angular_momentum"},
        /* Admitted rotational-DOF scalar contraction; no frame/form inference. */
        {PHYSICAL_MULTIPLY,"quantity.moment_of_inertia","quantity.angular_rate","quantity.rotational_momentum"},
        {PHYSICAL_MULTIPLY,"quantity.rotational_momentum","quantity.angular_rate","quantity.energy"},
        {PHYSICAL_DIVIDE,"quantity.orbital_angular_momentum","quantity.moment_of_inertia","quantity.angular_rate"},
        {PHYSICAL_MULTIPLY,"quantity.angular_rate","quantity.duration","quantity.angle"},
        {PHYSICAL_MULTIPLY,"quantity.force","quantity.duration","quantity.momentum"},
        {PHYSICAL_DIVIDE,"quantity.momentum","quantity.duration","quantity.force"},
        {PHYSICAL_DIVIDE,"quantity.gravity_pair_strength","quantity.geometric_length","quantity.energy"},
        {PHYSICAL_MULTIPLY,"quantity.gravity_pair_strength","quantity.inverse_length","quantity.energy"},
    };
        /* finite scalar component/coordinate and radian-normalization rules */
    for(size_t i=0;i<sizeof(rows)/sizeof(rows[0]);i++) {
        if(op!=rows[i].op || strcmp(names[2],rows[i].result)) continue;
        if(!strcmp(names[0],rows[i].a) && !strcmp(names[1],rows[i].b)) return true;
        if(op==PHYSICAL_MULTIPLY && !strcmp(names[1],rows[i].a) && !strcmp(names[0],rows[i].b)) return true;
    }
    return false;
}
