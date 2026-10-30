// SPDX-License-Identifier: Apache-2.0
#include "Extensions/PhysicalSemantics/physical_operations.h"
#include <string.h>

/* Explicit scalar cross-block generalized inertia. This validates contractions,
 * not matrix provenance, DOF selection, frame/basis or material admission. */
bool physical_dynamic_contract(const FisicsPhysicalPolicy *p,
                               FisicsUnitsMathOperation op,
                               uint32_t a, uint32_t b, uint32_t k) {
    if (!p || p->schema_version != 7 || op != PHYSICAL_MULTIPLY) return false;
    static const struct { const char *id; int8_t dim[8]; } kinds[] = {
        {"quantity.inertia_translation_rotation", {1,1,0,0,0,0,0,0}},
        {"quantity.inertia_rotation_translation", {1,1,0,0,0,0,0,0}},
        {"quantity.velocity", {1,0,-1,0,0,0,0,0}},
        {"quantity.angular_rate", {0,0,-1,0,0,0,0,0}},
        {"quantity.momentum", {1,1,-1,0,0,0,0,0}},
        {"quantity.rotational_momentum", {2,1,-1,0,0,0,0,0}}
    };
    uint32_t ids[3] = {a,b,k};
    const char *names[3] = {0};
    for (unsigned n=0; n<3; n++) {
        uint32_t id=ids[n];
        if (!id || id>p->kind_count ||
            p->models[id-1].role != FISICS_PHYSICAL_LINEAR) return false;
        for (size_t i=0; i<sizeof(kinds)/sizeof(kinds[0]); i++)
            if (!strcmp(p->kinds[id-1].id,kinds[i].id) &&
                !memcmp(p->kinds[id-1].dimension,kinds[i].dim,8))
                names[n]=kinds[i].id;
        if (!names[n]) return false;
    }
    static const struct { const char *a,*b,*result; } rows[] = {
        {"quantity.velocity", "quantity.inertia_translation_rotation", "quantity.rotational_momentum"},
        {"quantity.angular_rate", "quantity.inertia_translation_rotation", "quantity.momentum"},
        {"quantity.angular_rate", "quantity.inertia_rotation_translation", "quantity.momentum"},
        {"quantity.velocity", "quantity.inertia_rotation_translation", "quantity.rotational_momentum"}
    };
    /* Both multiplication orders are valid scalar contractions. The coefficient
     * role remains distinct at storage/parameter boundaries; no alias inference. */
    for (size_t i=0; i<sizeof(rows)/sizeof(rows[0]); i++)
        if (!strcmp(names[2],rows[i].result) &&
            ((!strcmp(names[0],rows[i].a) && !strcmp(names[1],rows[i].b)) ||
             (!strcmp(names[1],rows[i].a) && !strcmp(names[0],rows[i].b))))
            return true;
    return false;
}
