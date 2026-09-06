// SPDX-License-Identifier: Apache-2.0

#include "Compiler/hardware_project_graph_json.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void diag(FisicsSemanticProjectDiagnostic *d, const char *fmt, ...) {
  if (!d || d->message[0])
    return;
  va_list args;
  va_start(args, fmt);
  vsnprintf(d->message, sizeof(d->message), fmt, args);
  va_end(args);
}

static bool js(FILE *out, const char *value) {
  if (fputc('"', out) == EOF)
    return false;
  for (const unsigned char *p = (const unsigned char *)(value ? value : ""); *p;
       ++p) {
    switch (*p) {
    case '"':
      if (fputs("\\\"", out) == EOF)
        return false;
      break;
    case '\\':
      if (fputs("\\\\", out) == EOF)
        return false;
      break;
    case '\n':
      if (fputs("\\n", out) == EOF)
        return false;
      break;
    case '\r':
      if (fputs("\\r", out) == EOF)
        return false;
      break;
    case '\t':
      if (fputs("\\t", out) == EOF)
        return false;
      break;
    default:
      if (*p < 0x20u) {
        if (fprintf(out, "\\u%04x", (unsigned)*p) < 0)
          return false;
      } else if (fputc(*p, out) == EOF)
        return false;
    }
  }
  return fputc('"', out) != EOF;
}

static bool nullable(FILE *out, const char *value) {
  return value ? js(out, value) : fputs("null", out) != EOF;
}

static bool string_list(FILE *out, const FisicsHardwareStringList *list) {
  if (fputc('[', out) == EOF)
    return false;
  for (size_t i = 0; i < list->count; ++i) {
    if ((i && fputs(", ", out) == EOF) || !js(out, list->items[i]))
      return false;
  }
  return fputc(']', out) != EOF;
}

static bool ref_json(FILE *out, const char *id_key,
                     const FisicsHardwareRef *ref) {
  return fputs("{\"", out) != EOF && fputs(id_key, out) != EOF &&
         fputs("\":", out) != EOF && js(out, ref->id) &&
         fputs(",\"revision\":", out) != EOF && js(out, ref->revision) &&
         fputc('}', out) != EOF;
}

static bool write_receipt(FILE *out, const FisicsHardwareReceipt *r,
                          bool final) {
  return fputs("    {\n      \"path\": ", out) != EOF && js(out, r->path) &&
         fprintf(out,
                 ",\n      \"hash_fnv1a64\": \"%016" PRIx64
                 "\",\n      \"length_bytes\": %zu,\n      \"catalog_id\": ",
                 r->hash, r->length) >= 0 &&
         nullable(out, r->catalog_id) &&
         fputs(",\n      \"authority_kind\": ", out) != EOF &&
         nullable(out, r->authority_kind) &&
         fputs(",\n      \"authority_identity\": ", out) != EOF &&
         nullable(out, r->authority_identity) &&
         fprintf(out, "\n    }%s\n", final ? "" : ",") >= 0;
}

static bool write_bus_profile(FILE *out, const FisicsHardwareBusProfile *p,
                              bool final) {
  if (fputs("    {\n      \"profile_id\": ", out) == EOF || !js(out, p->id) ||
      fputs(",\n      \"revision\": ", out) == EOF || !js(out, p->revision) ||
      fputs(",\n      \"transport_kind\": ", out) == EOF ||
      !js(out, p->transport_kind) ||
      fputs(",\n      \"controller_capability\": ", out) == EOF ||
      !js(out, p->controller_capability) ||
      fputs(",\n      \"participant_capability\": ", out) == EOF ||
      !js(out, p->participant_capability) ||
      fputs(",\n      \"allocation_spaces\": [\n", out) == EOF)
    return false;
  for (size_t i = 0; i < p->space_count; ++i) {
    const FisicsHardwareAllocationSpace *s = &p->spaces[i];
    if (fputs("        {\"space_id\":", out) == EOF || !js(out, s->space_id) ||
        fputs(",\"value_kind\":", out) == EOF || !js(out, s->value_kind) ||
        fputs(",\"minimum_uint64\":", out) == EOF ||
        !nullable(out, s->minimum) ||
        fputs(",\"maximum_uint64\":", out) == EOF ||
        !nullable(out, s->maximum) ||
        fprintf(out, ",\"claim_required\":%s,\"collision_policy\":",
                s->claim_required ? "true" : "false") < 0 ||
        !js(out, s->collision_policy) ||
        fprintf(out, "}%s\n", i + 1u == p->space_count ? "" : ",") < 0)
      return false;
  }
  return fputs("      ],\n      \"origin_catalogs\": ", out) != EOF &&
         string_list(out, &p->origins) &&
         fprintf(out, "\n    }%s\n", final ? "" : ",") >= 0;
}

