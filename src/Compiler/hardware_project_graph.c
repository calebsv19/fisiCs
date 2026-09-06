// SPDX-License-Identifier: Apache-2.0

#include "Compiler/hardware_project_graph.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "Compiler/semantic_project_json.h"
#include "Extensions/SemanticMetadata/semantic_metadata_parser.h"

#define HARDWARE_INPUT_MAX_BYTES (64u * 1024u * 1024u)
#define HARDWARE_TOTAL_INPUT_MAX_BYTES (512u * 1024u * 1024u)
#define HARDWARE_MAX_DEFINITIONS 4096u
#define HARDWARE_MAX_INSTANCES 65536u
#define HARDWARE_MAX_CHILDREN 1024u
#define HARDWARE_MAX_SET_ITEMS 256u
#define HARDWARE_MAX_STRING 4096u

typedef struct PhysicalContractInput {
  char *id;
  char *quantity_kind;
  char *value_form;
  char *reference_kind;
  char *state;
  int8_t dimension[8];
} PhysicalContractInput;

typedef struct PhysicalInput {
  char *project_id;
  char *registry_fingerprint;
  FisicsHardwareStringList source_paths;
  PhysicalContractInput *contracts;
  size_t contract_count;
} PhysicalInput;

static void diag(FisicsSemanticProjectDiagnostic *d, const char *fmt, ...) {
  if (!d || d->message[0])
    return;
  va_list args;
  va_start(args, fmt);
  vsnprintf(d->message, sizeof(d->message), fmt, args);
  va_end(args);
}

static char *dup_text(const char *text) {
  if (!text)
    return NULL;
  size_t n = strlen(text) + 1u;
  if (n - 1u > HARDWARE_MAX_STRING)
    return NULL;
  char *out = (char *)malloc(n);
  if (out)
    memcpy(out, text, n);
  return out;
}

static bool canonical_name(const char *text) {
  const char *detail = NULL;
  return text && strlen(text) <= 127u &&
         fisics_semantic_metadata_is_canonical_name(text, &detail);
}

static bool revision_valid(const char *text) {
  if (!text || !text[0] || strlen(text) > 64u)
    return false;
  for (const unsigned char *p = (const unsigned char *)text; *p; ++p) {
    if (*p < 0x21u || *p > 0x7eu)
      return false;
  }
  return true;
}

static bool exact_fields(const FisicsJsonValue *object,
                         const char *const *fields, size_t count) {
  if (!object || object->type != FISICS_JSON_OBJECT ||
      object->as.object.count != count)
    return false;
  for (size_t i = 0; i < count; ++i) {
    if (!fisics_json_object_get(object, fields[i]))
      return false;
  }
  return true;
}

static const char *string_value(const FisicsJsonValue *object,
                                const char *key) {
  const FisicsJsonValue *value = fisics_json_object_get(object, key);
  return value && value->type == FISICS_JSON_STRING ? value->as.text : NULL;
}

static bool nullable_string(const FisicsJsonValue *object, const char *key,
                            char **out) {
  const FisicsJsonValue *value = fisics_json_object_get(object, key);
  if (!value ||
      (value->type != FISICS_JSON_NULL && value->type != FISICS_JSON_STRING)) {
    return false;
  }
  if (value->type == FISICS_JSON_NULL) {
    *out = NULL;
    return true;
  }
  *out = dup_text(value->as.text);
  return *out != NULL;
}

static uint64_t hash_bytes(const unsigned char *bytes, size_t length) {
  uint64_t hash = UINT64_C(1469598103934665603);
  for (size_t i = 0; i < length; ++i) {
    hash ^= bytes[i];
    hash *= UINT64_C(1099511628211);
  }
  return hash;
}

static bool parse_hex64(const char *text, uint64_t *out) {
  if (!text || strlen(text) != 16u)
    return false;
  uint64_t value = 0;
  for (size_t i = 0; i < 16u; ++i) {
    unsigned digit;
    if (text[i] >= '0' && text[i] <= '9')
      digit = (unsigned)(text[i] - '0');
    else if (text[i] >= 'a' && text[i] <= 'f')
      digit = (unsigned)(text[i] - 'a' + 10);
    else
      return false;
    value = value * 16u + digit;
  }
  *out = value;
  return true;
}

