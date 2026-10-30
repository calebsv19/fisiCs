// SPDX-License-Identifier: Apache-2.0
#include "Extensions/PhysicalSemantics/physical_operations.h"
#include <string.h>
bool physical_math_unary(FisicsUnitsMathOperation op) {
  return op == FISICS_UNITS_MATH_FABS || op == FISICS_UNITS_MATH_EXPM1 ||
         op == FISICS_UNITS_MATH_LOG || op == FISICS_UNITS_MATH_SQRT;
}
const char *physical_operation_origin(FisicsUnitsMathOperation op) {
  return op == FISICS_UNITS_MATH_FABS ? "preserve" :
      op == FISICS_UNITS_MATH_FMAX || op == FISICS_UNITS_MATH_FMIN ? "same" : "none";
}
const char *physical_operation_name(FisicsUnitsMathOperation op) {
  return op == FISICS_UNITS_MATH_SQRT ? "sqrt"
         : op == FISICS_UNITS_MATH_HYPOT ? "hypot"
         : op == FISICS_UNITS_MATH_ATAN2 ? "atan2"
         : op == FISICS_UNITS_MATH_FABS   ? "fabs"
         : op == FISICS_UNITS_MATH_FMAX ? "fmax"
         : op == FISICS_UNITS_MATH_FMIN ? "fmin"
         : op == FISICS_UNITS_MATH_EXPM1 ? "expm1"
         : op == FISICS_UNITS_MATH_LOG ? "log"
         : op == PHYSICAL_MULTIPLY      ? "multiply"
         : op == PHYSICAL_DIVIDE        ? "divide"
                                         : op == PHYSICAL_COUPLING_TR ? "frame_coupling_tr"
         : op == PHYSICAL_COUPLING_RT ? "frame_coupling_rt"
         : op == PHYSICAL_TRANSLATION_WORK ? "translation_work"
         : op == PHYSICAL_ROTATION_WORK ? "rotation_work"
         : op == PHYSICAL_LINE_MOMENT ? "transverse_line_moment"
         : "unknown";
}
static bool named(const FisicsPhysicalPolicy *p, uint32_t k, const char *id,
                  const int8_t dim[8]) {
  return !strcmp(p->kinds[k - 1].id, id) &&
         !memcmp(p->kinds[k - 1].dimension, dim, 8);
}
static bool linear(const FisicsPhysicalPolicy *p, uint32_t k) {
  return p->models[k - 1].role == FISICS_PHYSICAL_LINEAR ||
         p->models[k - 1].role == FISICS_PHYSICAL_DIFFERENCE;
}
bool physical_scalar_contract(const FisicsPhysicalPolicy *p,
                              FisicsUnitsMathOperation op, uint32_t a,
                              uint32_t b, uint32_t k) {
  if (!a || !b || !k || a > p->kind_count || b > p->kind_count ||
      k > p->kind_count || !linear(p, a) || !linear(p, b) || !linear(p, k))
    return false;
  static const int8_t zero[8] = {0}, time[8] = {0, 0, 1}, length[8] = {1},
                      velocity[8] = {1, 0, -1}, acceleration[8] = {1, 0, -2};
  bool mul = op == PHYSICAL_MULTIPLY;
  if (!mul && op != PHYSICAL_DIVIDE)
    return false;
  /* Signed integer arithmetic avoids overflowing the compact Dim8 storage. */
  for (unsigned i = 0; i < 8; i++)
    if ((int)p->kinds[a - 1].dimension[i] +
            (mul ? 1 : -1) * (int)p->kinds[b - 1].dimension[i] !=
        (int)p->kinds[k - 1].dimension[i])
      return false;
  if (physical_dynamic_contract(p, op, a, b, k) ||
      physical_cosmic_contract(p, op, a, b, k) ||
      physical_thermal_contract(p, op, a, b, k))
    return true;
  if (p->schema_version == 7 && physical_structural_contract(p, op, a, b, k))
    return true;
  if (p->schema_version >= 4 &&
      physical_mechanical_contract(p, op, a, b, k))
    return true;
  if (named(p, b, "quantity.ratio", zero) && a == k &&
      p->models[b - 1].role == FISICS_PHYSICAL_LINEAR)
    return true;
  if (mul && named(p, a, "quantity.ratio", zero) && b == k &&
      p->models[a - 1].role == FISICS_PHYSICAL_LINEAR)
    return true;
  if (mul && named(p, a, "quantity.duration", time)) {
    uint32_t swap = a;
    a = b;
    b = swap;
  }
  if (!named(p, b, "quantity.duration", time) ||
      p->models[b - 1].role != FISICS_PHYSICAL_LINEAR)
    return false;
  uint32_t v = mul ? k : a, ac = mul ? a : k;
  if (named(p, v, "quantity.velocity", velocity) &&
      named(p, ac, "quantity.acceleration", acceleration) &&
      p->models[v - 1].role == FISICS_PHYSICAL_LINEAR &&
      p->models[ac - 1].role == FISICS_PHYSICAL_LINEAR)
    return true;
  uint32_t d = mul ? k : a;
  v = mul ? a : k;
  return named(p, d, "quantity.displacement", length) &&
         named(p, v, "quantity.velocity", velocity) &&
         p->models[d - 1].role == FISICS_PHYSICAL_DIFFERENCE &&
         p->models[v - 1].role == FISICS_PHYSICAL_LINEAR;
}
const FisicsPhysicalOperationRule *
physical_scalar_rule(const FisicsPhysicalPolicy *p, FisicsUnitsMathOperation op,
                     uint32_t a, uint32_t b) {
  if (!p || p->schema_version < 3)
    return NULL;
  size_t lo = 0, hi = p->operation_count;
  while (lo < hi) {
    size_t m = lo + (hi - lo) / 2;
    const FisicsPhysicalOperationRule *r = &p->operations[m];
    if (r->operation < op ||
        (r->operation == op && (r->left < a || (r->left == a && r->right < b))))
      lo = m + 1;
    else
      hi = m;
  }
  if (lo == p->operation_count)
    return NULL;
  const FisicsPhysicalOperationRule *r = &p->operations[lo];
  return r->operation == op && r->left == a && r->right == b ? r : NULL;
}