static bool write_board_profile(FILE *out, const FisicsHardwareBoardProfile *p,
                                bool final) {
  if (fputs("    {\n      \"profile_id\": ", out) == EOF || !js(out, p->id) ||
      fputs(",\n      \"revision\": ", out) == EOF || !js(out, p->revision) ||
      fputs(",\n      \"capability_ids\": ", out) == EOF ||
      !string_list(out, &p->capabilities) ||
      fputs(",\n      \"endpoints\": [\n", out) == EOF)
    return false;
  for (size_t i = 0; i < p->endpoint_count; ++i) {
    const FisicsHardwareEndpoint *e = &p->endpoints[i];
    if (fputs("        {\"endpoint_id\":", out) == EOF || !js(out, e->id) ||
        fputs(",\"capability_ids\":", out) == EOF ||
        !string_list(out, &e->capabilities) ||
        fputs(",\"allocation_policy\":", out) == EOF ||
        !js(out, e->allocation_policy) ||
        fprintf(out, "}%s\n", i + 1u == p->endpoint_count ? "" : ",") < 0)
      return false;
  }
  return fputs("      ],\n      \"origin_catalogs\": ", out) != EOF &&
         string_list(out, &p->origins) &&
         fprintf(out, "\n    }%s\n", final ? "" : ",") >= 0;
}

static bool write_requirement(FILE *out,
                              const FisicsHardwarePhysicalRequirement *r) {
  if (fputs("{\"mode\":", out) == EOF || !js(out, r->mode) ||
      fputs(",\"quantity_kind\":", out) == EOF ||
      !nullable(out, r->quantity_kind) ||
      fputs(",\"dimension_system\":", out) == EOF ||
      !nullable(out, r->dimension_system) ||
      fputs(",\"dimension_vector\":[", out) == EOF)
    return false;
  if (r->has_dimension) {
    for (size_t i = 0; i < 8u; ++i) {
      if (fprintf(out, "%s%d", i ? "," : "", (int)r->dimension[i]) < 0)
        return false;
    }
  }
  return fputs("],\"value_form\":", out) != EOF &&
         nullable(out, r->value_form) &&
         fputs(",\"reference_kind\":", out) != EOF &&
         nullable(out, r->reference_kind) && fputc('}', out) != EOF;
}

static bool write_device_model(FILE *out, const FisicsHardwareDeviceModel *p,
                               bool final) {
  if (fputs("    {\n      \"model_id\": ", out) == EOF || !js(out, p->id) ||
      fputs(",\n      \"revision\": ", out) == EOF || !js(out, p->revision) ||
      fputs(",\n      \"capability_ids\": ", out) == EOF ||
      !string_list(out, &p->capabilities) ||
      fputs(",\n      \"driver_policy\": ", out) == EOF ||
      !js(out, p->driver_policy) ||
      fputs(",\n      \"attachment_ports\": [\n", out) == EOF)
    return false;
  for (size_t i = 0; i < p->attachment_port_count; ++i) {
    const FisicsHardwareAttachmentPort *a = &p->attachment_ports[i];
    if (fputs("        {\"port_id\":", out) == EOF || !js(out, a->id) ||
        fputs(",\"connection_kind\":", out) == EOF ||
        !js(out, a->connection_kind) ||
        fputs(",\"required_capability\":", out) == EOF ||
        !js(out, a->required_capability) ||
        fputs(",\"required_transport_kind\":", out) == EOF ||
        !nullable(out, a->required_transport_kind) ||
        fprintf(out, ",\"required\":%s}%s\n", a->required ? "true" : "false",
                i + 1u == p->attachment_port_count ? "" : ",") < 0)
      return false;
  }
  if (fputs("      ],\n      \"physical_ports\": [\n", out) == EOF)
    return false;
  for (size_t i = 0; i < p->physical_port_count; ++i) {
    const FisicsHardwarePhysicalPort *port = &p->physical_ports[i];
    if (fputs("        {\"port_id\":", out) == EOF || !js(out, port->id) ||
        fputs(",\"direction\":", out) == EOF || !js(out, port->direction) ||
        fprintf(out, ",\"required\":%s,\"requirement\":",
                port->required ? "true" : "false") < 0 ||
        !write_requirement(out, &port->requirement) ||
        fprintf(out, "}%s\n", i + 1u == p->physical_port_count ? "" : ",") < 0)
      return false;
  }
  return fputs("      ],\n      \"origin_catalogs\": ", out) != EOF &&
         string_list(out, &p->origins) &&
         fprintf(out, "\n    }%s\n", final ? "" : ",") >= 0;
}

