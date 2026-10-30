// SPDX-License-Identifier: Apache-2.0
#include "Compiler/diagnostic_metadata.h"
#include "Extensions/PhysicalSemantics/physical_policy.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x)                                                               \
  do {                                                                         \
    if (!(x)) {                                                                \
      fprintf(stderr, "line %d: %s\n", __LINE__, #x);                          \
      return 1;                                                                \
    }                                                                          \
  } while (0)
int main(int argc, char **argv) {
  CHECK(argc >= 4);
  FisicsPhysicalPolicy p = {0};
  FisicsPhysicalPolicyDiagnostic d = {0};
  CHECK(fisics_physical_policy_load(argv[1], &p, &d));
  CHECK(p.kind_count == 9 && p.origin_count == 3 && p.contract_count == 3);
  uint32_t energy = fisics_physical_policy_find_kind(&p, "quantity.energy");
  uint32_t torque = fisics_physical_policy_find_kind(&p, "quantity.torque");
  uint32_t point = fisics_physical_policy_find_kind(&p, "quantity.position");
  uint32_t delta =
      fisics_physical_policy_find_kind(&p, "quantity.displacement");
  uint32_t origin = fisics_physical_policy_find_origin(&p, "origin.world");
  CHECK(
      energy && torque && energy != torque &&
      !memcmp(p.kinds[energy - 1].dimension, p.kinds[torque - 1].dimension, 8));
  CHECK(p.models[point - 1].role == FISICS_PHYSICAL_ABSOLUTE &&
        p.models[point - 1].paired_kind == delta);
  CHECK(p.models[delta - 1].role == FISICS_PHYSICAL_DIFFERENCE &&
        p.models[delta - 1].paired_kind == point);
  CHECK(!fisics_physical_policy_find_kind(&p, "unknown") &&
        !fisics_physical_policy_find_origin(&p, "unknown"));
  CHECK(
      p.models[fisics_physical_policy_find_kind(&p, "quantity.unprofiled") - 1]
          .role == FISICS_PHYSICAL_UNPROFILED);
  FisicsPhysicalType t = {0};
  CHECK(fisics_physical_policy_make_type(&p, point, origin, &t, &d));
  FisicsPhysicalType saved = t;
  uint32_t badk[] = {0, UINT32_MAX, point, point, delta, energy};
  uint32_t bado[] = {
      0,      0,     0, fisics_physical_policy_find_origin(&p, "origin.kelvin"),
      origin, origin};
  for (size_t i = 0; i < 6; i++) {
    CHECK(!fisics_physical_policy_make_type(&p, badk[i], bado[i], &t, &d));
    CHECK(!memcmp(&saved, &t, sizeof(t)) && d.code == 4303);
  }
  CHECK(fisics_physical_policy_make_type(&p, delta, 0, &t, &d));
  for (int code = 4301; code <= 4306; code++) {
    CHECK(fisics_diag_category_id_from_code(code) ==
          FISICS_DIAG_CATEGORY_EXTENSION);
    CHECK(!strcmp(fisics_diag_stage_name_from_code(code), "extension"));
    CHECK(fisics_diag_explanation_by_code(code));
    CHECK(fisics_diag_explanation_by_name(fisics_diag_code_name(code)));
  }
  FisicsPhysicalPolicy other = {0};
  CHECK(fisics_physical_policy_load(argv[1], &other, &d));
  CHECK(other.kinds != p.kinds && other.models != p.models &&
        other.fingerprint == p.fingerprint);
  fisics_physical_policy_free(&other);
  CHECK(fisics_physical_policy_make_type(
      &p, fisics_physical_policy_find_kind(&p, "quantity.unprofiled"), 0, &t,
      &d));
  FisicsPhysicalPolicy before = p;
  FisicsPhysicalLoadOptions invalid = fisics_physical_load_defaults();
  invalid.max_kinds++;
  CHECK(!fisics_physical_policy_load_with_options(argv[1], &p, invalid, &d) &&
        d.code == 4301 && !memcmp(&p, &before, sizeof(p)));
  CHECK(!fisics_physical_policy_load(NULL, &p, &d) && d.code == 4301 &&
        !memcmp(&p, &before, sizeof(p)));
  CHECK(!fisics_physical_policy_load(argv[1], NULL, &d) && d.code == 4301);
  FisicsPhysicalLoadOptions o = fisics_physical_load_defaults();
  if (!strcmp(argv[3], "sweep")) {
    size_t i = 0;
    for (; i < 2000; i++) {
      o.fail_after = i;
      if (fisics_physical_policy_load_with_options(argv[2], &p, o, &d))
        break;
      CHECK(d.code == 4305 && !memcmp(&p, &before, sizeof(p)));
      CHECK(!strcmp(p.policy_id, "test.physics") &&
            p.fingerprint == before.fingerprint);
    }
    CHECK(i > 40 && i < 2000);
    printf("{\"allocations\":%zu}\n", i);
  } else if (!strcmp(argv[3], "fail")) {
    CHECK(argc == 5);
    CHECK(!fisics_physical_policy_load(argv[2], &p, &d));
    CHECK((int)d.code == atoi(argv[4]) && d.message[0] &&
          !memcmp(&p, &before, sizeof(p)));
    CHECK(fisics_physical_policy_load(argv[1], &p, &d));
    puts("{}");
  } else if ((!strcmp(argv[3], "limit") || !strcmp(argv[3], "allow"))) {
    CHECK(argc == 6);
    size_t n = (size_t)strtoull(argv[5], NULL, 10);
    if (!strcmp(argv[4], "bytes"))
      o.max_file_bytes = n;
    else if (!strcmp(argv[4], "total"))
      o.max_total_bytes = n;
    else if (!strcmp(argv[4], "kinds"))
      o.max_kinds = n;
    else if (!strcmp(argv[4], "contracts"))
      o.max_contracts = n;
    else if (!strcmp(argv[4], "catalogs"))
      o.max_catalogs = n;
    else if (!strcmp(argv[4], "origins"))
      o.max_origins = n;
    else
      CHECK(0);
    if (!strcmp(argv[3], "allow")) {
      CHECK(fisics_physical_policy_load_with_options(argv[2], &p, o, &d));
      CHECK(p.fingerprint == before.fingerprint);
    } else {
      CHECK(!fisics_physical_policy_load_with_options(argv[2], &p, o, &d));
      CHECK(d.code == 4304 && !memcmp(&p, &before, sizeof(p)));
    }
    puts("{}");
  } else {
    CHECK(fisics_physical_policy_load(argv[2], &p, &d));
    if (p.kind_count)
      CHECK(fisics_physical_policy_find_kind(
                &p, p.kinds[p.kind_count - 1].id) == p.kind_count);
    printf("{\"fingerprint\":\"%016" PRIx64 "\",\"registry\":\"%016" PRIx64
           "\",\"kinds\":%zu,\"policy_hash\":\"%016" PRIx64
           "\",\"policy_bytes\":%zu,\"definitions\":[",
           p.fingerprint, p.registry_fingerprint, p.kind_count,
           p.policy_receipt.hash, p.policy_receipt.bytes);
    for (size_t i = 0; i < p.kind_count; i++)
      printf("%s\"%016" PRIx64 "\"", i ? "," : "",
             p.models[i].definition_fingerprint);
    printf("],\"catalog_receipts\":[");
    for (size_t i = 0; i < p.catalog_count; i++)
      printf("%s{\"hash\":\"%016" PRIx64 "\",\"bytes\":%zu}", i ? "," : "",
             p.catalogs[i].receipt.hash, p.catalogs[i].receipt.bytes);
    puts("]}");
  }
  fisics_physical_policy_free(&p);
  fisics_physical_policy_free(&p);
  CHECK(!p.kinds && !p.catalogs && !p.models && !p.fingerprint);
  return 0;
}
