// SPDX-License-Identifier: Apache-2.0
#include "Extensions/PhysicalSemantics/physical_semantics_internal.h"
#include "Extensions/extension_profile.h"
#include <stdio.h>
#define CHECK(x)                                                               \
  do {                                                                         \
    if (!(x)) {                                                                \
      fprintf(stderr, "line %d: %s\n", __LINE__, #x);                          \
      return 1;                                                                \
    }                                                                          \
  } while (0)
int main(int argc, char **argv) {
  CHECK(argc == 2);
  CompilerContext *c = cc_create();
  CHECK(c);
  cc_set_overlay_features(c, FISICS_OVERLAY_PHYSICS_SEMANTICS |
                                 FISICS_OVERLAY_PHYSICS_UNITS);
  CHECK(fisics_physical_prepare(c, argv[1]));
  const FisicsPhysicalPolicy *p = &physical_state(c)->policy;
  ASTNode left = {0}, right = {0}, operation = {0};
  const char *kinds[][3] = {
      {"quantity.position", "quantity.displacement", "origin.world"},
      {"quantity.temperature.absolute", "quantity.temperature.difference",
       "origin.kelvin.zero"}};
  size_t checks = 0;
  /* Cross all assumption states and both point placements. A higher-trust
     requirement must retain its actual source loss, not the operation location.
   */
  for (size_t pair = 0; pair < 2; pair++) {
    FisicsPhysicalKindId a =
        fisics_physical_policy_find_kind(p, kinds[pair][0]);
    FisicsPhysicalKindId d =
        fisics_physical_policy_find_kind(p, kinds[pair][1]);
    FisicsPhysicalOriginId o =
        fisics_physical_policy_find_origin(p, kinds[pair][2]);
    CHECK(a && d && o);
    for (int ls = PHYSICAL_KNOWN; ls <= PHYSICAL_DECLARED_TRUST; ls++)
      for (int rs = PHYSICAL_KNOWN; rs <= PHYSICAL_DECLARED_TRUST; rs++) {
        PhysicalFact l = {.type = {a, o},
                          .state = ls,
                          .origin = &left,
                          .loss = ls == PHYSICAL_KNOWN ? NULL : &left};
        PhysicalFact r = {.type = {d, 0},
                          .state = rs,
                          .origin = &right,
                          .loss = rs == PHYSICAL_KNOWN ? NULL : &right};
        PhysicalFactState expected = (PhysicalFactState)(ls > rs ? ls : rs);
        const ASTNode *loss = ls >= rs ? l.loss : r.loss;
        PhysicalFact result = physical_affine_binary(c, &operation, "+", l, r);
        CHECK(result.type.kind == a && result.type.origin == o &&
              result.state == expected && result.loss == loss);
        result = physical_affine_binary(c, &operation, "-", l, r);
        CHECK(result.type.kind == a && result.type.origin == o &&
              result.state == expected && result.loss == loss);
        result = physical_affine_binary(c, &operation, "+", r, l);
        /* Equal trust may choose the point's original source, so loss is l. */
        CHECK(result.type.kind == a && result.type.origin == o &&
              result.state == expected && result.loss == loss);
        r.type = l.type;
        result = physical_affine_binary(c, &operation, "-", l, r);
        CHECK(result.type.kind == d && !result.type.origin &&
              result.state == expected && result.loss == loss);
        checks += 4;
      }
  }
  CHECK(!compiler_diagnostics_error_count(c));
  cc_destroy(c);
  printf("physical affine trust/origin controls: PASS (%zu checks)\n", checks);
  return 0;
}