static bool read_json_file(const char *path, uint64_t *aggregate,
                           FisicsJsonValue **out, uint64_t *out_hash,
                           size_t *out_length,
                           FisicsSemanticProjectDiagnostic *d) {
  struct stat st;
  if (!path || stat(path, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size < 0) {
    diag(d, "hardware project input is not a regular file: %s",
         path ? path : "<null>");
    return false;
  }
  uint64_t size = (uint64_t)st.st_size;
  if (size > HARDWARE_INPUT_MAX_BYTES ||
      *aggregate > HARDWARE_TOTAL_INPUT_MAX_BYTES - size) {
    diag(d, "hardware project inputs exceed frozen byte limits: %s", path);
    return false;
  }
  FILE *file = fopen(path, "rb");
  if (!file) {
    diag(d, "failed to read hardware project input: %s", path);
    return false;
  }
  size_t length = (size_t)size;
  unsigned char *bytes = (unsigned char *)malloc(length + 1u);
  bool ok = bytes && fread(bytes, 1u, length, file) == length &&
            fgetc(file) == EOF && !ferror(file);
  if (fclose(file) != 0)
    ok = false;
  if (!ok) {
    free(bytes);
    diag(d, "failed to read complete hardware project input: %s", path);
    return false;
  }
  bytes[length] = '\0';
  FisicsJsonDiagnostic jd = {0};
  ok = fisics_json_parse((const char *)bytes, length, out, &jd);
  if (!ok)
    diag(d, "invalid hardware project JSON %s: %s", path, jd.message);
  if (ok) {
    *out_hash = hash_bytes(bytes, length);
    *out_length = length;
    *aggregate += size;
  }
  free(bytes);
  return ok;
}

static bool verify_source(const char *path, size_t expected_length,
                          uint64_t expected_hash,
                          FisicsSemanticProjectDiagnostic *d) {
  char *canonical = realpath(path, NULL);
  if (!canonical || strcmp(canonical, path) != 0) {
    free(canonical);
    diag(d, "hardware input physical source is missing or noncanonical: %s",
         path);
    return false;
  }
  free(canonical);
  struct stat st;
  if (stat(path, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size < 0 ||
      (uint64_t)st.st_size != expected_length) {
    diag(d, "stale hardware input physical source length: %s", path);
    return false;
  }
  FILE *file = fopen(path, "rb");
  if (!file) {
    diag(d, "failed to read hardware input physical source: %s", path);
    return false;
  }
  uint64_t hash = UINT64_C(1469598103934665603);
  size_t total = 0;
  unsigned char buffer[16384];
  while (!feof(file)) {
    size_t count = fread(buffer, 1u, sizeof(buffer), file);
    total += count;
    for (size_t i = 0; i < count; ++i) {
      hash ^= buffer[i];
      hash *= UINT64_C(1099511628211);
    }
    if (ferror(file)) {
      fclose(file);
      diag(d, "failed while hashing hardware input physical source: %s", path);
      return false;
    }
  }
  if (fclose(file) != 0 || total != expected_length || hash != expected_hash) {
    diag(d, "stale hardware input physical source hash: %s", path);
    return false;
  }
  return true;
}

static int compare_text_ptrs(const void *left, const void *right) {
  return strcmp(*(const char *const *)left, *(const char *const *)right);
}

static void free_string_list(FisicsHardwareStringList *list) {
  if (!list)
    return;
  for (size_t i = 0; i < list->count; ++i)
    free(list->items[i]);
  free(list->items);
  memset(list, 0, sizeof(*list));
}

static bool parse_string_list(const FisicsJsonValue *value,
                              bool require_canonical, size_t maximum,
                              FisicsHardwareStringList *out, const char *label,
                              FisicsSemanticProjectDiagnostic *d) {
  if (!value || value->type != FISICS_JSON_ARRAY ||
      value->as.array.count > maximum) {
    diag(d, "%s must be an array of at most %zu strings", label, maximum);
    return false;
  }
  if (!value->as.array.count)
    return true;
  out->items = (char **)calloc(value->as.array.count, sizeof(char *));
  if (!out->items) {
    diag(d, "out of memory loading %s", label);
    return false;
  }
  out->count = value->as.array.count;
  for (size_t i = 0; i < out->count; ++i) {
    const FisicsJsonValue *item = value->as.array.items[i];
    if (!item || item->type != FISICS_JSON_STRING || !item->as.text[0] ||
        (require_canonical && !canonical_name(item->as.text))) {
      diag(d, "invalid %s[%zu]", label, i);
      return false;
    }
    out->items[i] = dup_text(item->as.text);
    if (!out->items[i]) {
      diag(d, "out of memory loading %s", label);
      return false;
    }
  }
  qsort(out->items, out->count, sizeof(char *), compare_text_ptrs);
  for (size_t i = 1; i < out->count; ++i) {
    if (strcmp(out->items[i - 1u], out->items[i]) == 0) {
      diag(d, "duplicate %s value: %s", label, out->items[i]);
      return false;
    }
  }
  return true;
}

static bool string_list_contains(const FisicsHardwareStringList *list,
                                 const char *text) {
  for (size_t i = 0; i < list->count; ++i) {
    if (strcmp(list->items[i], text) == 0)
      return true;
  }
  return false;
}

static bool add_origin(FisicsHardwareStringList *origins, const char *path,
                       FisicsSemanticProjectDiagnostic *d) {
  char **next =
      (char **)realloc(origins->items, (origins->count + 1u) * sizeof(char *));
  if (!next) {
    diag(d, "out of memory retaining hardware catalog origin");
    return false;
  }
  origins->items = next;
  origins->items[origins->count] = dup_text(path);
  if (!origins->items[origins->count]) {
    diag(d, "out of memory retaining hardware catalog origin");
    return false;
  }
  origins->count++;
  qsort(origins->items, origins->count, sizeof(char *), compare_text_ptrs);
  return true;
}

static void free_ref(FisicsHardwareRef *ref) {
  if (!ref)
    return;
  free(ref->id);
  free(ref->revision);
  memset(ref, 0, sizeof(*ref));
}

static bool parse_ref(const FisicsJsonValue *value, const char *id_key,
                      FisicsHardwareRef *out) {
  const char *fields[2] = {id_key, "revision"};
  const char *id = string_value(value, id_key);
  const char *revision = string_value(value, "revision");
  if (!exact_fields(value, fields, 2u) || !canonical_name(id) ||
      !revision_valid(revision))
    return false;
  out->id = dup_text(id);
  out->revision = dup_text(revision);
  return out->id && out->revision;
}

static bool canonical_u64(const char *text, uint64_t *out) {
  if (!text || !text[0] || (text[0] == '0' && text[1]))
    return false;
  uint64_t value = 0;
  for (const unsigned char *p = (const unsigned char *)text; *p; ++p) {
    if (*p < '0' || *p > '9')
      return false;
    unsigned digit = (unsigned)(*p - '0');
    if (value > (UINT64_MAX - digit) / 10u)
      return false;
    value = value * 10u + digit;
  }
  *out = value;
  return true;
}

static void free_physical_input(PhysicalInput *input) {
  if (!input)
    return;
  free(input->project_id);
  free(input->registry_fingerprint);
  free_string_list(&input->source_paths);
  for (size_t i = 0; i < input->contract_count; ++i) {
    free(input->contracts[i].id);
    free(input->contracts[i].quantity_kind);
    free(input->contracts[i].value_form);
    free(input->contracts[i].reference_kind);
    free(input->contracts[i].state);
  }
  free(input->contracts);
  memset(input, 0, sizeof(*input));
}

static bool parse_dim8(const FisicsJsonValue *value, int8_t out[8]) {
  if (!value || value->type != FISICS_JSON_ARRAY || value->as.array.count != 8u)
    return false;
  for (size_t i = 0; i < 8u; ++i) {
    int n = 0;
    if (!fisics_json_integer_int(value->as.array.items[i], &n) || n < -127 ||
        n > 127) {
      return false;
    }
    out[i] = (int8_t)n;
  }
  return true;
}

static bool parse_physical_graph(const char *path, const char *expected_project,
                                 uint64_t *aggregate, PhysicalInput *out,
                                 uint64_t *out_hash, size_t *out_length,
                                 FisicsSemanticProjectDiagnostic *d) {
  FisicsJsonValue *root = NULL;
  if (!read_json_file(path, aggregate, &root, out_hash, out_length, d))
    return false;
  static const char *const fields[] = {
      "schema",           "schema_version",
      "producer",         "project",
      "complete",         "physical_acceptance",
      "dimension_system", "unit_registry",
      "coverage",         "semantic_graph_receipt",
      "source_receipts",  "quantity_kinds",
      "units_bindings",   "physical_contracts",
      "semantic_edges",   "unbound_units_evidence"};
  uint64_t version = 0;
  const FisicsJsonValue *complete = fisics_json_object_get(root, "complete");
  const FisicsJsonValue *project = fisics_json_object_get(root, "project");
  const FisicsJsonValue *registry =
      fisics_json_object_get(root, "unit_registry");
  static const char *const project_fields[] = {"id", "manifest_path"};
  static const char *const registry_fields[] = {"id", "fingerprint_fnv1a64"};
  if (!exact_fields(root, fields, 16u) ||
      strcmp(string_value(root, "schema") ? string_value(root, "schema") : "",
             "fisiCs.physical_project_graph") != 0 ||
      !fisics_json_integer_u64(fisics_json_object_get(root, "schema_version"),
                               &version) ||
      version != 1u || !complete || complete->type != FISICS_JSON_BOOL ||
      !complete->as.boolean ||
      strcmp(string_value(root, "physical_acceptance")
                 ? string_value(root, "physical_acceptance")
                 : "",
             "not_assessed") != 0 ||
      !exact_fields(project, project_fields, 2u) ||
      strcmp(string_value(project, "id") ? string_value(project, "id") : "",
             expected_project) != 0 ||
      !exact_fields(registry, registry_fields, 2u) ||
      strcmp(string_value(registry, "id") ? string_value(registry, "id") : "",
             "fisiCs.unit_registry") != 0) {
    diag(d, "hardware input requires a complete matching "
            "fisiCs.physical_project_graph v1");
    goto fail;
  }
  uint64_t fingerprint = 0;
  const char *fp = string_value(registry, "fingerprint_fnv1a64");
  if (!parse_hex64(fp, &fingerprint)) {
    diag(d, "invalid hardware input unit registry fingerprint");
    goto fail;
  }
  (void)fingerprint;
  out->project_id = dup_text(expected_project);
  out->registry_fingerprint = dup_text(fp);
  if (!out->project_id || !out->registry_fingerprint)
    goto oom;

  const FisicsJsonValue *receipts =
      fisics_json_object_get(root, "source_receipts");
  if (!receipts || receipts->type != FISICS_JSON_ARRAY ||
      receipts->as.array.count > HARDWARE_MAX_INSTANCES) {
    diag(d, "invalid hardware input physical source receipts");
    goto fail;
  }
  out->source_paths.count = receipts->as.array.count;
  out->source_paths.items = calloc(out->source_paths.count, sizeof(char *));
  if (out->source_paths.count && !out->source_paths.items)
    goto oom;
  static const char *const receipt_fields[] = {"artifact_path", "source"};
  static const char *const source_fields[] = {"path", "hash_fnv1a64",
                                              "length_bytes"};
  for (size_t i = 0; i < receipts->as.array.count; ++i) {
    const FisicsJsonValue *receipt = receipts->as.array.items[i];
    const FisicsJsonValue *source = fisics_json_object_get(receipt, "source");
    size_t source_length = 0;
    uint64_t source_hash = 0;
    const char *source_path = string_value(source, "path");
    const char *hash_text = string_value(source, "hash_fnv1a64");
    if (!exact_fields(receipt, receipt_fields, 2u) ||
        !exact_fields(source, source_fields, 3u) || !source_path ||
        !parse_hex64(hash_text, &source_hash) ||
        !fisics_json_integer_size(
            fisics_json_object_get(source, "length_bytes"), &source_length) ||
        !verify_source(source_path, source_length, source_hash, d))
      goto fail;
    out->source_paths.items[i] = dup_text(source_path);
    if (!out->source_paths.items[i])
      goto oom;
  }

  const FisicsJsonValue *quantities =
      fisics_json_object_get(root, "quantity_kinds");
  const FisicsJsonValue *contracts =
      fisics_json_object_get(root, "physical_contracts");
  if (!quantities || quantities->type != FISICS_JSON_ARRAY || !contracts ||
      contracts->type != FISICS_JSON_ARRAY ||
      quantities->as.array.count > HARDWARE_MAX_DEFINITIONS ||
      contracts->as.array.count > HARDWARE_MAX_INSTANCES) {
    diag(d, "invalid hardware input physical quantities or contracts");
    goto fail;
  }
  char **quantity_ids =
      (char **)calloc(quantities->as.array.count, sizeof(char *));
  int8_t (*quantity_dims)[8] =
      calloc(quantities->as.array.count, sizeof(*quantity_dims));
  if ((quantities->as.array.count && (!quantity_ids || !quantity_dims))) {
    free(quantity_ids);
    free(quantity_dims);
    goto oom;
  }
  static const char *const quantity_fields[] = {
      "id", "dimension_system", "dimension_vector", "dimension_text",
      "origin_catalog"};
  for (size_t i = 0; i < quantities->as.array.count; ++i) {
    const FisicsJsonValue *q = quantities->as.array.items[i];
    const char *id = string_value(q, "id");
    if (!exact_fields(q, quantity_fields, 5u) || !canonical_name(id) ||
        strcmp(string_value(q, "dimension_system")
                   ? string_value(q, "dimension_system")
                   : "",
               "fisiCs.si_dim8.v1") != 0 ||
        !parse_dim8(fisics_json_object_get(q, "dimension_vector"),
                    quantity_dims[i])) {
      for (size_t j = 0; j < i; ++j)
        free(quantity_ids[j]);
      free(quantity_ids);
      free(quantity_dims);
      diag(d, "invalid hardware input physical quantity at index %zu", i);
      goto fail;
    }
    quantity_ids[i] = dup_text(id);
    if (!quantity_ids[i]) {
      for (size_t j = 0; j < i; ++j)
        free(quantity_ids[j]);
      free(quantity_ids);
      free(quantity_dims);
      goto oom;
    }
  }
  out->contract_count = contracts->as.array.count;
  out->contracts = (PhysicalContractInput *)calloc(out->contract_count,
                                                   sizeof(*out->contracts));
  if (out->contract_count && !out->contracts) {
    for (size_t i = 0; i < quantities->as.array.count; ++i)
      free(quantity_ids[i]);
    free(quantity_ids);
    free(quantity_dims);
    goto oom;
  }
  static const char *const contract_fields[] = {
      "contract_id",   "subject",       "quantity_kind",
      "unit_policy",   "value_form",    "reference_semantics",
      "frame_id",      "state",         "evidence_origin",
      "record_index",  "binding_index", "conversion",
      "origin_catalog"};
  static const char *const form_fields[] = {"kind", "component_count",
                                            "tensor_rank", "tensor_shape"};
  static const char *const reference_fields[] = {"kind", "reference_subject",
                                                 "period_binary64"};
  for (size_t i = 0; i < out->contract_count; ++i) {
    const FisicsJsonValue *c = contracts->as.array.items[i];
    const FisicsJsonValue *form = fisics_json_object_get(c, "value_form");
    const FisicsJsonValue *reference =
        fisics_json_object_get(c, "reference_semantics");
    const char *id = string_value(c, "contract_id");
    const char *quantity = string_value(c, "quantity_kind");
    const char *form_kind = string_value(form, "kind");
    const char *reference_kind = string_value(reference, "kind");
    const char *state = string_value(c, "state");
    if (!exact_fields(c, contract_fields, 13u) || !canonical_name(id) ||
        !canonical_name(quantity) || !exact_fields(form, form_fields, 4u) ||
        !canonical_name(form_kind) ||
        !exact_fields(reference, reference_fields, 3u) ||
        !canonical_name(reference_kind) ||
        (!state || (strcmp(state, "source_compatible") != 0 &&
                    strcmp(state, "source_unverified") != 0 &&
                    strcmp(state, "external_declared") != 0))) {
      diag(d, "invalid hardware input physical contract at index %zu", i);
      for (size_t j = 0; j < quantities->as.array.count; ++j)
        free(quantity_ids[j]);
      free(quantity_ids);
      free(quantity_dims);
      goto fail;
    }
    PhysicalContractInput *target = &out->contracts[i];
    target->id = dup_text(id);
    target->quantity_kind = dup_text(quantity);
    target->value_form = dup_text(form_kind);
    target->reference_kind = dup_text(reference_kind);
    target->state = dup_text(state);
    bool found = false;
    for (size_t j = 0; j < quantities->as.array.count; ++j) {
      if (strcmp(quantity_ids[j], quantity) == 0) {
        memcpy(target->dimension, quantity_dims[j], sizeof(target->dimension));
        found = true;
        break;
      }
    }
    if (!target->id || !target->quantity_kind || !target->value_form ||
        !target->reference_kind || !target->state || !found) {
      diag(d, "hardware input physical contract has unresolved quantity: %s",
           quantity);
      for (size_t j = 0; j < quantities->as.array.count; ++j)
        free(quantity_ids[j]);
      free(quantity_ids);
      free(quantity_dims);
      goto fail;
    }
  }
  for (size_t i = 0; i < quantities->as.array.count; ++i)
    free(quantity_ids[i]);
  free(quantity_ids);
  free(quantity_dims);
  fisics_json_free(root);
  return true;

oom:
  diag(d, "out of memory loading hardware input physical graph");
fail:
  fisics_json_free(root);
  free_physical_input(out);
  return false;
}

static void free_bus_profile(FisicsHardwareBusProfile *p) {
  free(p->id);
  free(p->revision);
  free(p->transport_kind);
  free(p->controller_capability);
  free(p->participant_capability);
  for (size_t i = 0; i < p->space_count; ++i) {
    free(p->spaces[i].space_id);
    free(p->spaces[i].value_kind);
    free(p->spaces[i].minimum);
    free(p->spaces[i].maximum);
    free(p->spaces[i].collision_policy);
  }
  free(p->spaces);
  free_string_list(&p->origins);
  memset(p, 0, sizeof(*p));
}

static void free_board_profile(FisicsHardwareBoardProfile *p) {
  free(p->id);
  free(p->revision);
  free_string_list(&p->capabilities);
  for (size_t i = 0; i < p->endpoint_count; ++i) {
    free(p->endpoints[i].id);
    free_string_list(&p->endpoints[i].capabilities);
    free(p->endpoints[i].allocation_policy);
  }
  free(p->endpoints);
  free_string_list(&p->origins);
  memset(p, 0, sizeof(*p));
}

static void free_device_model(FisicsHardwareDeviceModel *p) {
  free(p->id);
  free(p->revision);
  free_string_list(&p->capabilities);
  free(p->driver_policy);
  for (size_t i = 0; i < p->attachment_port_count; ++i) {
    FisicsHardwareAttachmentPort *a = &p->attachment_ports[i];
    free(a->id);
    free(a->connection_kind);
    free(a->required_capability);
    free(a->required_transport_kind);
  }
  free(p->attachment_ports);
  for (size_t i = 0; i < p->physical_port_count; ++i) {
    FisicsHardwarePhysicalPort *port = &p->physical_ports[i];
    free(port->id);
    free(port->direction);
    free(port->requirement.mode);
    free(port->requirement.quantity_kind);
    free(port->requirement.dimension_system);
    free(port->requirement.value_form);
    free(port->requirement.reference_kind);
  }
  free(p->physical_ports);
  free_string_list(&p->origins);
  memset(p, 0, sizeof(*p));
}

static void free_driver(FisicsHardwareDriverContract *p) {
  free(p->id);
  free(p->revision);
  for (size_t i = 0; i < p->supported_model_count; ++i)
    free_ref(&p->supported_models[i]);
  free(p->supported_models);
  free_string_list(&p->required_transports);
  free_string_list(&p->origins);
  memset(p, 0, sizeof(*p));
}

void fisics_hardware_project_graph_free(FisicsHardwareProjectGraph *g) {
  if (!g)
    return;
  free(g->project_id);
  free(g->manifest_path);
  free(g->physical_graph_path);
  free(g->unit_registry_fingerprint);
  free_string_list(&g->physical_source_paths);
  for (size_t i = 0; i < g->receipt_count; ++i) {
    free(g->receipts[i].path);
    free(g->receipts[i].catalog_id);
    free(g->receipts[i].authority_kind);
    free(g->receipts[i].authority_identity);
  }
  free(g->receipts);
  for (size_t i = 0; i < g->bus_profile_count; ++i)
    free_bus_profile(&g->bus_profiles[i]);
  free(g->bus_profiles);
  for (size_t i = 0; i < g->board_profile_count; ++i)
    free_board_profile(&g->board_profiles[i]);
  free(g->board_profiles);
  for (size_t i = 0; i < g->device_model_count; ++i)
    free_device_model(&g->device_models[i]);
  free(g->device_models);
  for (size_t i = 0; i < g->driver_contract_count; ++i)
    free_driver(&g->driver_contracts[i]);
  free(g->driver_contracts);
  for (size_t i = 0; i < g->node_count; ++i) {
    free(g->nodes[i].id);
    free_ref(&g->nodes[i].board_profile);
  }
  free(g->nodes);
  for (size_t i = 0; i < g->bus_count; ++i) {
    free(g->buses[i].id);
    free(g->buses[i].owner_node_id);
    free(g->buses[i].scope);
    free_ref(&g->buses[i].bus_profile);
    free(g->buses[i].controller_endpoint_id);
  }
  free(g->buses);
  for (size_t i = 0; i < g->device_count; ++i) {
    free(g->devices[i].id);
    free(g->devices[i].owner_node_id);
    free_ref(&g->devices[i].model);
    free_ref(&g->devices[i].driver);
  }
  free(g->devices);
  for (size_t i = 0; i < g->connection_count; ++i) {
    FisicsHardwareConnection *c = &g->connections[i];
    free(c->id);
    free(c->device_id);
    free(c->attachment_port_id);
    free(c->target_kind);
    free(c->bus_id);
    free(c->board_endpoint_id);
    for (size_t j = 0; j < c->claim_count; ++j) {
      free(c->claims[j].space_id);
      free(c->claims[j].value_uint64);
      free(c->claims[j].value_id);
    }
    free(c->claims);
  }
  free(g->connections);
  for (size_t i = 0; i < g->signal_binding_count; ++i) {
    FisicsHardwareSignalBinding *b = &g->signal_bindings[i];
    free(b->id);
    free(b->device_id);
    free(b->physical_port_id);
    free(b->physical_contract_id);
    free(b->physical_evidence_state);
    free(b->compatibility_mode);
  }
  free(g->signal_bindings);
  for (size_t i = 0; i < g->unbound_port_count; ++i) {
    free(g->unbound_ports[i].device_id);
    free(g->unbound_ports[i].port_kind);
    free(g->unbound_ports[i].port_id);
  }
  free(g->unbound_ports);
  memset(g, 0, sizeof(*g));
}

static int compare_space(const void *left, const void *right) {
  const FisicsHardwareAllocationSpace *a = left;
  const FisicsHardwareAllocationSpace *b = right;
  return strcmp(a->space_id, b->space_id);
}

static int compare_endpoint(const void *left, const void *right) {
  return strcmp(((const FisicsHardwareEndpoint *)left)->id,
                ((const FisicsHardwareEndpoint *)right)->id);
}

static int compare_attachment(const void *left, const void *right) {
  return strcmp(((const FisicsHardwareAttachmentPort *)left)->id,
                ((const FisicsHardwareAttachmentPort *)right)->id);
}

static int compare_physical_port(const void *left, const void *right) {
  return strcmp(((const FisicsHardwarePhysicalPort *)left)->id,
                ((const FisicsHardwarePhysicalPort *)right)->id);
}

static int compare_ref_value(const void *left, const void *right) {
  const FisicsHardwareRef *a = left;
  const FisicsHardwareRef *b = right;
  int c = strcmp(a->id, b->id);
  return c ? c : strcmp(a->revision, b->revision);
}

static bool parse_bus_profile(const FisicsJsonValue *value, const char *origin,
                              FisicsHardwareBusProfile *out,
                              FisicsSemanticProjectDiagnostic *d) {
  static const char *const fields[] = {"profile_id",
                                       "revision",
                                       "transport_kind",
                                       "controller_capability",
                                       "participant_capability",
                                       "allocation_spaces"};
  const char *id = string_value(value, "profile_id");
  const char *revision = string_value(value, "revision");
  const char *transport = string_value(value, "transport_kind");
  const char *controller = string_value(value, "controller_capability");
  const char *participant = string_value(value, "participant_capability");
  const FisicsJsonValue *spaces =
      fisics_json_object_get(value, "allocation_spaces");
  if (!exact_fields(value, fields, 6u) || !canonical_name(id) ||
      !revision_valid(revision) || !canonical_name(transport) ||
      !canonical_name(controller) || !canonical_name(participant) || !spaces ||
      spaces->type != FISICS_JSON_ARRAY ||
      spaces->as.array.count > HARDWARE_MAX_SET_ITEMS)
    return false;
  out->id = dup_text(id);
  out->revision = dup_text(revision);
  out->transport_kind = dup_text(transport);
  out->controller_capability = dup_text(controller);
  out->participant_capability = dup_text(participant);
  out->space_count = spaces->as.array.count;
  out->spaces = calloc(out->space_count, sizeof(*out->spaces));
  if (!out->id || !out->revision || !out->transport_kind ||
      !out->controller_capability || !out->participant_capability ||
      (out->space_count && !out->spaces) ||
      !add_origin(&out->origins, origin, d))
    return false;
  static const char *const space_fields[] = {
      "space_id",       "value_kind",     "minimum_uint64",
      "maximum_uint64", "claim_required", "collision_policy"};
  for (size_t i = 0; i < out->space_count; ++i) {
    const FisicsJsonValue *s = spaces->as.array.items[i];
    FisicsHardwareAllocationSpace *target = &out->spaces[i];
    const char *sid = string_value(s, "space_id");
    const char *kind = string_value(s, "value_kind");
    const char *collision = string_value(s, "collision_policy");
    const FisicsJsonValue *required =
        fisics_json_object_get(s, "claim_required");
    if (!exact_fields(s, space_fields, 6u) || !canonical_name(sid) ||
        (!kind || (strcmp(kind, "uint64_decimal") != 0 &&
                   strcmp(kind, "canonical_id") != 0)) ||
        (!collision || (strcmp(collision, "exclusive") != 0 &&
                        strcmp(collision, "shared") != 0)) ||
        !required || required->type != FISICS_JSON_BOOL ||
        !nullable_string(s, "minimum_uint64", &target->minimum) ||
        !nullable_string(s, "maximum_uint64", &target->maximum))
      return false;
    target->space_id = dup_text(sid);
    target->value_kind = dup_text(kind);
    target->collision_policy = dup_text(collision);
    target->claim_required = required->as.boolean;
    if (!target->space_id || !target->value_kind || !target->collision_policy)
      return false;
    uint64_t minimum = 0, maximum = 0;
    if (strcmp(kind, "uint64_decimal") == 0) {
      if (!target->minimum || !target->maximum ||
          !canonical_u64(target->minimum, &minimum) ||
          !canonical_u64(target->maximum, &maximum) || minimum > maximum)
        return false;
    } else if (target->minimum || target->maximum)
      return false;
  }
  qsort(out->spaces, out->space_count, sizeof(*out->spaces), compare_space);
  for (size_t i = 1; i < out->space_count; ++i) {
    if (strcmp(out->spaces[i - 1u].space_id, out->spaces[i].space_id) == 0)
      return false;
  }
  return true;
}

static bool parse_board_profile(const FisicsJsonValue *value,
                                const char *origin,
                                FisicsHardwareBoardProfile *out,
                                FisicsSemanticProjectDiagnostic *d) {
  static const char *const fields[] = {"profile_id", "revision",
                                       "capability_ids", "endpoints"};
  const char *id = string_value(value, "profile_id");
  const char *revision = string_value(value, "revision");
  const FisicsJsonValue *endpoints = fisics_json_object_get(value, "endpoints");
  if (!exact_fields(value, fields, 4u) || !canonical_name(id) ||
      !revision_valid(revision) || !endpoints ||
      endpoints->type != FISICS_JSON_ARRAY ||
      endpoints->as.array.count > HARDWARE_MAX_CHILDREN ||
      !parse_string_list(fisics_json_object_get(value, "capability_ids"), true,
                         HARDWARE_MAX_SET_ITEMS, &out->capabilities,
                         "board capability_ids", d))
    return false;
  out->id = dup_text(id);
  out->revision = dup_text(revision);
  out->endpoint_count = endpoints->as.array.count;
  out->endpoints = calloc(out->endpoint_count, sizeof(*out->endpoints));
  if (!out->id || !out->revision || (out->endpoint_count && !out->endpoints) ||
      !add_origin(&out->origins, origin, d))
    return false;
  static const char *const endpoint_fields[] = {"endpoint_id", "capability_ids",
                                                "allocation_policy"};
  for (size_t i = 0; i < out->endpoint_count; ++i) {
    const FisicsJsonValue *e = endpoints->as.array.items[i];
    FisicsHardwareEndpoint *target = &out->endpoints[i];
    const char *eid = string_value(e, "endpoint_id");
    const char *policy = string_value(e, "allocation_policy");
    if (!exact_fields(e, endpoint_fields, 3u) || !canonical_name(eid) ||
        (!policy ||
         (strcmp(policy, "exclusive") != 0 && strcmp(policy, "shared") != 0)) ||
        !parse_string_list(fisics_json_object_get(e, "capability_ids"), true,
                           HARDWARE_MAX_SET_ITEMS, &target->capabilities,
                           "endpoint capability_ids", d))
      return false;
    target->id = dup_text(eid);
    target->allocation_policy = dup_text(policy);
    if (!target->id || !target->allocation_policy)
      return false;
  }
  qsort(out->endpoints, out->endpoint_count, sizeof(*out->endpoints),
        compare_endpoint);
  for (size_t i = 1; i < out->endpoint_count; ++i) {
    if (strcmp(out->endpoints[i - 1u].id, out->endpoints[i].id) == 0)
      return false;
  }
  return true;
}

static bool parse_requirement(const FisicsJsonValue *value,
                              FisicsHardwarePhysicalRequirement *out) {
  static const char *const fields[] = {
      "mode",       "quantity_kind", "dimension_system", "dimension_vector",
      "value_form", "reference_kind"};
  const char *mode = string_value(value, "mode");
  const FisicsJsonValue *vector =
      fisics_json_object_get(value, "dimension_vector");
  if (!exact_fields(value, fields, 6u) ||
      (!mode || (strcmp(mode, "exact_quantity") != 0 &&
                 strcmp(mode, "dimension_only") != 0)) ||
      !nullable_string(value, "quantity_kind", &out->quantity_kind) ||
      !nullable_string(value, "dimension_system", &out->dimension_system) ||
      !nullable_string(value, "value_form", &out->value_form) ||
      !nullable_string(value, "reference_kind", &out->reference_kind) ||
      !vector || vector->type != FISICS_JSON_ARRAY)
    return false;
  out->mode = dup_text(mode);
  if (!out->mode)
    return false;
  if (out->quantity_kind && !canonical_name(out->quantity_kind))
    return false;
  if (out->value_form && !canonical_name(out->value_form))
    return false;
  if (out->reference_kind && !canonical_name(out->reference_kind))
    return false;
  if (strcmp(mode, "exact_quantity") == 0) {
    return out->quantity_kind && !out->dimension_system &&
           vector->as.array.count == 0u;
  }
  if (out->quantity_kind || !out->dimension_system ||
      strcmp(out->dimension_system, "fisiCs.si_dim8.v1") != 0 ||
      !parse_dim8(vector, out->dimension))
    return false;
  out->has_dimension = true;
  return true;
}

static bool parse_device_model(const FisicsJsonValue *value, const char *origin,
                               FisicsHardwareDeviceModel *out,
                               FisicsSemanticProjectDiagnostic *d) {
  static const char *const fields[] = {"model_id",         "revision",
                                       "capability_ids",   "driver_policy",
                                       "attachment_ports", "physical_ports"};
  const char *id = string_value(value, "model_id");
  const char *revision = string_value(value, "revision");
  const char *policy = string_value(value, "driver_policy");
  const FisicsJsonValue *attachments =
      fisics_json_object_get(value, "attachment_ports");
  const FisicsJsonValue *physical =
      fisics_json_object_get(value, "physical_ports");
  if (!exact_fields(value, fields, 6u) || !canonical_name(id) ||
      !revision_valid(revision) ||
      (!policy ||
       (strcmp(policy, "required") != 0 && strcmp(policy, "optional") != 0 &&
        strcmp(policy, "none") != 0)) ||
      !attachments || attachments->type != FISICS_JSON_ARRAY || !physical ||
      physical->type != FISICS_JSON_ARRAY ||
      attachments->as.array.count > HARDWARE_MAX_CHILDREN ||
      physical->as.array.count > HARDWARE_MAX_CHILDREN ||
      !parse_string_list(fisics_json_object_get(value, "capability_ids"), true,
                         HARDWARE_MAX_SET_ITEMS, &out->capabilities,
                         "device capability_ids", d))
    return false;
  out->id = dup_text(id);
  out->revision = dup_text(revision);
  out->driver_policy = dup_text(policy);
  out->attachment_port_count = attachments->as.array.count;
  out->physical_port_count = physical->as.array.count;
  out->attachment_ports =
      calloc(out->attachment_port_count, sizeof(*out->attachment_ports));
  out->physical_ports =
      calloc(out->physical_port_count, sizeof(*out->physical_ports));
  if (!out->id || !out->revision || !out->driver_policy ||
      (out->attachment_port_count && !out->attachment_ports) ||
      (out->physical_port_count && !out->physical_ports) ||
      !add_origin(&out->origins, origin, d))
    return false;
  static const char *const attachment_fields[] = {
      "port_id", "connection_kind", "required_capability",
      "required_transport_kind", "required"};
  for (size_t i = 0; i < out->attachment_port_count; ++i) {
    const FisicsJsonValue *a = attachments->as.array.items[i];
    FisicsHardwareAttachmentPort *target = &out->attachment_ports[i];
    const char *pid = string_value(a, "port_id");
    const char *kind = string_value(a, "connection_kind");
    const char *capability = string_value(a, "required_capability");
    const FisicsJsonValue *required = fisics_json_object_get(a, "required");
    if (!exact_fields(a, attachment_fields, 5u) || !canonical_name(pid) ||
        !canonical_name(capability) ||
        (!kind || (strcmp(kind, "bus_participant") != 0 &&
                   strcmp(kind, "direct_endpoint") != 0)) ||
        !required || required->type != FISICS_JSON_BOOL ||
        !nullable_string(a, "required_transport_kind",
                         &target->required_transport_kind))
      return false;
    if ((strcmp(kind, "bus_participant") == 0) !=
            (target->required_transport_kind != NULL) ||
        (target->required_transport_kind &&
         !canonical_name(target->required_transport_kind)))
      return false;
    target->id = dup_text(pid);
    target->connection_kind = dup_text(kind);
    target->required_capability = dup_text(capability);
    target->required = required->as.boolean;
    if (!target->id || !target->connection_kind || !target->required_capability)
      return false;
  }
  qsort(out->attachment_ports, out->attachment_port_count,
        sizeof(*out->attachment_ports), compare_attachment);
  for (size_t i = 1; i < out->attachment_port_count; ++i) {
    if (strcmp(out->attachment_ports[i - 1u].id, out->attachment_ports[i].id) ==
        0)
      return false;
  }
  static const char *const physical_fields[] = {"port_id", "direction",
                                                "required", "requirement"};
  for (size_t i = 0; i < out->physical_port_count; ++i) {
    const FisicsJsonValue *p = physical->as.array.items[i];
    FisicsHardwarePhysicalPort *target = &out->physical_ports[i];
    const char *pid = string_value(p, "port_id");
    const char *direction = string_value(p, "direction");
    const FisicsJsonValue *required = fisics_json_object_get(p, "required");
    if (!exact_fields(p, physical_fields, 4u) || !canonical_name(pid) ||
        (!direction || (strcmp(direction, "produces") != 0 &&
                        strcmp(direction, "consumes") != 0 &&
                        strcmp(direction, "bidirectional") != 0)) ||
        !required || required->type != FISICS_JSON_BOOL ||
        !parse_requirement(fisics_json_object_get(p, "requirement"),
                           &target->requirement))
      return false;
    target->id = dup_text(pid);
    target->direction = dup_text(direction);
    target->required = required->as.boolean;
    if (!target->id || !target->direction)
      return false;
  }
  qsort(out->physical_ports, out->physical_port_count,
        sizeof(*out->physical_ports), compare_physical_port);
  for (size_t i = 1; i < out->physical_port_count; ++i) {
    if (strcmp(out->physical_ports[i - 1u].id, out->physical_ports[i].id) == 0)
      return false;
  }
  return true;
}

static bool parse_driver(const FisicsJsonValue *value, const char *origin,
                         FisicsHardwareDriverContract *out,
                         FisicsSemanticProjectDiagnostic *d) {
  static const char *const fields[] = {
      "driver_id", "revision", "supported_models", "required_transport_kinds"};
  const char *id = string_value(value, "driver_id");
  const char *revision = string_value(value, "revision");
  const FisicsJsonValue *models =
      fisics_json_object_get(value, "supported_models");
  if (!exact_fields(value, fields, 4u) || !canonical_name(id) ||
      !revision_valid(revision) || !models ||
      models->type != FISICS_JSON_ARRAY ||
      models->as.array.count > HARDWARE_MAX_SET_ITEMS ||
      !parse_string_list(
          fisics_json_object_get(value, "required_transport_kinds"), true,
          HARDWARE_MAX_SET_ITEMS, &out->required_transports,
          "driver required_transport_kinds", d))
    return false;
  out->id = dup_text(id);
  out->revision = dup_text(revision);
  out->supported_model_count = models->as.array.count;
  out->supported_models =
      calloc(out->supported_model_count, sizeof(*out->supported_models));
  if (!out->id || !out->revision ||
      (out->supported_model_count && !out->supported_models) ||
      !add_origin(&out->origins, origin, d))
    return false;
  for (size_t i = 0; i < out->supported_model_count; ++i) {
    if (!parse_ref(models->as.array.items[i], "model_id",
                   &out->supported_models[i]))
      return false;
  }
  qsort(out->supported_models, out->supported_model_count,
        sizeof(*out->supported_models), compare_ref_value);
  for (size_t i = 1; i < out->supported_model_count; ++i) {
    if (compare_ref_value(&out->supported_models[i - 1u],
                          &out->supported_models[i]) == 0)
      return false;
  }
  return true;
}

static bool string_list_equal(const FisicsHardwareStringList *a,
                              const FisicsHardwareStringList *b) {
  if (a->count != b->count)
    return false;
  for (size_t i = 0; i < a->count; ++i) {
    if (strcmp(a->items[i], b->items[i]) != 0)
      return false;
  }
  return true;
}

static bool bus_equal(const FisicsHardwareBusProfile *a,
                      const FisicsHardwareBusProfile *b) {
  if (strcmp(a->transport_kind, b->transport_kind) ||
      strcmp(a->controller_capability, b->controller_capability) ||
      strcmp(a->participant_capability, b->participant_capability) ||
      a->space_count != b->space_count)
    return false;
  for (size_t i = 0; i < a->space_count; ++i) {
    const FisicsHardwareAllocationSpace *x = &a->spaces[i];
    const FisicsHardwareAllocationSpace *y = &b->spaces[i];
    if (strcmp(x->space_id, y->space_id) ||
        strcmp(x->value_kind, y->value_kind) ||
        strcmp(x->collision_policy, y->collision_policy) ||
        x->claim_required != y->claim_required ||
        !!x->minimum != !!y->minimum || !!x->maximum != !!y->maximum ||
        (x->minimum && strcmp(x->minimum, y->minimum)) ||
        (x->maximum && strcmp(x->maximum, y->maximum)))
      return false;
  }
  return true;
}

static bool board_equal(const FisicsHardwareBoardProfile *a,
                        const FisicsHardwareBoardProfile *b) {
  if (!string_list_equal(&a->capabilities, &b->capabilities) ||
      a->endpoint_count != b->endpoint_count)
    return false;
  for (size_t i = 0; i < a->endpoint_count; ++i) {
    if (strcmp(a->endpoints[i].id, b->endpoints[i].id) ||
        strcmp(a->endpoints[i].allocation_policy,
               b->endpoints[i].allocation_policy) ||
        !string_list_equal(&a->endpoints[i].capabilities,
                           &b->endpoints[i].capabilities))
      return false;
  }
  return true;
}

static bool requirement_equal(const FisicsHardwarePhysicalRequirement *a,
                              const FisicsHardwarePhysicalRequirement *b) {
  return strcmp(a->mode, b->mode) == 0 &&
         !!a->quantity_kind == !!b->quantity_kind &&
         (!a->quantity_kind ||
          strcmp(a->quantity_kind, b->quantity_kind) == 0) &&
         !!a->dimension_system == !!b->dimension_system &&
         (!a->dimension_system ||
          strcmp(a->dimension_system, b->dimension_system) == 0) &&
         a->has_dimension == b->has_dimension &&
         (!a->has_dimension || memcmp(a->dimension, b->dimension, 8u) == 0) &&
         !!a->value_form == !!b->value_form &&
         (!a->value_form || strcmp(a->value_form, b->value_form) == 0) &&
         !!a->reference_kind == !!b->reference_kind &&
         (!a->reference_kind ||
          strcmp(a->reference_kind, b->reference_kind) == 0);
}

static bool model_equal(const FisicsHardwareDeviceModel *a,
                        const FisicsHardwareDeviceModel *b) {
  if (strcmp(a->driver_policy, b->driver_policy) ||
      !string_list_equal(&a->capabilities, &b->capabilities) ||
      a->attachment_port_count != b->attachment_port_count ||
      a->physical_port_count != b->physical_port_count)
    return false;
  for (size_t i = 0; i < a->attachment_port_count; ++i) {
    const FisicsHardwareAttachmentPort *x = &a->attachment_ports[i];
    const FisicsHardwareAttachmentPort *y = &b->attachment_ports[i];
    if (strcmp(x->id, y->id) ||
        strcmp(x->connection_kind, y->connection_kind) ||
        strcmp(x->required_capability, y->required_capability) ||
        x->required != y->required ||
        !!x->required_transport_kind != !!y->required_transport_kind ||
        (x->required_transport_kind &&
         strcmp(x->required_transport_kind, y->required_transport_kind)))
      return false;
  }
  for (size_t i = 0; i < a->physical_port_count; ++i) {
    const FisicsHardwarePhysicalPort *x = &a->physical_ports[i];
    const FisicsHardwarePhysicalPort *y = &b->physical_ports[i];
    if (strcmp(x->id, y->id) || strcmp(x->direction, y->direction) ||
        x->required != y->required ||
        !requirement_equal(&x->requirement, &y->requirement))
      return false;
  }
  return true;
}

static bool driver_equal(const FisicsHardwareDriverContract *a,
                         const FisicsHardwareDriverContract *b) {
  if (a->supported_model_count != b->supported_model_count ||
      !string_list_equal(&a->required_transports, &b->required_transports))
    return false;
  for (size_t i = 0; i < a->supported_model_count; ++i) {
    if (compare_ref_value(&a->supported_models[i], &b->supported_models[i]) !=
        0)
      return false;
  }
  return true;
}

static bool same_key(const char *aid, const char *ar, const char *bid,
                     const char *br) {
  return strcmp(aid, bid) == 0 && strcmp(ar, br) == 0;
}

#define APPEND_ITEM(array, count, value, type, label)                          \
  do {                                                                         \
    type *next_items = realloc((array), ((count) + 1u) * sizeof(type));        \
    if (!next_items) {                                                         \
      diag(d, "out of memory appending %s", (label));                          \
      return false;                                                            \
    }                                                                          \
    (array) = next_items;                                                      \
    (array)[(count)++] = (value);                                              \
  } while (0)

static bool merge_bus(FisicsHardwareProjectGraph *g,
                      FisicsHardwareBusProfile *item,
                      FisicsSemanticProjectDiagnostic *d) {
  for (size_t i = 0; i < g->bus_profile_count; ++i) {
    if (!same_key(g->bus_profiles[i].id, g->bus_profiles[i].revision, item->id,
                  item->revision))
      continue;
    if (!bus_equal(&g->bus_profiles[i], item)) {
      diag(d, "conflicting hardware bus profile %s revision %s from %s and %s",
           item->id, item->revision, g->bus_profiles[i].origins.items[0],
           item->origins.items[0]);
      return false;
    }
    bool ok =
        add_origin(&g->bus_profiles[i].origins, item->origins.items[0], d);
    free_bus_profile(item);
    return ok;
  }
  APPEND_ITEM(g->bus_profiles, g->bus_profile_count, *item,
              FisicsHardwareBusProfile, "bus profile");
  memset(item, 0, sizeof(*item));
  return true;
}

static bool merge_board(FisicsHardwareProjectGraph *g,
                        FisicsHardwareBoardProfile *item,
                        FisicsSemanticProjectDiagnostic *d) {
  for (size_t i = 0; i < g->board_profile_count; ++i) {
    if (!same_key(g->board_profiles[i].id, g->board_profiles[i].revision,
                  item->id, item->revision))
      continue;
    if (!board_equal(&g->board_profiles[i], item)) {
      diag(d,
           "conflicting hardware board profile %s revision %s from %s and %s",
           item->id, item->revision, g->board_profiles[i].origins.items[0],
           item->origins.items[0]);
      return false;
    }
    bool ok =
        add_origin(&g->board_profiles[i].origins, item->origins.items[0], d);
    free_board_profile(item);
    return ok;
  }
  APPEND_ITEM(g->board_profiles, g->board_profile_count, *item,
              FisicsHardwareBoardProfile, "board profile");
  memset(item, 0, sizeof(*item));
  return true;
}

static bool merge_model(FisicsHardwareProjectGraph *g,
                        FisicsHardwareDeviceModel *item,
                        FisicsSemanticProjectDiagnostic *d) {
  for (size_t i = 0; i < g->device_model_count; ++i) {
    if (!same_key(g->device_models[i].id, g->device_models[i].revision,
                  item->id, item->revision))
      continue;
    if (!model_equal(&g->device_models[i], item)) {
      diag(d, "conflicting hardware device model %s revision %s from %s and %s",
           item->id, item->revision, g->device_models[i].origins.items[0],
           item->origins.items[0]);
      return false;
    }
    bool ok =
        add_origin(&g->device_models[i].origins, item->origins.items[0], d);
    free_device_model(item);
    return ok;
  }
  APPEND_ITEM(g->device_models, g->device_model_count, *item,
              FisicsHardwareDeviceModel, "device model");
  memset(item, 0, sizeof(*item));
  return true;
}

static bool merge_driver(FisicsHardwareProjectGraph *g,
                         FisicsHardwareDriverContract *item,
                         FisicsSemanticProjectDiagnostic *d) {
  for (size_t i = 0; i < g->driver_contract_count; ++i) {
    if (!same_key(g->driver_contracts[i].id, g->driver_contracts[i].revision,
                  item->id, item->revision))
      continue;
    if (!driver_equal(&g->driver_contracts[i], item)) {
      diag(d,
           "conflicting hardware driver contract %s revision %s from %s and %s",
           item->id, item->revision, g->driver_contracts[i].origins.items[0],
           item->origins.items[0]);
      return false;
    }
    bool ok =
        add_origin(&g->driver_contracts[i].origins, item->origins.items[0], d);
    free_driver(item);
    return ok;
  }
  APPEND_ITEM(g->driver_contracts, g->driver_contract_count, *item,
              FisicsHardwareDriverContract, "driver contract");
  memset(item, 0, sizeof(*item));
  return true;
}

static bool append_receipt(FisicsHardwareProjectGraph *g, const char *path,
                           uint64_t hash, size_t length, const char *catalog_id,
                           const char *authority_kind,
                           const char *authority_identity,
                           FisicsSemanticProjectDiagnostic *d) {
  FisicsHardwareReceipt receipt = {0};
  receipt.path = dup_text(path);
  receipt.hash = hash;
  receipt.length = length;
  receipt.catalog_id = dup_text(catalog_id);
  receipt.authority_kind = dup_text(authority_kind);
  receipt.authority_identity = dup_text(authority_identity);
  if (!receipt.path || (catalog_id && !receipt.catalog_id) ||
      (authority_kind && !receipt.authority_kind) ||
      (authority_identity && !receipt.authority_identity)) {
    free(receipt.path);
    free(receipt.catalog_id);
    free(receipt.authority_kind);
    free(receipt.authority_identity);
    diag(d, "out of memory retaining hardware input receipt");
    return false;
  }
  APPEND_ITEM(g->receipts, g->receipt_count, receipt, FisicsHardwareReceipt,
              "hardware receipt");
  return true;
}

static bool parse_catalog(const char *path, uint64_t *aggregate,
                          FisicsHardwareProjectGraph *g,
                          FisicsSemanticProjectDiagnostic *d) {
  FisicsJsonValue *root = NULL;
  uint64_t hash = 0;
  size_t length = 0;
  if (!read_json_file(path, aggregate, &root, &hash, &length, d))
    return false;
  static const char *const fields[] = {
      "schema",       "schema_version", "catalog_id",    "authority",
      "bus_profiles", "board_profiles", "device_models", "driver_contracts"};
  static const char *const authority_fields[] = {"kind", "identity"};
  uint64_t version = 0;
  const char *catalog_id = string_value(root, "catalog_id");
  const FisicsJsonValue *authority = fisics_json_object_get(root, "authority");
  const char *authority_kind = string_value(authority, "kind");
  const char *authority_id = string_value(authority, "identity");
  if (!exact_fields(root, fields, 8u) ||
      strcmp(string_value(root, "schema") ? string_value(root, "schema") : "",
             "fisiCs.hardware_component_catalog") != 0 ||
      !fisics_json_integer_u64(fisics_json_object_get(root, "schema_version"),
                               &version) ||
      version != 1u || !canonical_name(catalog_id) ||
      !exact_fields(authority, authority_fields, 2u) ||
      !canonical_name(authority_id) ||
      (!authority_kind || (strcmp(authority_kind, "project") != 0 &&
                           strcmp(authority_kind, "external_declared") != 0))) {
    diag(d, "invalid hardware component catalog header: %s", path);
    goto fail;
  }
  for (size_t i = 0; i < g->receipt_count; ++i) {
    if (g->receipts[i].catalog_id &&
        strcmp(g->receipts[i].catalog_id, catalog_id) == 0) {
      diag(d, "duplicate hardware catalog_id %s from %s and %s", catalog_id,
           g->receipts[i].path, path);
      goto fail;
    }
  }
  if (!append_receipt(g, path, hash, length, catalog_id, authority_kind,
                      authority_id, d))
    goto fail;
  const char *names[] = {"bus_profiles", "board_profiles", "device_models",
                         "driver_contracts"};
  for (size_t kind = 0; kind < 4u; ++kind) {
    const FisicsJsonValue *array = fisics_json_object_get(root, names[kind]);
    if (!array || array->type != FISICS_JSON_ARRAY ||
        array->as.array.count > HARDWARE_MAX_DEFINITIONS) {
      diag(d, "invalid %s array in hardware catalog: %s", names[kind], path);
      goto fail;
    }
    for (size_t i = 0; i < array->as.array.count; ++i) {
      bool ok = false;
      if (kind == 0u) {
        FisicsHardwareBusProfile item = {0};
        ok = parse_bus_profile(array->as.array.items[i], path, &item, d) &&
             merge_bus(g, &item, d);
        if (!ok)
          free_bus_profile(&item);
      } else if (kind == 1u) {
        FisicsHardwareBoardProfile item = {0};
        ok = parse_board_profile(array->as.array.items[i], path, &item, d) &&
             merge_board(g, &item, d);
        if (!ok)
          free_board_profile(&item);
      } else if (kind == 2u) {
        FisicsHardwareDeviceModel item = {0};
        ok = parse_device_model(array->as.array.items[i], path, &item, d) &&
             merge_model(g, &item, d);
        if (!ok)
          free_device_model(&item);
      } else {
        FisicsHardwareDriverContract item = {0};
        ok = parse_driver(array->as.array.items[i], path, &item, d) &&
             merge_driver(g, &item, d);
        if (!ok)
          free_driver(&item);
      }
      if (!ok) {
        if (!d->message[0])
          diag(d, "invalid %s[%zu] in %s", names[kind], i, path);
        goto fail;
      }
    }
  }
  fisics_json_free(root);
  return true;
fail:
  fisics_json_free(root);
  return false;
}

static int compare_bus_profile_key(const void *left, const void *right) {
  const FisicsHardwareBusProfile *a = left;
  const FisicsHardwareBusProfile *b = right;
  int c = strcmp(a->id, b->id);
  return c ? c : strcmp(a->revision, b->revision);
}
static int compare_board_profile_key(const void *left, const void *right) {
  const FisicsHardwareBoardProfile *a = left;
  const FisicsHardwareBoardProfile *b = right;
  int c = strcmp(a->id, b->id);
  return c ? c : strcmp(a->revision, b->revision);
}
static int compare_model_key(const void *left, const void *right) {
  const FisicsHardwareDeviceModel *a = left;
  const FisicsHardwareDeviceModel *b = right;
  int c = strcmp(a->id, b->id);
  return c ? c : strcmp(a->revision, b->revision);
}
static int compare_driver_key(const void *left, const void *right) {
  const FisicsHardwareDriverContract *a = left;
  const FisicsHardwareDriverContract *b = right;
  int c = strcmp(a->id, b->id);
  return c ? c : strcmp(a->revision, b->revision);
}
static int compare_node_key(const void *left, const void *right) {
  return strcmp(((const FisicsHardwareNode *)left)->id,
                ((const FisicsHardwareNode *)right)->id);
}
static int compare_bus_key(const void *left, const void *right) {
  return strcmp(((const FisicsHardwareBus *)left)->id,
                ((const FisicsHardwareBus *)right)->id);
}
static int compare_device_key(const void *left, const void *right) {
  return strcmp(((const FisicsHardwareDevice *)left)->id,
                ((const FisicsHardwareDevice *)right)->id);
}
static int compare_connection_key(const void *left, const void *right) {
  return strcmp(((const FisicsHardwareConnection *)left)->id,
                ((const FisicsHardwareConnection *)right)->id);
}
static int compare_binding_key(const void *left, const void *right) {
  return strcmp(((const FisicsHardwareSignalBinding *)left)->id,
                ((const FisicsHardwareSignalBinding *)right)->id);
}
static int compare_receipt_key(const void *left, const void *right) {
  return strcmp(((const FisicsHardwareReceipt *)left)->path,
                ((const FisicsHardwareReceipt *)right)->path);
}
static int compare_unbound_key(const void *left, const void *right) {
  const FisicsHardwareUnboundPort *a = left;
  const FisicsHardwareUnboundPort *b = right;
  int c = strcmp(a->device_id, b->device_id);
  if (!c)
    c = strcmp(a->port_kind, b->port_kind);
  return c ? c : strcmp(a->port_id, b->port_id);
}
static int compare_claim_key(const void *left, const void *right) {
  return strcmp(((const FisicsHardwareAllocationClaim *)left)->space_id,
                ((const FisicsHardwareAllocationClaim *)right)->space_id);
}

static bool parse_topology(const char *path, const char *project_id,
                           uint64_t *aggregate, FisicsHardwareProjectGraph *g,
                           FisicsSemanticProjectDiagnostic *d) {
  FisicsJsonValue *root = NULL;
  uint64_t hash = 0;
  size_t length = 0;
  if (!read_json_file(path, aggregate, &root, &hash, &length, d))
    return false;
  static const char *const fields[] = {
      "schema", "schema_version", "project_id",  "nodes",
      "buses",  "devices",        "connections", "signal_bindings"};
  uint64_t version = 0;
  if (!exact_fields(root, fields, 8u) ||
      strcmp(string_value(root, "schema") ? string_value(root, "schema") : "",
             "fisiCs.hardware_topology") != 0 ||
      !fisics_json_integer_u64(fisics_json_object_get(root, "schema_version"),
                               &version) ||
      version != 1u ||
      strcmp(string_value(root, "project_id") ? string_value(root, "project_id")
                                              : "",
             project_id) != 0) {
    diag(d, "hardware topology requires matching fisiCs.hardware_topology "
            "schema v1");
    goto fail;
  }
  if (!append_receipt(g, path, hash, length, NULL, NULL, NULL, d))
    goto fail;
  const FisicsJsonValue *nodes = fisics_json_object_get(root, "nodes");
  const FisicsJsonValue *buses = fisics_json_object_get(root, "buses");
  const FisicsJsonValue *devices = fisics_json_object_get(root, "devices");
  const FisicsJsonValue *connections =
      fisics_json_object_get(root, "connections");
  const FisicsJsonValue *bindings =
      fisics_json_object_get(root, "signal_bindings");
  const FisicsJsonValue *arrays[] = {nodes, buses, devices, connections,
                                     bindings};
  size_t total_instances = 0u;
  for (size_t i = 0; i < 5u; ++i) {
    if (!arrays[i] || arrays[i]->type != FISICS_JSON_ARRAY ||
        arrays[i]->as.array.count > HARDWARE_MAX_INSTANCES) {
      diag(d, "hardware topology instance array exceeds frozen limits");
      goto fail;
    }
    if (arrays[i]->as.array.count > HARDWARE_MAX_INSTANCES - total_instances) {
      diag(d,
           "hardware topology combined instance count exceeds frozen limits");
      goto fail;
    }
    total_instances += arrays[i]->as.array.count;
  }
  g->node_count = nodes->as.array.count;
  g->nodes = calloc(g->node_count, sizeof(*g->nodes));
  g->bus_count = buses->as.array.count;
  g->buses = calloc(g->bus_count, sizeof(*g->buses));
  g->device_count = devices->as.array.count;
  g->devices = calloc(g->device_count, sizeof(*g->devices));
  g->connection_count = connections->as.array.count;
  g->connections = calloc(g->connection_count, sizeof(*g->connections));
  g->signal_binding_count = bindings->as.array.count;
  g->signal_bindings =
      calloc(g->signal_binding_count, sizeof(*g->signal_bindings));
  if ((g->node_count && !g->nodes) || (g->bus_count && !g->buses) ||
      (g->device_count && !g->devices) ||
      (g->connection_count && !g->connections) ||
      (g->signal_binding_count && !g->signal_bindings))
    goto oom;

  static const char *const node_fields[] = {"node_id", "board_profile"};
  for (size_t i = 0; i < g->node_count; ++i) {
    const FisicsJsonValue *n = nodes->as.array.items[i];
    const char *id = string_value(n, "node_id");
    if (!exact_fields(n, node_fields, 2u) || !canonical_name(id) ||
        !parse_ref(fisics_json_object_get(n, "board_profile"), "profile_id",
                   &g->nodes[i].board_profile)) {
      diag(d, "invalid hardware node at index %zu", i);
      goto fail;
    }
    g->nodes[i].id = dup_text(id);
    if (!g->nodes[i].id)
      goto oom;
  }
  static const char *const bus_fields[] = {"bus_id", "owner_node_id", "scope",
                                           "bus_profile",
                                           "controller_endpoint_id"};
  for (size_t i = 0; i < g->bus_count; ++i) {
    const FisicsJsonValue *b = buses->as.array.items[i];
    const char *id = string_value(b, "bus_id");
    const char *owner = string_value(b, "owner_node_id");
    const char *scope = string_value(b, "scope");
    const char *endpoint = string_value(b, "controller_endpoint_id");
    if (!exact_fields(b, bus_fields, 5u) || !canonical_name(id) ||
        !canonical_name(owner) || !canonical_name(endpoint) ||
        (!scope || (strcmp(scope, "node_local") != 0 &&
                    strcmp(scope, "project_shared") != 0)) ||
        !parse_ref(fisics_json_object_get(b, "bus_profile"), "profile_id",
                   &g->buses[i].bus_profile)) {
      diag(d, "invalid hardware bus at index %zu", i);
      goto fail;
    }
    g->buses[i].id = dup_text(id);
    g->buses[i].owner_node_id = dup_text(owner);
    g->buses[i].scope = dup_text(scope);
    g->buses[i].controller_endpoint_id = dup_text(endpoint);
    if (!g->buses[i].id || !g->buses[i].owner_node_id || !g->buses[i].scope ||
        !g->buses[i].controller_endpoint_id)
      goto oom;
  }
  static const char *const device_fields[] = {"device_id", "owner_node_id",
                                              "model", "driver"};
  for (size_t i = 0; i < g->device_count; ++i) {
    const FisicsJsonValue *device = devices->as.array.items[i];
    const char *id = string_value(device, "device_id");
    const char *owner = string_value(device, "owner_node_id");
    const FisicsJsonValue *driver = fisics_json_object_get(device, "driver");
    if (!exact_fields(device, device_fields, 4u) || !canonical_name(id) ||
        !canonical_name(owner) ||
        !parse_ref(fisics_json_object_get(device, "model"), "model_id",
                   &g->devices[i].model)) {
      diag(d, "invalid hardware device at index %zu", i);
      goto fail;
    }
    if (driver && driver->type == FISICS_JSON_NULL) {
      g->devices[i].has_driver = false;
    } else if (parse_ref(driver, "driver_id", &g->devices[i].driver)) {
      g->devices[i].has_driver = true;
    } else {
      diag(d, "invalid hardware device driver at index %zu", i);
      goto fail;
    }
    g->devices[i].id = dup_text(id);
    g->devices[i].owner_node_id = dup_text(owner);
    if (!g->devices[i].id || !g->devices[i].owner_node_id)
      goto oom;
  }
  static const char *const connection_fields[] = {
      "connection_id", "device_id", "attachment_port_id", "target",
      "allocation_claims"};
  static const char *const target_fields[] = {"kind", "bus_id",
                                              "board_endpoint_id"};
  static const char *const claim_fields[] = {"space_id", "value_uint64",
                                             "value_id"};
  for (size_t i = 0; i < g->connection_count; ++i) {
    const FisicsJsonValue *c = connections->as.array.items[i];
    FisicsHardwareConnection *target = &g->connections[i];
    const FisicsJsonValue *target_value = fisics_json_object_get(c, "target");
    const FisicsJsonValue *claims =
        fisics_json_object_get(c, "allocation_claims");
    const char *id = string_value(c, "connection_id");
    const char *device = string_value(c, "device_id");
    const char *port = string_value(c, "attachment_port_id");
    const char *kind = string_value(target_value, "kind");
    if (!exact_fields(c, connection_fields, 5u) || !canonical_name(id) ||
        !canonical_name(device) || !canonical_name(port) ||
        !exact_fields(target_value, target_fields, 3u) ||
        (!kind ||
         (strcmp(kind, "bus") != 0 && strcmp(kind, "direct_endpoint") != 0)) ||
        !nullable_string(target_value, "bus_id", &target->bus_id) ||
        !nullable_string(target_value, "board_endpoint_id",
                         &target->board_endpoint_id) ||
        (strcmp(kind, "bus") == 0
             ? (!target->bus_id || target->board_endpoint_id)
             : (target->bus_id || !target->board_endpoint_id)) ||
        (target->bus_id && !canonical_name(target->bus_id)) ||
        (target->board_endpoint_id &&
         !canonical_name(target->board_endpoint_id)) ||
        !claims || claims->type != FISICS_JSON_ARRAY ||
        claims->as.array.count > HARDWARE_MAX_SET_ITEMS) {
      diag(d, "invalid hardware connection at index %zu", i);
      goto fail;
    }
    target->id = dup_text(id);
    target->device_id = dup_text(device);
    target->attachment_port_id = dup_text(port);
    target->target_kind = dup_text(kind);
    target->claim_count = claims->as.array.count;
    target->claims = calloc(target->claim_count, sizeof(*target->claims));
    if (!target->id || !target->device_id || !target->attachment_port_id ||
        !target->target_kind || (target->claim_count && !target->claims))
      goto oom;
    for (size_t j = 0; j < target->claim_count; ++j) {
      const FisicsJsonValue *claim = claims->as.array.items[j];
      const char *sid = string_value(claim, "space_id");
      if (!exact_fields(claim, claim_fields, 3u) || !canonical_name(sid) ||
          !nullable_string(claim, "value_uint64",
                           &target->claims[j].value_uint64) ||
          !nullable_string(claim, "value_id", &target->claims[j].value_id) ||
          (!!target->claims[j].value_uint64 == !!target->claims[j].value_id) ||
          (target->claims[j].value_id &&
           !canonical_name(target->claims[j].value_id))) {
        diag(d, "invalid allocation claim in connection %s", id);
        goto fail;
      }
      target->claims[j].space_id = dup_text(sid);
      if (!target->claims[j].space_id)
        goto oom;
    }
    qsort(target->claims, target->claim_count, sizeof(*target->claims),
          compare_claim_key);
    for (size_t j = 1; j < target->claim_count; ++j) {
      if (strcmp(target->claims[j - 1u].space_id, target->claims[j].space_id) ==
          0) {
        diag(d, "duplicate allocation space claim %s in connection %s",
             target->claims[j].space_id, id);
        goto fail;
      }
    }
  }
  static const char *const binding_fields[] = {
      "binding_id", "device_id", "physical_port_id", "physical_contract_id"};
  for (size_t i = 0; i < g->signal_binding_count; ++i) {
    const FisicsJsonValue *b = bindings->as.array.items[i];
    const char *id = string_value(b, "binding_id");
    const char *device = string_value(b, "device_id");
    const char *port = string_value(b, "physical_port_id");
    const char *contract = string_value(b, "physical_contract_id");
    if (!exact_fields(b, binding_fields, 4u) || !canonical_name(id) ||
        !canonical_name(device) || !canonical_name(port) ||
        !canonical_name(contract)) {
      diag(d, "invalid hardware signal binding at index %zu", i);
      goto fail;
    }
    g->signal_bindings[i].id = dup_text(id);
    g->signal_bindings[i].device_id = dup_text(device);
    g->signal_bindings[i].physical_port_id = dup_text(port);
    g->signal_bindings[i].physical_contract_id = dup_text(contract);
    if (!g->signal_bindings[i].id || !g->signal_bindings[i].device_id ||
        !g->signal_bindings[i].physical_port_id ||
        !g->signal_bindings[i].physical_contract_id)
      goto oom;
  }
  qsort(g->nodes, g->node_count, sizeof(*g->nodes), compare_node_key);
  qsort(g->buses, g->bus_count, sizeof(*g->buses), compare_bus_key);
  qsort(g->devices, g->device_count, sizeof(*g->devices), compare_device_key);
  qsort(g->connections, g->connection_count, sizeof(*g->connections),
        compare_connection_key);
  qsort(g->signal_bindings, g->signal_binding_count,
        sizeof(*g->signal_bindings), compare_binding_key);
  for (size_t i = 1; i < g->node_count; ++i)
    if (!strcmp(g->nodes[i - 1].id, g->nodes[i].id)) {
      diag(d, "duplicate hardware node_id: %s", g->nodes[i].id);
      goto fail;
    }
  for (size_t i = 1; i < g->bus_count; ++i)
    if (!strcmp(g->buses[i - 1].id, g->buses[i].id)) {
      diag(d, "duplicate hardware bus_id: %s", g->buses[i].id);
      goto fail;
    }
  for (size_t i = 1; i < g->device_count; ++i)
    if (!strcmp(g->devices[i - 1].id, g->devices[i].id)) {
      diag(d, "duplicate hardware device_id: %s", g->devices[i].id);
      goto fail;
    }
  for (size_t i = 1; i < g->connection_count; ++i)
    if (!strcmp(g->connections[i - 1].id, g->connections[i].id)) {
      diag(d, "duplicate hardware connection_id: %s", g->connections[i].id);
      goto fail;
    }
  for (size_t i = 1; i < g->signal_binding_count; ++i)
    if (!strcmp(g->signal_bindings[i - 1].id, g->signal_bindings[i].id)) {
      diag(d, "duplicate hardware binding_id: %s", g->signal_bindings[i].id);
      goto fail;
    }
  fisics_json_free(root);
  return true;
oom:
  diag(d, "out of memory loading hardware topology");
fail:
  fisics_json_free(root);
  return false;
}

static size_t find_node(const FisicsHardwareProjectGraph *g, const char *id) {
  for (size_t i = 0; i < g->node_count; ++i)
    if (!strcmp(g->nodes[i].id, id))
      return i;
  return SIZE_MAX;
}
static size_t find_bus_profile(const FisicsHardwareProjectGraph *g,
                               const FisicsHardwareRef *r) {
  for (size_t i = 0; i < g->bus_profile_count; ++i)
    if (same_key(g->bus_profiles[i].id, g->bus_profiles[i].revision, r->id,
                 r->revision))
      return i;
  return SIZE_MAX;
}
static size_t find_board_profile(const FisicsHardwareProjectGraph *g,
                                 const FisicsHardwareRef *r) {
  for (size_t i = 0; i < g->board_profile_count; ++i)
    if (same_key(g->board_profiles[i].id, g->board_profiles[i].revision, r->id,
                 r->revision))
      return i;
  return SIZE_MAX;
}
static size_t find_model(const FisicsHardwareProjectGraph *g,
                         const FisicsHardwareRef *r) {
  for (size_t i = 0; i < g->device_model_count; ++i)
    if (same_key(g->device_models[i].id, g->device_models[i].revision, r->id,
                 r->revision))
      return i;
  return SIZE_MAX;
}
static size_t find_driver(const FisicsHardwareProjectGraph *g,
                          const FisicsHardwareRef *r) {
  for (size_t i = 0; i < g->driver_contract_count; ++i)
    if (same_key(g->driver_contracts[i].id, g->driver_contracts[i].revision,
                 r->id, r->revision))
      return i;
  return SIZE_MAX;
}
static size_t find_bus(const FisicsHardwareProjectGraph *g, const char *id) {
  for (size_t i = 0; i < g->bus_count; ++i)
    if (!strcmp(g->buses[i].id, id))
      return i;
  return SIZE_MAX;
}
static size_t find_device(const FisicsHardwareProjectGraph *g, const char *id) {
  for (size_t i = 0; i < g->device_count; ++i)
    if (!strcmp(g->devices[i].id, id))
      return i;
  return SIZE_MAX;
}

static bool add_unbound(FisicsHardwareProjectGraph *g, const char *device,
                        const char *kind, const char *port,
                        FisicsSemanticProjectDiagnostic *d) {
  FisicsHardwareUnboundPort item = {dup_text(device), dup_text(kind),
                                    dup_text(port)};
  if (!item.device_id || !item.port_kind || !item.port_id) {
    free(item.device_id);
    free(item.port_kind);
    free(item.port_id);
    diag(d, "out of memory retaining unbound hardware port");
    return false;
  }
  APPEND_ITEM(g->unbound_ports, g->unbound_port_count, item,
              FisicsHardwareUnboundPort, "unbound port");
  return true;
}

static bool resolve_graph(const FisicsHardwareProjectManifest *manifest,
                          const PhysicalInput *physical,
                          FisicsHardwareProjectGraph *g,
                          FisicsSemanticProjectDiagnostic *d) {
  for (size_t i = 0; i < g->node_count; ++i) {
    size_t profile = find_board_profile(g, &g->nodes[i].board_profile);
    if (profile == SIZE_MAX) {
      diag(d, "unresolved board profile %s revision %s for node %s",
           g->nodes[i].board_profile.id, g->nodes[i].board_profile.revision,
           g->nodes[i].id);
      return false;
    }
    g->nodes[i].board_profile_index = profile;
  }
  for (size_t i = 0; i < g->bus_count; ++i) {
    FisicsHardwareBus *bus = &g->buses[i];
    bus->owner_node_index = find_node(g, bus->owner_node_id);
    bus->bus_profile_index = find_bus_profile(g, &bus->bus_profile);
    if (bus->owner_node_index == SIZE_MAX ||
        bus->bus_profile_index == SIZE_MAX) {
      diag(d, "unresolved owner or profile for bus %s", bus->id);
      return false;
    }
    FisicsHardwareBoardProfile *board =
        &g->board_profiles[g->nodes[bus->owner_node_index].board_profile_index];
    size_t endpoint = SIZE_MAX;
    for (size_t j = 0; j < board->endpoint_count; ++j)
      if (!strcmp(board->endpoints[j].id, bus->controller_endpoint_id)) {
        endpoint = j;
        break;
      }
    if (endpoint == SIZE_MAX ||
        !string_list_contains(
            &board->endpoints[endpoint].capabilities,
            g->bus_profiles[bus->bus_profile_index].controller_capability)) {
      diag(d, "bus %s controller endpoint lacks required capability", bus->id);
      return false;
    }
    bus->controller_endpoint_index = endpoint;
    for (size_t j = 0; j < i; ++j)
      if (g->buses[j].owner_node_index == bus->owner_node_index &&
          g->buses[j].controller_endpoint_index == endpoint &&
          (!strcmp(board->endpoints[endpoint].allocation_policy,
                   "exclusive"))) {
        diag(d, "exclusive board endpoint conflict between buses %s and %s",
             g->buses[j].id, bus->id);
        return false;
      }
  }
  for (size_t i = 0; i < g->device_count; ++i) {
    FisicsHardwareDevice *device = &g->devices[i];
    device->owner_node_index = find_node(g, device->owner_node_id);
    device->model_index = find_model(g, &device->model);
    if (device->owner_node_index == SIZE_MAX ||
        device->model_index == SIZE_MAX) {
      diag(d, "unresolved owner or model for device %s", device->id);
      return false;
    }
    FisicsHardwareDeviceModel *model = &g->device_models[device->model_index];
    if (!strcmp(model->driver_policy, "required") && !device->has_driver) {
      diag(d, "device %s requires a driver contract", device->id);
      return false;
    }
    if (!strcmp(model->driver_policy, "none") && device->has_driver) {
      diag(d, "device %s forbids a driver contract", device->id);
      return false;
    }
    if (device->has_driver) {
      device->driver_index = find_driver(g, &device->driver);
      if (device->driver_index == SIZE_MAX) {
        diag(d, "unresolved driver contract for device %s", device->id);
        return false;
      }
      FisicsHardwareDriverContract *driver =
          &g->driver_contracts[device->driver_index];
      bool supported = false;
      for (size_t j = 0; j < driver->supported_model_count; ++j)
        if (same_key(driver->supported_models[j].id,
                     driver->supported_models[j].revision, device->model.id,
                     device->model.revision)) {
          supported = true;
          break;
        }
      if (!supported) {
        diag(d,
             "driver %s revision %s does not support device model %s revision "
             "%s",
             driver->id, driver->revision, device->model.id,
             device->model.revision);
        return false;
      }
    }
  }
  for (size_t i = 0; i < g->connection_count; ++i) {
    FisicsHardwareConnection *c = &g->connections[i];
    c->device_index = find_device(g, c->device_id);
    if (c->device_index == SIZE_MAX) {
      diag(d, "connection %s references missing device %s", c->id,
           c->device_id);
      return false;
    }
    FisicsHardwareDevice *device = &g->devices[c->device_index];
    FisicsHardwareDeviceModel *model = &g->device_models[device->model_index];
    size_t port = SIZE_MAX;
    for (size_t j = 0; j < model->attachment_port_count; ++j)
      if (!strcmp(model->attachment_ports[j].id, c->attachment_port_id)) {
        port = j;
        break;
      }
    if (port == SIZE_MAX) {
      diag(d, "connection %s references missing attachment port %s", c->id,
           c->attachment_port_id);
      return false;
    }
    c->attachment_port_index = port;
    for (size_t j = 0; j < i; ++j)
      if (g->connections[j].device_index == c->device_index &&
          g->connections[j].attachment_port_index == port) {
        diag(d, "duplicate attachment connection for device %s port %s",
             device->id, c->attachment_port_id);
        return false;
      }
    FisicsHardwareAttachmentPort *ap = &model->attachment_ports[port];
    if (!strcmp(c->target_kind, "bus")) {
      if (strcmp(ap->connection_kind, "bus_participant")) {
        diag(d, "connection %s target kind mismatches attachment port", c->id);
        return false;
      }
      c->target_index = find_bus(g, c->bus_id);
      if (c->target_index == SIZE_MAX) {
        diag(d, "connection %s references missing bus %s", c->id, c->bus_id);
        return false;
      }
      FisicsHardwareBus *bus = &g->buses[c->target_index];
      FisicsHardwareBusProfile *bp = &g->bus_profiles[bus->bus_profile_index];
      if (strcmp(ap->required_transport_kind, bp->transport_kind) ||
          strcmp(ap->required_capability, bp->participant_capability)) {
        diag(d, "connection %s transport or participant capability mismatch",
             c->id);
        return false;
      }
      if (!strcmp(bus->scope, "node_local") &&
          bus->owner_node_index != device->owner_node_index) {
        diag(d, "connection %s crosses node-local bus ownership", c->id);
        return false;
      }
      for (size_t j = 0; j < c->claim_count; ++j) {
        size_t si = SIZE_MAX;
        for (size_t k = 0; k < bp->space_count; ++k)
          if (!strcmp(bp->spaces[k].space_id, c->claims[j].space_id)) {
            si = k;
            break;
          }
        if (si == SIZE_MAX) {
          diag(d, "connection %s claims unknown allocation space %s", c->id,
               c->claims[j].space_id);
          return false;
        }
        c->claims[j].space_index = si;
        FisicsHardwareAllocationSpace *s = &bp->spaces[si];
        if (!strcmp(s->value_kind, "uint64_decimal")) {
          uint64_t v, min, max;
          if (!c->claims[j].value_uint64 ||
              !canonical_u64(c->claims[j].value_uint64, &v) ||
              !canonical_u64(s->minimum, &min) ||
              !canonical_u64(s->maximum, &max) || v < min || v > max) {
            diag(d, "connection %s allocation %s is out of range or wrong kind",
                 c->id, s->space_id);
            return false;
          }
        } else if (!c->claims[j].value_id) {
          diag(d, "connection %s allocation %s has wrong value kind", c->id,
               s->space_id);
          return false;
        }
        if (!strcmp(s->collision_policy, "exclusive")) {
          for (size_t p = 0; p < i; ++p) {
            FisicsHardwareConnection *prior = &g->connections[p];
            if (strcmp(prior->target_kind, "bus") ||
                prior->target_index != c->target_index)
              continue;
            for (size_t q = 0; q < prior->claim_count; ++q) {
              FisicsHardwareAllocationClaim *pc = &prior->claims[q];
              if (strcmp(pc->space_id, c->claims[j].space_id))
                continue;
              bool equal =
                  (pc->value_uint64 && c->claims[j].value_uint64 &&
                   !strcmp(pc->value_uint64, c->claims[j].value_uint64)) ||
                  (pc->value_id && c->claims[j].value_id &&
                   !strcmp(pc->value_id, c->claims[j].value_id));
              if (equal) {
                diag(d,
                     "exclusive allocation conflict between connections %s and "
                     "%s in %s",
                     prior->id, c->id, s->space_id);
                return false;
              }
            }
          }
        }
      }
      for (size_t k = 0; k < bp->space_count; ++k)
        if (bp->spaces[k].claim_required) {
          bool found = false;
          for (size_t j = 0; j < c->claim_count; ++j)
            if (c->claims[j].space_index == k) {
              found = true;
              break;
            }
          if (!found) {
            diag(d, "connection %s is missing required allocation claim %s",
                 c->id, bp->spaces[k].space_id);
            return false;
          }
        }
    } else {
      if (strcmp(ap->connection_kind, "direct_endpoint") || c->claim_count) {
        diag(d, "connection %s invalid direct endpoint shape", c->id);
        return false;
      }
      FisicsHardwareBoardProfile *board =
          &g->board_profiles[g->nodes[device->owner_node_index]
                                 .board_profile_index];
      size_t endpoint = SIZE_MAX;
      for (size_t j = 0; j < board->endpoint_count; ++j)
        if (!strcmp(board->endpoints[j].id, c->board_endpoint_id)) {
          endpoint = j;
          break;
        }
      if (endpoint == SIZE_MAX ||
          !string_list_contains(&board->endpoints[endpoint].capabilities,
                                ap->required_capability)) {
        diag(d, "connection %s direct endpoint lacks required capability",
             c->id);
        return false;
      }
      c->target_index = endpoint;
      if (!strcmp(board->endpoints[endpoint].allocation_policy, "exclusive")) {
        for (size_t j = 0; j < i; ++j) {
          FisicsHardwareConnection *prior = &g->connections[j];
          if (strcmp(prior->target_kind, "direct_endpoint"))
            continue;
          FisicsHardwareDevice *pd = &g->devices[prior->device_index];
          if (pd->owner_node_index == device->owner_node_index &&
              prior->target_index == endpoint) {
            diag(d,
                 "exclusive direct endpoint conflict between connections %s "
                 "and %s",
                 prior->id, c->id);
            return false;
          }
        }
        for (size_t j = 0; j < g->bus_count; ++j)
          if (g->buses[j].owner_node_index == device->owner_node_index &&
              g->buses[j].controller_endpoint_index == endpoint) {
            diag(d,
                 "exclusive endpoint conflict between bus %s and connection %s",
                 g->buses[j].id, c->id);
            return false;
          }
      }
    }
  }
  for (size_t i = 0; i < g->device_count; ++i) {
    FisicsHardwareDevice *device = &g->devices[i];
    FisicsHardwareDeviceModel *model = &g->device_models[device->model_index];
    for (size_t p = 0; p < model->attachment_port_count; ++p)
      if (model->attachment_ports[p].required) {
        bool found = false;
        for (size_t j = 0; j < g->connection_count; ++j)
          if (g->connections[j].device_index == i &&
              g->connections[j].attachment_port_index == p) {
            found = true;
            break;
          }
        if (!found) {
          if (manifest->port_policy == FISICS_HARDWARE_PORTS_REQUIRE_ALL) {
            diag(d, "device %s has unbound required attachment port %s",
                 device->id, model->attachment_ports[p].id);
            return false;
          }
          if (!add_unbound(g, device->id, "attachment",
                           model->attachment_ports[p].id, d))
            return false;
        }
      }
  }
  for (size_t i = 0; i < g->device_count; ++i)
    if (g->devices[i].has_driver) {
      FisicsHardwareDriverContract *driver =
          &g->driver_contracts[g->devices[i].driver_index];
      for (size_t t = 0; t < driver->required_transports.count; ++t) {
        bool found = false;
        for (size_t c = 0; c < g->connection_count; ++c)
          if (g->connections[c].device_index == i &&
              !strcmp(g->connections[c].target_kind, "bus")) {
            FisicsHardwareBus *bus = &g->buses[g->connections[c].target_index];
            if (!strcmp(g->bus_profiles[bus->bus_profile_index].transport_kind,
                        driver->required_transports.items[t])) {
              found = true;
              break;
            }
          }
        if (!found) {
          diag(d, "device %s driver requires unresolved transport %s",
               g->devices[i].id, driver->required_transports.items[t]);
          return false;
        }
      }
    }
  for (size_t i = 0; i < g->signal_binding_count; ++i) {
    FisicsHardwareSignalBinding *b = &g->signal_bindings[i];
    b->device_index = find_device(g, b->device_id);
    if (b->device_index == SIZE_MAX) {
      diag(d, "signal binding %s references missing device %s", b->id,
           b->device_id);
      return false;
    }
    FisicsHardwareDevice *device = &g->devices[b->device_index];
    FisicsHardwareDeviceModel *model = &g->device_models[device->model_index];
    size_t port = SIZE_MAX;
    for (size_t p = 0; p < model->physical_port_count; ++p)
      if (!strcmp(model->physical_ports[p].id, b->physical_port_id)) {
        port = p;
        break;
      }
    if (port == SIZE_MAX) {
      diag(d, "signal binding %s references missing physical port %s", b->id,
           b->physical_port_id);
      return false;
    }
    b->physical_port_index = port;
    for (size_t j = 0; j < i; ++j)
      if (g->signal_bindings[j].device_index == b->device_index &&
          g->signal_bindings[j].physical_port_index == port) {
        diag(d, "duplicate physical port binding for device %s port %s",
             device->id, b->physical_port_id);
        return false;
      }
    size_t contract = SIZE_MAX;
    for (size_t c = 0; c < physical->contract_count; ++c)
      if (!strcmp(physical->contracts[c].id, b->physical_contract_id)) {
        contract = c;
        break;
      }
    if (contract == SIZE_MAX) {
      diag(d, "signal binding %s references missing physical contract %s",
           b->id, b->physical_contract_id);
      return false;
    }
    b->physical_contract_index = contract;
    const PhysicalContractInput *pc = &physical->contracts[contract];
    if (manifest->evidence_policy == FISICS_HARDWARE_EVIDENCE_REQUIRE_SOURCE &&
        strcmp(pc->state, "source_compatible")) {
      diag(d, "signal binding %s requires source_compatible physical evidence",
           b->id);
      return false;
    }
    FisicsHardwarePhysicalRequirement *req =
        &model->physical_ports[port].requirement;
    if (!strcmp(req->mode, "exact_quantity")) {
      if (strcmp(req->quantity_kind, pc->quantity_kind)) {
        diag(d, "signal binding %s quantity mismatch", b->id);
        return false;
      }
    } else if (memcmp(req->dimension, pc->dimension, 8u)) {
      diag(d, "signal binding %s dimension mismatch", b->id);
      return false;
    }
    if ((req->value_form && strcmp(req->value_form, pc->value_form)) ||
        (req->reference_kind &&
         strcmp(req->reference_kind, pc->reference_kind))) {
      diag(d, "signal binding %s value form or reference mismatch", b->id);
      return false;
    }
    b->physical_evidence_state = dup_text(pc->state);
    b->compatibility_mode = dup_text(req->mode);
    if (!b->physical_evidence_state || !b->compatibility_mode) {
      diag(d, "out of memory retaining signal compatibility");
      return false;
    }
  }
  for (size_t i = 0; i < g->device_count; ++i) {
    FisicsHardwareDevice *device = &g->devices[i];
    FisicsHardwareDeviceModel *model = &g->device_models[device->model_index];
    for (size_t p = 0; p < model->physical_port_count; ++p)
      if (model->physical_ports[p].required) {
        bool found = false;
        for (size_t j = 0; j < g->signal_binding_count; ++j)
          if (g->signal_bindings[j].device_index == i &&
              g->signal_bindings[j].physical_port_index == p) {
            found = true;
            break;
          }
        if (!found) {
          if (manifest->port_policy == FISICS_HARDWARE_PORTS_REQUIRE_ALL) {
            diag(d, "device %s has unbound required physical port %s",
                 device->id, model->physical_ports[p].id);
            return false;
          }
          if (!add_unbound(g, device->id, "physical",
                           model->physical_ports[p].id, d))
            return false;
        }
      }
  }
  return true;
}

bool fisics_hardware_project_graph_build(
    const FisicsHardwareProjectManifest *manifest,
    FisicsHardwareProjectGraph *out, FisicsSemanticProjectDiagnostic *d) {
  if (d)
    d->message[0] = '\0';
  if (!manifest || !out) {
    diag(d, "invalid hardware project graph arguments");
    return false;
  }
  memset(out, 0, sizeof(*out));
  if (getenv("FISICS_TEST_FAIL_HARDWARE_PROJECT_GRAPH_ALLOC")) {
    diag(d, "injected hardware project graph allocation failure");
    return false;
  }
  out->project_id = dup_text(manifest->project_id);
  out->manifest_path = dup_text(manifest->manifest_path);
  out->physical_graph_path = dup_text(manifest->physical_graph_path);
  if (!out->project_id || !out->manifest_path || !out->physical_graph_path) {
    diag(d, "out of memory initializing hardware project graph");
    goto fail;
  }
  uint64_t aggregate = 0;
  PhysicalInput physical = {0};
  if (!parse_physical_graph(manifest->physical_graph_path, manifest->project_id,
                            &aggregate, &physical, &out->physical_graph_hash,
                            &out->physical_graph_length, d))
    goto fail;
  out->unit_registry_fingerprint = dup_text(physical.registry_fingerprint);
  if (!out->unit_registry_fingerprint) {
    diag(d, "out of memory retaining physical graph provenance");
    goto fail_physical;
  }
  out->physical_source_paths = physical.source_paths;
  memset(&physical.source_paths, 0, sizeof(physical.source_paths));
  for (size_t i = 0; i < manifest->catalog_path_count; ++i)
    if (!parse_catalog(manifest->catalog_paths[i], &aggregate, out, d))
      goto fail_physical;
  if (out->bus_profile_count > HARDWARE_MAX_DEFINITIONS ||
      out->board_profile_count > HARDWARE_MAX_DEFINITIONS ||
      out->device_model_count > HARDWARE_MAX_DEFINITIONS ||
      out->driver_contract_count > HARDWARE_MAX_DEFINITIONS) {
    diag(d, "merged hardware catalog definition count exceeds frozen limits");
    goto fail_physical;
  }
  qsort(out->bus_profiles, out->bus_profile_count, sizeof(*out->bus_profiles),
        compare_bus_profile_key);
  qsort(out->board_profiles, out->board_profile_count,
        sizeof(*out->board_profiles), compare_board_profile_key);
  qsort(out->device_models, out->device_model_count,
        sizeof(*out->device_models), compare_model_key);
  qsort(out->driver_contracts, out->driver_contract_count,
        sizeof(*out->driver_contracts), compare_driver_key);
  if (!parse_topology(manifest->topology_path, manifest->project_id, &aggregate,
                      out, d) ||
      !resolve_graph(manifest, &physical, out, d))
    goto fail_physical;
  qsort(out->receipts, out->receipt_count, sizeof(*out->receipts),
        compare_receipt_key);
  qsort(out->unbound_ports, out->unbound_port_count,
        sizeof(*out->unbound_ports), compare_unbound_key);
  free_physical_input(&physical);
  return true;
fail_physical:
  free_physical_input(&physical);
fail:
  fisics_hardware_project_graph_free(out);
  return false;
}