static bool write_driver(FILE *out, const FisicsHardwareDriverContract *p,
                         bool final) {
  if (fputs("    {\n      \"driver_id\": ", out) == EOF || !js(out, p->id) ||
      fputs(",\n      \"revision\": ", out) == EOF || !js(out, p->revision) ||
      fputs(",\n      \"supported_models\": [", out) == EOF)
    return false;
  for (size_t i = 0; i < p->supported_model_count; ++i) {
    if ((i && fputc(',', out) == EOF) ||
        !ref_json(out, "model_id", &p->supported_models[i]))
      return false;
  }
  return fputs("],\n      \"required_transport_kinds\": ", out) != EOF &&
         string_list(out, &p->required_transports) &&
         fputs(",\n      \"origin_catalogs\": ", out) != EOF &&
         string_list(out, &p->origins) &&
         fprintf(out, "\n    }%s\n", final ? "" : ",") >= 0;
}

static bool write_document(FILE *out, const FisicsHardwareProjectGraph *g) {
  if (fputs("{\n  \"schema\": \"fisiCs.hardware_project_graph\",\n"
            "  \"schema_version\": 1,\n"
            "  \"producer\": {\"name\":\"fisiCs\",\"version\":\"" FISICS_VERSION
            "\"},\n"
            "  \"project\": {\"id\":",
            out) == EOF ||
      !js(out, g->project_id) || fputs(",\"manifest_path\":", out) == EOF ||
      !js(out, g->manifest_path) ||
      fputs("},\n  \"complete\": true,\n  \"assessment\": {\n"
            "    \"catalog_integrity\": \"validated\",\n"
            "    \"declared_topology\": ",
            out) == EOF ||
      !js(out, g->unbound_port_count ? "incomplete" : "compatible") ||
      fputs(",\n    \"hardware_presence\": \"not_assessed\",\n"
            "    \"wiring\": \"not_assessed\",\n"
            "    \"driver_implementation\": \"not_assessed\",\n"
            "    \"calibration\": \"not_assessed\",\n"
            "    \"runtime_observation\": \"not_assessed\",\n"
            "    \"simulation\": \"not_assessed\",\n"
            "    \"deployment\": \"not_assessed\",\n"
            "    \"physical_acceptance\": \"not_assessed\"\n"
            "  },\n  \"coverage\": {\"unbound_required_ports\":",
            out) == EOF ||
      fprintf(out, "%zu},\n  \"physical_graph_receipt\": {\"path\":",
              g->unbound_port_count) < 0 ||
      !js(out, g->physical_graph_path) ||
      fprintf(out,
              ",\"hash_fnv1a64\":\"%016" PRIx64
              "\",\"length_bytes\":%zu,\"project_id\":",
              g->physical_graph_hash, g->physical_graph_length) < 0 ||
      !js(out, g->project_id) ||
      fputs(",\"unit_registry_fingerprint_fnv1a64\":", out) == EOF ||
      !js(out, g->unit_registry_fingerprint) ||
      fputs("},\n  \"input_receipts\": [\n", out) == EOF)
    return false;
  for (size_t i = 0; i < g->receipt_count; ++i) {
    if (!write_receipt(out, &g->receipts[i], i + 1u == g->receipt_count))
      return false;
  }
  if (fputs("  ],\n  \"catalogs\": [\n", out) == EOF)
    return false;
  bool first = true;
  for (size_t i = 0; i < g->receipt_count; ++i)
    if (g->receipts[i].catalog_id) {
      if (!first && fputs(",\n", out) == EOF)
        return false;
      if (fputs("    {\"catalog_id\":", out) == EOF ||
          !js(out, g->receipts[i].catalog_id) ||
          fputs(",\"authority\":{\"kind\":", out) == EOF ||
          !js(out, g->receipts[i].authority_kind) ||
          fputs(",\"identity\":", out) == EOF ||
          !js(out, g->receipts[i].authority_identity) ||
          fprintf(out, "},\"receipt_index\":%zu}", i) < 0)
        return false;
      first = false;
    }
  if (fputs("\n  ],\n  \"bus_profiles\": [\n", out) == EOF)
    return false;
  for (size_t i = 0; i < g->bus_profile_count; ++i)
    if (!write_bus_profile(out, &g->bus_profiles[i],
                           i + 1u == g->bus_profile_count))
      return false;
  if (fputs("  ],\n  \"board_profiles\": [\n", out) == EOF)
    return false;
  for (size_t i = 0; i < g->board_profile_count; ++i)
    if (!write_board_profile(out, &g->board_profiles[i],
                             i + 1u == g->board_profile_count))
      return false;
  if (fputs("  ],\n  \"device_models\": [\n", out) == EOF)
    return false;
  for (size_t i = 0; i < g->device_model_count; ++i)
    if (!write_device_model(out, &g->device_models[i],
                            i + 1u == g->device_model_count))
      return false;
  if (fputs("  ],\n  \"driver_contracts\": [\n", out) == EOF)
    return false;
  for (size_t i = 0; i < g->driver_contract_count; ++i)
    if (!write_driver(out, &g->driver_contracts[i],
                      i + 1u == g->driver_contract_count))
      return false;
  if (fputs("  ],\n  \"nodes\": [\n", out) == EOF)
    return false;
  for (size_t i = 0; i < g->node_count; ++i) {
    const FisicsHardwareNode *n = &g->nodes[i];
    if (fputs("    {\"node_id\":", out) == EOF || !js(out, n->id) ||
        fputs(",\"board_profile\":", out) == EOF ||
        !ref_json(out, "profile_id", &n->board_profile) ||
        fprintf(out, ",\"board_profile_index\":%zu}%s\n",
                n->board_profile_index, i + 1u == g->node_count ? "" : ",") < 0)
      return false;
  }
  if (fputs("  ],\n  \"buses\": [\n", out) == EOF)
    return false;
  for (size_t i = 0; i < g->bus_count; ++i) {
    const FisicsHardwareBus *b = &g->buses[i];
    if (fputs("    {\"bus_id\":", out) == EOF || !js(out, b->id) ||
        fputs(",\"owner_node_id\":", out) == EOF ||
        !js(out, b->owner_node_id) ||
        fprintf(out, ",\"owner_node_index\":%zu,\"scope\":",
                b->owner_node_index) < 0 ||
        !js(out, b->scope) || fputs(",\"bus_profile\":", out) == EOF ||
        !ref_json(out, "profile_id", &b->bus_profile) ||
        fprintf(out, ",\"bus_profile_index\":%zu,\"controller_endpoint_id\":",
                b->bus_profile_index) < 0 ||
        !js(out, b->controller_endpoint_id) ||
        fprintf(out, ",\"controller_endpoint_index\":%zu}%s\n",
                b->controller_endpoint_index,
                i + 1u == g->bus_count ? "" : ",") < 0)
      return false;
  }
  if (fputs("  ],\n  \"devices\": [\n", out) == EOF)
    return false;
  for (size_t i = 0; i < g->device_count; ++i) {
    const FisicsHardwareDevice *d = &g->devices[i];
    if (fputs("    {\"device_id\":", out) == EOF || !js(out, d->id) ||
        fputs(",\"owner_node_id\":", out) == EOF ||
        !js(out, d->owner_node_id) ||
        fprintf(out, ",\"owner_node_index\":%zu,\"model\":",
                d->owner_node_index) < 0 ||
        !ref_json(out, "model_id", &d->model) ||
        fprintf(out, ",\"model_index\":%zu,\"driver\":", d->model_index) < 0)
      return false;
    if (d->has_driver) {
      if (!ref_json(out, "driver_id", &d->driver))
        return false;
    } else if (fputs("null", out) == EOF)
      return false;
    if (fputs(",\"driver_index\":", out) == EOF)
      return false;
    if (d->has_driver) {
      if (fprintf(out, "%zu", d->driver_index) < 0)
        return false;
    } else if (fputs("null", out) == EOF)
      return false;
    if (fprintf(out, "}%s\n", i + 1u == g->device_count ? "" : ",") < 0)
      return false;
  }
  if (fputs("  ],\n  \"connections\": [\n", out) == EOF)
    return false;
  for (size_t i = 0; i < g->connection_count; ++i) {
    const FisicsHardwareConnection *c = &g->connections[i];
    if (fputs("    {\"connection_id\":", out) == EOF || !js(out, c->id) ||
        fputs(",\"device_id\":", out) == EOF || !js(out, c->device_id) ||
        fprintf(out, ",\"device_index\":%zu,\"attachment_port_id\":",
                c->device_index) < 0 ||
        !js(out, c->attachment_port_id) ||
        fprintf(out, ",\"attachment_port_index\":%zu,\"target\":{\"kind\":",
                c->attachment_port_index) < 0 ||
        !js(out, c->target_kind) || fputs(",\"bus_id\":", out) == EOF ||
        !nullable(out, c->bus_id) ||
        fputs(",\"board_endpoint_id\":", out) == EOF ||
        !nullable(out, c->board_endpoint_id) ||
        fprintf(out, ",\"target_index\":%zu},\"allocation_claims\":[",
                c->target_index) < 0)
      return false;
    for (size_t j = 0; j < c->claim_count; ++j) {
      const FisicsHardwareAllocationClaim *a = &c->claims[j];
      if (j && fputc(',', out) == EOF)
        return false;
      if (fputs("{\"space_id\":", out) == EOF || !js(out, a->space_id) ||
          fprintf(out, ",\"space_index\":%zu,\"value_uint64\":",
                  a->space_index) < 0 ||
          !nullable(out, a->value_uint64) ||
          fputs(",\"value_id\":", out) == EOF || !nullable(out, a->value_id) ||
          fputc('}', out) == EOF)
        return false;
    }
    if (fprintf(out, "]}%s\n", i + 1u == g->connection_count ? "" : ",") < 0)
      return false;
  }
  if (fputs("  ],\n  \"signal_bindings\": [\n", out) == EOF)
    return false;
  for (size_t i = 0; i < g->signal_binding_count; ++i) {
    const FisicsHardwareSignalBinding *b = &g->signal_bindings[i];
    if (fputs("    {\"binding_id\":", out) == EOF || !js(out, b->id) ||
        fputs(",\"device_id\":", out) == EOF || !js(out, b->device_id) ||
        fprintf(out, ",\"device_index\":%zu,\"physical_port_id\":",
                b->device_index) < 0 ||
        !js(out, b->physical_port_id) ||
        fprintf(out, ",\"physical_port_index\":%zu,\"physical_contract_id\":",
                b->physical_port_index) < 0 ||
        !js(out, b->physical_contract_id) ||
        fprintf(out,
                ",\"physical_contract_index\":%zu,\"physical_evidence_state\":",
                b->physical_contract_index) < 0 ||
        !js(out, b->physical_evidence_state) ||
        fputs(",\"compatibility_mode\":", out) == EOF ||
        !js(out, b->compatibility_mode) ||
        fprintf(out, ",\"result\":\"compatible\"}%s\n",
                i + 1u == g->signal_binding_count ? "" : ",") < 0)
      return false;
  }
  if (fputs("  ],\n  \"unbound_required_ports\": [\n", out) == EOF)
    return false;
  for (size_t i = 0; i < g->unbound_port_count; ++i) {
    const FisicsHardwareUnboundPort *p = &g->unbound_ports[i];
    if (fputs("    {\"device_id\":", out) == EOF || !js(out, p->device_id) ||
        fputs(",\"port_kind\":", out) == EOF || !js(out, p->port_kind) ||
        fputs(",\"port_id\":", out) == EOF || !js(out, p->port_id) ||
        fprintf(out, "}%s\n", i + 1u == g->unbound_port_count ? "" : ",") < 0)
      return false;
  }
  return fputs("  ]\n}\n", out) != EOF;
}

