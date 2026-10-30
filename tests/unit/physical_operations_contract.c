// SPDX-License-Identifier: Apache-2.0
#include "Extensions/PhysicalSemantics/physical_operations.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#define CHECK(x)                                                               \
  do {                                                                         \
    if (!(x)) {                                                                \
      fprintf(stderr, "line %d: %s\n", __LINE__, #x);                          \
      return 1;                                                                \
    }                                                                          \
  } while (0)
int main(int argc, char **argv) {
  CHECK(argc == 4);
  FisicsPhysicalPolicy p = {0}, other = {0};
  FisicsPhysicalPolicyDiagnostic d = {0};
  CHECK(fisics_physical_policy_load(argv[1], &p, &d));
  CHECK(p.schema_version == 1 && !p.operation_count && !p.operations &&
        !p.operation_fingerprint);
  FisicsPhysicalPolicy before = p;
  FisicsPhysicalLoadOptions options = fisics_physical_load_defaults();
  CHECK(!fisics_physical_policy_load(argv[3], &p, &d) && d.code == 4303 &&
        !memcmp(&p, &before, sizeof(p)));
  size_t failures = 0;
  for (; failures < 2000; failures++) {
    options.fail_after = failures;
    if (fisics_physical_policy_load_with_options(argv[2], &p, options, &d))
      break;
    CHECK(d.code == 4305 && !memcmp(&p, &before, sizeof(p)));
  }
  CHECK(failures > 0 && failures < 2000 && (p.schema_version >= 2 && p.schema_version <= 7) &&
        p.operation_count > 0);
  CHECK(p.operation_fingerprint == physical_operations_fingerprint(&p));
  CHECK(fisics_physical_policy_load(argv[2], &other, &d));
  CHECK(other.operations != p.operations && other.fingerprint == p.fingerprint);
  for (size_t i = 0; i < p.operation_count; i++) {
    const FisicsPhysicalOperationRule *rule = &p.operations[i];
    if (rule->operation == PHYSICAL_MULTIPLY || rule->operation == PHYSICAL_DIVIDE) {
      CHECK(physical_scalar_rule(&p,rule->operation,rule->left,rule->right)==rule);
      CHECK(physical_scalar_contract(&p,rule->operation,rule->left,rule->right,rule->kind));
    } else if (physical_cosmic_math_operation(rule->operation)) {
      CHECK(physical_scalar_rule(&p,rule->operation,rule->left,rule->right)==rule);
      CHECK(physical_cosmic_math_contract(&p,rule->operation,rule->left,rule->right,rule->kind));
    } else if (physical_constructor_operation(rule->operation)) {
      CHECK(physical_constructor_rule(&p,rule->id)==rule);
      CHECK(physical_structural_contract(&p,rule->operation,rule->left,rule->right,rule->kind));
    } else CHECK(physical_operation_rule(&p, rule->operation, rule->kind) == rule);
    CHECK(!physical_operation_rule(&p, FISICS_UNITS_MATH_SQRT, rule->kind));
  }
  CHECK(!physical_operation_rule(NULL, FISICS_UNITS_MATH_FABS, 0));
  CHECK(!physical_operation_rule(&p, FISICS_UNITS_MATH_FABS, 0));
  before = p;
  CHECK(!fisics_physical_policy_load(argv[3], &p, &d) && d.code == 4303 &&
        !memcmp(&p, &before, sizeof(p)));
  CHECK(fisics_physical_policy_load(argv[1], &other, &d) &&
        other.schema_version == 1 && !other.operations &&
        !other.operation_count);
  fisics_physical_policy_free(&p);
  fisics_physical_policy_free(&other);
  CHECK(!p.operations && !p.operation_count && !p.schema_version);
  printf("{\"allocation_failure_thresholds\":%zu,\"passed\":true}\n", failures);
  return 0;
}
