// SPDX-License-Identifier: Apache-2.0
#include "Extensions/PhysicalSemantics/physical_representations.h"
#undef NDEBUG
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
int main(int argc, char **argv) {
  assert(argc == 3);
  FisicsPhysicalPolicy p = {0}, q = {0};
  FisicsPhysicalPolicyDiagnostic d = {0};
  if (!fisics_physical_policy_load(argv[1], &p, &d)) {
    fprintf(stderr, "%d %s\n", d.code, d.message);
    return 1;
  }
  assert(p.schema_version == 5 && p.representation_count == 20 &&
         p.shape_helper_count == 18);
  assert(fisics_physical_policy_load(argv[1], &q, &d));
  assert(q.representations != p.representations &&
         q.fingerprint == p.fingerprint);
  FisicsPhysicalPolicy saved = p;
  assert(!fisics_physical_policy_load(argv[2], &p, &d));
  assert(!memcmp(&saved, &p, sizeof(p)));
  size_t threshold = 0;
  for (; threshold < 2000; threshold++) {
    FisicsPhysicalLoadOptions o = fisics_physical_load_defaults();
    o.fail_after = threshold;
    if (fisics_physical_policy_load_with_options(argv[1], &p, o, &d))
      break;
    assert(d.code == 4305 && !memcmp(&saved, &p, sizeof(p)));
  }
  assert(threshold > 50 && threshold < 2000);
  assert(p.fingerprint == q.fingerprint);
  printf("{\"allocations\":%zu,\"representations\":%zu,\"helpers\":%zu,"
         "\"descriptor_bytes\":%zu,\"helper_bytes\":%zu,\"fingerprint\":\"%"
         "016" PRIx64 "\"}\n",
         threshold, p.representation_count, p.shape_helper_count,
         sizeof(*p.representations), sizeof(*p.shape_helpers),
         p.representation_fingerprint);
  fisics_physical_policy_free(&p);
  fisics_physical_policy_free(&q);
  return 0;
}