static char *canonical_output(const char *path) {
  char *existing = realpath(path, NULL);
  if (existing)
    return existing;
  const char *slash = strrchr(path, '/');
  char *directory = NULL;
  const char *leaf = path;
  if (slash) {
    size_t n = (size_t)(slash - path);
    if (!n)
      directory = strdup("/");
    else {
      directory = (char *)malloc(n + 1u);
      if (directory) {
        memcpy(directory, path, n);
        directory[n] = '\0';
      }
    }
    leaf = slash + 1u;
  } else
    directory = strdup(".");
  if (!directory || !leaf[0]) {
    free(directory);
    return NULL;
  }
  char *base = realpath(directory, NULL);
  free(directory);
  if (!base)
    return NULL;
  size_t n = strlen(base) + strlen(leaf) + 2u;
  char *out = (char *)malloc(n);
  if (out)
    snprintf(out, n, "%s%s%s", base, strcmp(base, "/") ? "/" : "", leaf);
  free(base);
  return out;
}

bool fisics_hardware_project_graph_write_json_atomic(
    const char *output_path, const FisicsHardwareProjectManifest *manifest,
    const FisicsHardwareProjectGraph *graph,
    FisicsSemanticProjectDiagnostic *d) {
  if (!output_path || !output_path[0] || !manifest || !graph) {
    diag(d, "invalid hardware graph publication arguments");
    return false;
  }
  char *output = canonical_output(output_path);
  if (!output) {
    diag(d, "cannot resolve hardware graph output path: %s", output_path);
    return false;
  }
  bool conflict = !strcmp(output, manifest->manifest_path) ||
                  !strcmp(output, manifest->physical_graph_path) ||
                  !strcmp(output, manifest->topology_path);
  for (size_t i = 0; !conflict && i < manifest->catalog_path_count; ++i)
    conflict = !strcmp(output, manifest->catalog_paths[i]);
  for (size_t i = 0; !conflict && i < graph->physical_source_paths.count; ++i)
    conflict = !strcmp(output, graph->physical_source_paths.items[i]);
  if (conflict) {
    diag(d, "hardware graph output conflicts with an input or source path: %s",
         output);
    free(output);
    return false;
  }
  size_t n = strlen(output);
  char *temp = malloc(n + sizeof(".tmp.XXXXXX"));
  if (!temp) {
    free(output);
    diag(d, "out of memory creating hardware graph output");
    return false;
  }
  snprintf(temp, n + sizeof(".tmp.XXXXXX"), "%s.tmp.XXXXXX", output);
  int fd = mkstemp(temp);
  if (fd < 0) {
    diag(d, "failed to create hardware graph temporary output: %s", output);
    free(temp);
    free(output);
    return false;
  }
  FILE *out = fdopen(fd, "w");
  bool ok = out && write_document(out, graph);
  if (getenv("FISICS_TEST_FAIL_HARDWARE_PROJECT_GRAPH_WRITE"))
    ok = false;
  if (out) {
    if (ok && fflush(out) != 0)
      ok = false;
    if (ok && fsync(fd) != 0)
      ok = false;
    if (fclose(out) != 0)
      ok = false;
  } else
    close(fd);
  if (ok && rename(temp, output) != 0)
    ok = false;
  if (!ok) {
    unlink(temp);
    diag(d, "failed to atomically publish hardware project graph: %s", output);
  }
  free(temp);
  free(output);
  return ok;
}
