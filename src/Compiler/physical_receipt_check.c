#include "Extensions/PhysicalSemantics/physical_operations.h"
#include "Extensions/PhysicalSemantics/physical_representations.h"
// SPDX-License-Identifier: Apache-2.0
#include "Compiler/physical_energy_receipt.h"
#include "Compiler/physical_receipt.h"
#include "Extensions/PhysicalSemantics/physical_frames.h"
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#define J FisicsJsonValue
static const J *get(const J *o, const char *k) {
  return fisics_json_object_get(o, k);
}
static const char *str(const J *j) {
  return j && j->type == FISICS_JSON_STRING ? j->as.text : NULL;
}
static bool text(const J *j, const char *s) {
  const char *v = str(j);
  return v && !strcmp(v, s);
}
static bool yes(const J *j) {
  return j && j->type == FISICS_JSON_BOOL && j->as.boolean;
}
static bool equal(const J *a, const J *b, unsigned depth) {
  if (!a || !b || a->type != b->type || depth > 32)
    return false;
  switch (a->type) {
  case FISICS_JSON_NULL:
    return true;
  case FISICS_JSON_BOOL:
    return a->as.boolean == b->as.boolean;
  case FISICS_JSON_STRING:
  case FISICS_JSON_NUMBER:
    return !strcmp(a->as.text, b->as.text);
  case FISICS_JSON_ARRAY:
    if (a->as.array.count != b->as.array.count)
      return false;
    for (size_t i = 0; i < a->as.array.count; i++)
      if (!equal(a->as.array.items[i], b->as.array.items[i], depth + 1))
        return false;
    return true;
  case FISICS_JSON_OBJECT:
    if (a->as.object.count != b->as.object.count)
      return false;
    for (size_t i = 0; i < a->as.object.count; i++)
      if (!equal(a->as.object.members[i].value,
                 get(b, a->as.object.members[i].key), depth + 1))
        return false;
    return true;
  }
  return false;
}
static bool current(const J *j) {
  const char *const fields[] = {"path", "hash", "bytes"};
  if (!physical_exact_fields(j, fields, 3))
    return false;
  const char *path = str(get(j, "path")), *hash = str(get(j, "hash"));
  size_t bytes = 0;
  if (!path || path[0] != '/' || !hash || strlen(hash) != 16 ||
      !fisics_json_integer_size(get(j, "bytes"), &bytes))
    return false;
  FisicsPhysicalInputReceipt actual;
  if (!physical_file_receipt(path, &actual))
    return false;
  char expected[17];
  snprintf(expected, sizeof(expected), "%016" PRIx64, actual.hash);
  return actual.bytes == bytes && !strcmp(expected, hash);
}
static bool declarations(const J *array, const FisicsPhysicalPolicy *p) {
  if (!array || array->type != FISICS_JSON_ARRAY ||
      array->as.array.count > 16384)
    return false;
  const char *prior = NULL;
  for (size_t i = 0; i < array->as.array.count; i++) {
    const J *d = array->as.array.items[i];
    const char *const fields[] = {"name",      "entity", "variadic",
                                  "prototype", "slots",  "shape_helper", "frame_transform", "energy_transfer"};
    if (!physical_exact_fields(d, fields, p->schema_version == 7 ? 8 : p->schema_version == 6 ? 7 : p->schema_version == 5 ? 6 : 5))
      return false;
    const char *name = str(get(d, "name"));
    const J *slots = get(d, "slots");
    if (!name || !name[0] || (prior && strcmp(prior, name) >= 0) || !slots ||
        slots->type != FISICS_JSON_ARRAY || !slots->as.array.count ||
        slots->as.array.count > 16384)
      return false;
    prior = name;
    if (!text(get(d, "entity"), "function") &&
        !text(get(d, "entity"), "variable"))
      return false;
    if (!get(d, "prototype") || get(d, "prototype")->type != FISICS_JSON_BOOL ||
        !get(d, "variadic") || get(d, "variadic")->type != FISICS_JSON_BOOL)
      return false;
    for (size_t k = 0; k < slots->as.array.count; k++) {
      const J *s = slots->as.array.items[k];
      const char *const sf[] = {"role", "index",   "fact",     "dimension",
                                "unit", "surface", "c_scalar", "c_record_tag"};
      const char *const ff[] = {"kind",
                                "role",
                                "origin",
                                "carrier",
                                "verified",
                                "definition_fingerprint",
                                "representation",
                                "representation_fingerprint",
                                "component_depth",
                                "frames", "physical_context"};
      const J *f = get(s, "fact");
      if (!physical_exact_fields(s, sf, p->schema_version >= 5 ? 8 : 6) ||
          !(p->schema_version>=6 ? physical_frame_fact_match(f,p) : physical_exact_fields(f, ff, p->schema_version == 5 ? 10 : 6)))
        return false;
      if (!text(get(s, "surface"), "scalar") &&
          !text(get(s, "surface"), "array") &&
          !text(get(s, "surface"), "unsupported") &&
          !(p->schema_version >= 5 &&
            text(get(s, "surface"), "representation")))
        return false;
      size_t ordinal;
      if (!fisics_json_integer_size(get(s, "index"), &ordinal) ||
          !text(get(s, "role"), text(get(d, "entity"), "variable") ? "value"
                                : k ? "parameter"
                                    : "return") ||
          ordinal != (k ? k - 1 : 0) || !get(f, "carrier") ||
          get(f, "carrier")->type != FISICS_JSON_BOOL || !get(f, "verified") ||
          get(f, "verified")->type != FISICS_JSON_BOOL)
        return false;
      if (p->schema_version >= 5) {
        const char *rep = str(get(f, "representation"));
        if (!get(s, "c_scalar") || !get(s, "c_record_tag"))
          return false;
        uint32_t rid = physical_representation_find(p, rep);
        int depth = -1;
        if ((p->schema_version==5 && !text(get(f, "frames"), "unassessed")) ||
            !fisics_json_integer_int(get(f, "component_depth"), &depth) ||
            depth != 0)
          return false;
        if (rep) {
          char hash[17];
          if (!rid || !yes(get(f, "verified")) ||
              !text(get(s, "surface"), "representation") ||
              yes(get(f, "carrier")))
            return false;
          snprintf(hash, sizeof(hash), "%016" PRIx64,
                   p->representations[rid - 1].fingerprint);
          if (!text(get(f, "representation_fingerprint"), hash))
            return false;
          const FisicsPhysicalRepresentation *r = &p->representations[rid - 1];
          if (r->record) {
            const char *tag = str(get(s, "c_record_tag"));
            if (!tag || !tag[0] || get(s, "c_scalar")->type != FISICS_JSON_NULL)
              return false;
          } else if (!text(get(s, "c_scalar"),
                           r->single_precision ? "float" : "double") ||
                     get(s, "c_record_tag")->type != FISICS_JSON_NULL)
            return false;
        } else if (!get(f, "representation") ||
                   get(f, "representation")->type != FISICS_JSON_NULL ||
                   !get(f, "representation_fingerprint") ||
                   get(f, "representation_fingerprint")->type !=
                       FISICS_JSON_NULL ||
                   text(get(s, "surface"), "representation"))
          return false;
      }
      const char *kind = str(get(f, "kind"));
      if (!kind) {
        int role;
        if (!fisics_json_integer_int(get(f, "role"), &role) || role ||
            yes(get(f, "verified")) || !get(f, "origin") ||
            get(f, "origin")->type != FISICS_JSON_NULL ||
            !get(f, "definition_fingerprint") ||
            get(f, "definition_fingerprint")->type != FISICS_JSON_NULL)
          return false;

        if (!get(f, "kind") || get(f, "kind")->type != FISICS_JSON_NULL)
          return false;
        continue;
      }
      FisicsPhysicalKindId id = fisics_physical_policy_find_kind(p, kind);
      int role;
      if (!id || !yes(get(f, "verified")) ||
          !fisics_json_integer_int(get(f, "role"), &role) ||
          role != (int)p->models[id - 1].role)
        return false;
      char definition[17];
      snprintf(definition, sizeof(definition), "%016" PRIx64,
               p->models[id - 1].definition_fingerprint);
      if (!text(get(f, "definition_fingerprint"), definition))
        return false;
      const char *origin = str(get(f, "origin"));
      FisicsPhysicalOriginId oid =
          origin ? fisics_physical_policy_find_origin(p, origin) : 0;
      FisicsPhysicalType t;
      FisicsPhysicalPolicyDiagnostic diag = {0};
      if ((origin && !oid) ||
          !fisics_physical_policy_make_type(p, id, oid, &t, &diag))
        return false;
      const J *dim = get(s, "dimension");
      if (!dim || dim->type != FISICS_JSON_ARRAY || dim->as.array.count != 8)
        return false;
      for (size_t m = 0; m < 8; m++) {
        int v;
        if (!fisics_json_integer_int(dim->as.array.items[m], &v) ||
            v != p->kinds[id - 1].dimension[m])
          return false;
      }
    }
    if (p->schema_version>=6 && !physical_frame_signature_match(d,p)) return false;
    if (p->schema_version==7 && !physical_energy_signature_match(d,p)) return false;
    if (p->schema_version >= 5) {
      const J *hj = get(d, "shape_helper");
      const char *id = str(hj);
      if (!hj)
        return false;
      if (!id) {
        if (hj->type != FISICS_JSON_NULL)
          return false;
      } else {
        const FisicsPhysicalShapeHelper *h = physical_shape_helper_find(p, id);
        if (!h || !text(get(d, "entity"), "function") ||
            slots->as.array.count != (h && h->operation==PHYSICAL_SHAPE_NORM?2u:3u) || yes(get(d, "variadic")) ||
            !yes(get(d, "prototype")))
          return false;
        for (unsigned i = 0; i < slots->as.array.count; i++) {
          const J *s = slots->as.array.items[i], *f = get(s, "fact");
          unsigned hi = i ? i - 1 : 2;
          if (!text(get(f, "kind"), p->kinds[h->kinds[hi] - 1].id))
            return false;
          uint32_t rep = h->representations[hi];
          if (rep) {
            if (!text(get(f, "representation"), p->representations[rep - 1].id))
              return false;
          } else if (get(f, "representation")->type != FISICS_JSON_NULL)
            return false;
        }
      }
    }
  }
  return true;
}
static bool validate(const J *r) {
  const char *const fields[] = {"schema",           "schema_version",
                                "compiler",         "analysis_valid",
                                "dimension_system", "registry_fingerprint",
                                "policy",           "dependencies",
                                "options",          "declarations",
                                "selection",        "expressions",
                                "obligations",      "declaration_bindings",
                                "operation_model",  "representation_model", "frame_model", "energy_model"};
  int version;
  if (!fisics_json_integer_int(get(r, "schema_version"), &version) ||
      (version != 1 && version != 2 && version != 3 && version != 4 &&
       version != 5 && version != 6 && version != 7))
    return false;
  if (!physical_exact_fields(r, fields,
                             version == 1   ? 14
                             : version == 7 ? 18
                             : version == 6 ? 17
                             : version == 5 ? 16
                                            : 15) ||
      !text(get(r, "schema"), "fisiCs.physical_semantics") ||
      !text(get(r, "compiler"), FISICS_VERSION) ||
      !yes(get(r, "analysis_valid")) ||
      !text(get(r, "dimension_system"), "fisiCs.si_dim8.v1"))
    return false;
  const J *options = get(r, "options");
  const char *const of[] = {"dialect",       "compatibility",
                            "overlays",      "trigraphs",
                            "target",        "data_layout",
                            "source",        "defines",
                            "include_paths", "effective_include_paths"};
  if (!physical_exact_fields(options, of, 10))
    return false;
  const char *arrays[] = {"defines", "include_paths",
                          "effective_include_paths"};
  for (size_t i = 0; i < 3; i++) {
    const J *a = get(options, arrays[i]);
    if (!a || a->type != FISICS_JSON_ARRAY || a->as.array.count > 4096)
      return false;
    for (size_t j = 0; j < a->as.array.count; j++)
      if (!str(a->as.array.items[j]))
        return false;
  }
  uint64_t number;
  const char *numbers[] = {"dialect", "compatibility", "overlays"};
  for (size_t i = 0; i < 3; i++)
    if (!fisics_json_integer_u64(get(options, numbers[i]), &number))
      return false;
  if (!get(options, "trigraphs") ||
      get(options, "trigraphs")->type != FISICS_JSON_BOOL)
    return false;
  const char *nullable[] = {"target", "data_layout"};
  for (size_t i = 0; i < 2; i++) {
    const J *v = get(options, nullable[i]);
    if (!v || (v->type != FISICS_JSON_NULL && v->type != FISICS_JSON_STRING))
      return false;
  }
  const J *deps = get(r, "dependencies"), *files = get(deps, "files"),
          *policy = get(r, "policy"), *catalogs = get(policy, "catalogs");
  if (!yes(get(deps, "complete")) || !files ||
      files->type != FISICS_JSON_ARRAY || !files->as.array.count ||
      files->as.array.count > 4096 || !current(get(policy, "input")) ||
      !catalogs || catalogs->type != FISICS_JSON_ARRAY ||
      !catalogs->as.array.count || catalogs->as.array.count > 1024)
    return false;
  const char *source = str(get(get(r, "options"), "source"));
  bool main_found = false;
  const char *prior_file = NULL;
  size_t total_bytes = 0;
  for (size_t i = 0; i < files->as.array.count; i++) {
    const char *path = str(get(files->as.array.items[i], "path"));
    size_t bytes;
    if (!path || (prior_file && strcmp(prior_file, path) >= 0) ||
        !fisics_json_integer_size(get(files->as.array.items[i], "bytes"),
                                  &bytes) ||
        bytes > FISICS_PHYSICAL_INPUT_MAX_BYTES ||
        total_bytes > FISICS_PHYSICAL_TOTAL_MAX_BYTES - bytes)
      return false;
    prior_file = path;
    total_bytes += bytes;

    if (!current(files->as.array.items[i]))
      return false;
    if (source && text(get(files->as.array.items[i], "path"), source))
      main_found = true;
  }
  if (!main_found)
    return false;
  for (size_t i = 0; i < catalogs->as.array.count; i++)
    if (!current(catalogs->as.array.items[i]))
      return false;
  FisicsPhysicalPolicy model = {0};
  FisicsPhysicalPolicyDiagnostic diag = {0};
  const char *path = str(get(get(policy, "input"), "path"));
  if (!fisics_physical_policy_load(path, &model, &diag))
    return false;
  char fp[17], rp[17];
  snprintf(fp, sizeof(fp), "%016" PRIx64, model.fingerprint);
  snprintf(rp, sizeof(rp), "%016" PRIx64, model.registry_fingerprint);
  bool ok = model.schema_version == (unsigned)version &&
            (version == 1 ||
             physical_operations_match(get(r, "operation_model"), &model)) &&
            (version < 5 || physical_representations_match(
                                 get(r, "representation_model"), &model)) &&
            (version < 6 || (physical_frames_match(get(r,"frame_model"),&model) && physical_frame_document_match(r,&model))) &&
            (version != 7 || physical_energy_match(get(r,"energy_model"),&model)) &&
            text(get(policy, "fingerprint"), fp) &&
            text(get(policy, "id"), model.policy_id) &&
            text(get(r, "registry_fingerprint"), rp) &&
            declarations(get(r, "declarations"), &model) &&
            model.catalog_count == catalogs->as.array.count;
  /* The complete loaded catalog set must be present, not merely a subset whose
     bytes happen to be fresh. Source receipts are local evidence, not
     signatures. */
  for (size_t i = 0; ok && i < model.catalog_count; i++) {
    bool found = false;
    for (size_t j = 0; j < catalogs->as.array.count; j++)
      if (text(get(catalogs->as.array.items[j], "path"),
               model.catalogs[i].path))
        found = true;
    ok = found;
  }
  fisics_physical_policy_free(&model);
  return ok;
}
int physical_receipt_check(size_t count, const char *const *paths) {
  if (count < 2 || count > 64) {
    fprintf(stderr,
            "Error: physical receipt check requires 2..64 receipt paths\n");
    return 1;
  }
  J **docs = calloc(count, sizeof(*docs));
  if (!docs)
    return 1;
  FisicsPhysicalLoadContext load = {.options = fisics_physical_load_defaults()};
  FisicsSemanticProjectDiagnostic diag = {0};
  bool ok = true;
  size_t compared = 0;
  for (size_t i = 0; ok && i < count; i++) {
    ok = fisics_physical_read_json(paths[i], FISICS_PHYSICAL_INPUT_MAX_BYTES,
                                   &docs[i], NULL, &load, &diag) &&
         validate(docs[i]);
    if (!ok)
      fprintf(stderr,
              "Error: invalid, incomplete or stale physical receipt: %s\n",
              paths[i]);
  }
  for (size_t i = 0; ok && i < count; i++)
    for (size_t j = 0; ok && j < i; j++) {
      const J *a = docs[i], *b = docs[j];
      if (equal(get(get(a, "options"), "source"),
                get(get(b, "options"), "source"), 0)) {
        ok = false;
        break;
      }
      if (!equal(get(get(a, "policy"), "fingerprint"),
                 get(get(b, "policy"), "fingerprint"), 0) ||
          !equal(get(a, "registry_fingerprint"), get(b, "registry_fingerprint"),
                 0)) {
        ok = false;
        break;
      }
      const char *opts[] = {"dialect",   "compatibility", "overlays",
                            "trigraphs", "target",        "data_layout"};
      for (size_t k = 0; k < 6; k++)
        if (!equal(get(get(a, "options"), opts[k]),
                   get(get(b, "options"), opts[k]), 0))
          ok = false;
      const J *ad = get(a, "declarations"), *bd = get(b, "declarations");
      size_t x = 0, y = 0;
      while (ok && x < ad->as.array.count && y < bd->as.array.count) {
        const J *aa = ad->as.array.items[x], *bb = bd->as.array.items[y];
        int order = strcmp(str(get(aa, "name")), str(get(bb, "name")));
        if (order < 0) {
          x++;
          continue;
        }
        if (order > 0) {
          y++;
          continue;
        }
        if (!equal(aa, bb, 0) || !yes(get(aa, "prototype"))) {
          ok = false;
          break;
        }
        const J *slots = get(aa, "slots");
        for (size_t k = 0; k < slots->as.array.count; k++)
          if (text(get(slots->as.array.items[k], "surface"), "unsupported")) {
            fprintf(stderr, "Error: unsupported shared physical signature "
                            "surface (pointer, callback, record or alias)\n");
            ok = false;
            break;
          }

        for (size_t k = 0; k < slots->as.array.count; k++)
          if (str(get(get(slots->as.array.items[k], "fact"), "kind")))
            ++compared;
        x++;
        y++;
      }
    }
  if (!compared)
    ok = false;
  if (ok)
    printf("Physical cross-TU declaration consistency: PASS (%zu shared "
           "physical slots); selected bodies are separate obligations\n",
           compared);
  else
    fprintf(stderr, "Error: physical cross-TU receipt/signature consistency "
                    "rejected (requires complete fresh inputs and matching "
                    "canonical physical slots)\n");
  for (size_t i = 0; i < count; i++)
    fisics_json_free(docs[i]);
  free(docs);
  return ok ? 0 : 1;
}
#undef J
