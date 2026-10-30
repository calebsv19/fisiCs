// SPDX-License-Identifier: Apache-2.0
#include "Extensions/PhysicalSemantics/physical_operations.h"
#include <string.h>
/* V7 opt-in structural scalar vocabulary. Geometric lengths are deliberately
 * distinct from displacement. No global force*length -> energy/torque rule. */
static bool kind(const FisicsPhysicalPolicy *p, uint32_t k, const char *id) {
  static const struct {
    const char *id;
    int8_t dim[8];
  } kinds[] = {{"temperature.difference", {0, 0, 0, 0, 1}},
               {"thermal_expansion_coefficient", {0, 0, 0, 0, -1}},
               {"mechanical_strain", {0}},
               {"pressure", {-1, 1, -2}},
               {"area", {2}},
               {"inertia", {4}},
               {"geometric_length", {1}},
               {"length_squared", {2}},
               {"length_cubed", {3}},
               {"section_modulus", {3}},
               {"axial_rigidity", {1, 1, -2}},
               {"flexural_rigidity", {3, 1, -2}},
               {"translational_stiffness", {0, 1, -2}},
               {"coupling_coefficient", {1, 1, -2}},
               {"force_per_angle", {1, 1, -2}},
               {"moment_per_displacement", {1, 1, -2}},
               {"rotational_stiffness", {2, 1, -2}},
               {"line_load", {0, 1, -2}},
               {"force", {1, 1, -2}},
               {"torque", {2, 1, -2}},
               {"displacement", {1}},
               {"angle", {0}},
               {"energy", {2, 1, -2}}};
  if (!k || k > p->kind_count)
    return false;
  if (!strcmp(id, "temperature.difference") &&
      p->models[k - 1].role != FISICS_PHYSICAL_DIFFERENCE)
    return false;
  if (strcmp(id, "temperature.difference") &&
      p->models[k - 1].role != FISICS_PHYSICAL_LINEAR &&
      (strcmp(id, "displacement") ||
       p->models[k - 1].role != FISICS_PHYSICAL_DIFFERENCE))
    return false;
  for (size_t i = 0; i < sizeof(kinds) / sizeof(kinds[0]); i++)
    if (!strcmp(id, kinds[i].id))
      return !strncmp(p->kinds[k - 1].id, "quantity.", 9) &&
             !strcmp(p->kinds[k - 1].id + 9, id) &&
             !memcmp(p->kinds[k - 1].dimension, kinds[i].dim, 8);
  return false;
}
bool physical_constructor_operation(FisicsUnitsMathOperation op) {
  return op >= PHYSICAL_COUPLING_TR && op <= PHYSICAL_LINE_MOMENT;
}
bool physical_structural_contract(const FisicsPhysicalPolicy *p,
                                  FisicsUnitsMathOperation op, uint32_t a,
                                  uint32_t b, uint32_t k) {
  if (!p || p->schema_version != 7)
    return false;
  static const struct {
    FisicsUnitsMathOperation op;
    const char *a, *b, *k;
  } rows[] = {
      {PHYSICAL_MULTIPLY, "thermal_expansion_coefficient", "temperature.difference", "mechanical_strain"},
      {PHYSICAL_MULTIPLY, "temperature.difference", "thermal_expansion_coefficient", "mechanical_strain"},
      {PHYSICAL_MULTIPLY, "axial_rigidity", "mechanical_strain", "force"},
      {PHYSICAL_DIVIDE, "displacement", "geometric_length", "mechanical_strain"},
      {PHYSICAL_MULTIPLY, "mechanical_strain", "geometric_length", "displacement"},
      {PHYSICAL_MULTIPLY, "pressure", "area", "axial_rigidity"},
      {PHYSICAL_MULTIPLY, "pressure", "inertia", "flexural_rigidity"},
      {PHYSICAL_MULTIPLY, "geometric_length", "geometric_length",
       "length_squared"},
      {PHYSICAL_MULTIPLY, "length_squared", "geometric_length", "length_cubed"},
      {PHYSICAL_DIVIDE, "axial_rigidity", "geometric_length",
       "translational_stiffness"},
      {PHYSICAL_DIVIDE, "flexural_rigidity", "length_cubed",
       "translational_stiffness"},
      {PHYSICAL_MULTIPLY, "translational_stiffness", "geometric_length",
       "coupling_coefficient"},
      {PHYSICAL_MULTIPLY, "coupling_coefficient", "geometric_length",
       "rotational_stiffness"},
      {PHYSICAL_MULTIPLY, "line_load", "geometric_length", "force"},
      {PHYSICAL_MULTIPLY, "line_load", "length_squared", "torque"},
      {PHYSICAL_DIVIDE, "force", "area", "pressure"},
      {PHYSICAL_DIVIDE, "torque", "section_modulus", "pressure"},
      {PHYSICAL_COUPLING_TR, "coupling_coefficient", NULL, "force_per_angle"},
      {PHYSICAL_COUPLING_RT, "coupling_coefficient", NULL,
       "moment_per_displacement"},
      {PHYSICAL_TRANSLATION_WORK, "force", "displacement", "energy"},
      {PHYSICAL_ROTATION_WORK, "torque", "angle", "energy"},
      {PHYSICAL_LINE_MOMENT, "force", "geometric_length", "torque"}};
  for (size_t i = 0; i < sizeof(rows) / sizeof(rows[0]); i++)
    if (op == rows[i].op && kind(p, a, rows[i].a) && kind(p, k, rows[i].k) &&
        (rows[i].b ? kind(p, b, rows[i].b) : !b))
      return true;
  return false;
}
const FisicsPhysicalOperationRule *
physical_constructor_rule(const FisicsPhysicalPolicy *p, const char *id) {
  if (!p || p->schema_version != 7 || !id)
    return NULL;
  for (size_t i = 0; i < p->operation_count; i++)
    if (physical_constructor_operation(p->operations[i].operation) &&
        !strcmp(p->operations[i].id, id))
      return &p->operations[i];
  return NULL;
}
