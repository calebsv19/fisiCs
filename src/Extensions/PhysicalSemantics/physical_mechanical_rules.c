// SPDX-License-Identifier: Apache-2.0
#include "Extensions/PhysicalSemantics/physical_operations.h"
#include <string.h>
/* This finite primitive model validates authored scalar component application.
 * No matrix shape, coordinate frame, inverse, work or energy rule is implied.
 */
bool physical_mechanical_contract(const FisicsPhysicalPolicy *p,
                                  FisicsUnitsMathOperation op, uint32_t a,
                                  uint32_t b, uint32_t k) {
  if (p->schema_version < 4 || op != PHYSICAL_MULTIPLY)
    return false;
  static const struct {
    const char *coefficient, *coordinate, *result;
    int8_t coefficient_dim[8], coordinate_dim[8], result_dim[8];
  } rules[] = {{"quantity.translational_stiffness",
                "quantity.displacement",
                "quantity.force",
                {0, 1, -2},
                {1},
                {1, 1, -2}},
               {"quantity.force_per_angle",
                "quantity.angle",
                "quantity.force",
                {1, 1, -2},
                {0},
                {1, 1, -2}},
               {"quantity.moment_per_displacement",
                "quantity.displacement",
                "quantity.torque",
                {1, 1, -2},
                {1},
                {2, 1, -2}},
               {"quantity.rotational_stiffness",
                "quantity.angle",
                "quantity.torque",
                {2, 1, -2},
                {0},
                {2, 1, -2}}};
  for (size_t i = 0; i < sizeof(rules) / sizeof(rules[0]); i++) {
    for (unsigned reverse = 0; reverse < 2; reverse++) {
      uint32_t coefficient = reverse ? b : a, coordinate = reverse ? a : b;
      if (strcmp(p->kinds[coefficient - 1].id, rules[i].coefficient) ||
          strcmp(p->kinds[coordinate - 1].id, rules[i].coordinate) ||
          strcmp(p->kinds[k - 1].id, rules[i].result))
        continue;
      if (p->models[coefficient - 1].role != FISICS_PHYSICAL_LINEAR ||
          p->models[k - 1].role != FISICS_PHYSICAL_LINEAR ||
          (p->models[coordinate - 1].role != FISICS_PHYSICAL_LINEAR &&
           (strcmp(rules[i].coordinate, "quantity.displacement") ||
            p->models[coordinate - 1].role != FISICS_PHYSICAL_DIFFERENCE)))
        return false;
      return !memcmp(p->kinds[coefficient - 1].dimension,
                     rules[i].coefficient_dim, 8) &&
             !memcmp(p->kinds[coordinate - 1].dimension,
                     rules[i].coordinate_dim, 8) &&
             !memcmp(p->kinds[k - 1].dimension, rules[i].result_dim, 8);
    }
  }
  return false;
}
