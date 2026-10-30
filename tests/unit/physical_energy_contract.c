// SPDX-License-Identifier: Apache-2.0
#include "Extensions/PhysicalSemantics/physical_energy_semantics.h"
#undef NDEBUG
#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
int main(int argc, char **argv) {
  assert(argc >= 3);
  FisicsPhysicalPolicy p = {0}, q = {0};
  FisicsPhysicalPolicyDiagnostic d = {0};
  if (!fisics_physical_policy_load(argv[1], &p, &d)) {
    fprintf(stderr, "%u %s\n", d.code, d.message);
    return 1;
  }
  FisicsPhysicalPolicy saved = p;
  if (argc == 4 && !strcmp(argv[3], "changed")) {
    assert(fisics_physical_policy_load(argv[2], &q, &d));
    assert(q.fingerprint != p.fingerprint && q.energy_fingerprint != p.energy_fingerprint);
    assert(q.frame_fingerprint == p.frame_fingerprint);
  } else if (argc == 4) {
    assert(!fisics_physical_policy_load(argv[2], &p, &d));
    assert((int)d.code == atoi(argv[3]) && !memcmp(&p, &saved, sizeof(p)));
  } else {
    assert(fisics_physical_policy_load(argv[2], &q, &d));
    size_t threshold = 0;
    for (; threshold < 2000; threshold++) {
      FisicsPhysicalLoadOptions o = fisics_physical_load_defaults();
      o.fail_after = threshold;
      if (fisics_physical_policy_load_with_options(argv[1], &p, o, &d))
        break;
      assert(d.code == 4305 && !memcmp(&p, &saved, sizeof(p)));
    }
    assert(threshold && threshold < 2000 && p.fingerprint == q.fingerprint);
    FILE *f = tmpfile();
    assert(f);
    physical_energy_write(f, &p);
    long n = ftell(f);
    assert(n > 0);
    rewind(f);
    char *s = malloc((size_t)n + 1);
    assert(s && fread(s, 1, (size_t)n, f) == (size_t)n);
    s[n] = 0;
    fclose(f);
    FisicsJsonValue *j = NULL;
    FisicsJsonDiagnostic jd = {0};
    assert(fisics_json_parse(s, (size_t)n, &j, &jd));
    assert(physical_energy_match(j, &p));
    fisics_json_free(j);
    free(s);
    assert(sizeof(PhysicalFact) == 64 && sizeof(PhysicalDeclaredContext) == 44);
    printf("{\"allocations\":%zu,\"fact_bytes\":%zu,\"context_bytes\":%zu,"
           "\"fingerprint\":\"%016" PRIx64 "\"}\n",
           threshold, sizeof(PhysicalFact), sizeof(PhysicalDeclaredContext),
           p.energy_fingerprint);
  }
  fisics_physical_policy_free(&p);
  fisics_physical_policy_free(&q);
  fisics_physical_policy_free(&p);
  return 0;
}
