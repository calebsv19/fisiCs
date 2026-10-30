// SPDX-License-Identifier: Apache-2.0
#include "Compiler/physical_energy_receipt.h"
#include "Compiler/physical_receipt.h"
#include "Extensions/PhysicalSemantics/physical_frames.h"
#include "Extensions/PhysicalSemantics/physical_representations.h"
#include <inttypes.h>
#include <string.h>
typedef FisicsJsonValue J;
static const J *g(const J *j, const char *k) {
  return fisics_json_object_get(j, k);
}
static const char *s(const J *j) {
  return j && j->type == FISICS_JSON_STRING ? j->as.text : NULL;
}
static bool eq(const J *j, const char *v) {
  const char *x = s(j);
  return x && v && !strcmp(x, v);
}
static bool null(const J *j) { return j && j->type == FISICS_JSON_NULL; }
static bool yes(const J *j) {
  return j && j->type == FISICS_JSON_BOOL && j->as.boolean;
}
static bool flag(const J *j) { return j && j->type == FISICS_JSON_BOOL; }
static bool n(const J *j, size_t *v) { return fisics_json_integer_size(j, v); }
static bool choice(const J *j, const char *list) {
  const char *v = s(j);
  if (!v)
    return false;
  size_t len = strlen(v);
  for (const char *p = list; *p;) {
    const char *e = strchr(p, '|');
    size_t k = e ? (size_t)(e - p) : strlen(p);
    if (k == len && !strncmp(p, v, k))
      return true;
    if (!e)
      break;
    p = e + 1;
  }
  return false;
}
bool physical_frame_fact_match(const J *f, const FisicsPhysicalPolicy *p) {
  const char *const ff[] = {"kind",
                            "role",
                            "origin",
                            "carrier",
                            "verified",
                            "definition_fingerprint",
                            "representation",
                            "representation_fingerprint",
                            "component_depth",
                            "physical_context", "energy_form"};
  const char *const cf[] = {"frame",           "component_axis",
                            "applied_at",      "displacement_from",
                            "displacement_to", "moment_reference",
                            "point_subject",   "facets"};
  const char *const facets[] = {"frame_identity", "compatibility", "geometry",
                                "observer_state", "references"};
  const J *v = g(f, "physical_context"), *a = g(v, "facets");
  if (!physical_exact_fields(f, ff, p->schema_version==7?11:10) || !physical_exact_fields(v, cf, 8) ||
      !physical_exact_fields(a, facets, 5) || !flag(g(f, "verified")) ||
      !flag(g(f, "carrier")))
    return false;
  if (!choice(g(a, "frame_identity"),
              "declared|unknown|contradictory|not_applicable") ||
      !choice(g(a, "compatibility"),
              "checked|unknown|contradictory|not_applicable") ||
      !choice(g(a, "geometry"), "declared|unassessed|not_applicable") ||
      !choice(g(a, "observer_state"), "declared|unassessed|not_applicable") ||
      !choice(
          g(a, "references"),
          "declared|checked_compatible|unknown|contradictory|not_applicable"))
    return false;
  if(p->schema_version==7 && !physical_energy_fact_match(f,p))return false;
  const char *kid = s(g(f, "kind"));
  uint32_t kind = fisics_physical_policy_find_kind(p, kid);
  size_t role, depth;
  if (!n(g(f, "role"), &role) || !n(g(f, "component_depth"), &depth) ||
      depth > 2)
    return false;
  if (kid) {
    char hash[17];
    if (!kind || role != (unsigned)p->models[kind - 1].role)
      return false;
    snprintf(hash, sizeof(hash), "%016" PRIx64,
             p->models[kind - 1].definition_fingerprint);
    if (!eq(g(f, "definition_fingerprint"), hash))
      return false;
  } else if (!null(g(f, "kind")) || role ||
             !null(g(f, "definition_fingerprint")) || yes(g(f, "verified")))
    return false;
  const char *rid = s(g(f, "representation"));
  uint32_t rep = physical_representation_find(p, rid);
  if (rid) {
    char hash[17];
    if (!rep || !kind)
      return false;
    snprintf(hash, sizeof(hash), "%016" PRIx64,
             p->representations[rep - 1].fingerprint);
    if (!eq(g(f, "representation_fingerprint"), hash))
      return false;
  } else if (!null(g(f, "representation")) ||
             !null(g(f, "representation_fingerprint")))
    return false;
  if (rep && p->representations[rep - 1].rank == 1 && null(g(v, "frame")) &&
      !choice(g(a, "frame_identity"), "unknown|contradictory"))
    return false;
  const char *origin = s(g(f, "origin"));
  uint32_t oid = fisics_physical_policy_find_origin(p, origin);
  if (origin) {
    if (!oid || !kind || p->origins[oid - 1].absolute_kind != kind)
      return false;
  } else if (!null(g(f, "origin")))
    return false;
  const char *frame = s(g(v, "frame"));
  uint32_t fid = physical_frame_find(p, frame);
  if (frame) {
    if (!fid)
      return false;
  } else if (!null(g(v, "frame")))
    return false;
  const char *axis = s(g(v, "component_axis"));
  if (axis) {
    if (!fid || !choice(g(v, "component_axis"), "axis.x|axis.y|axis.z") ||
        rep || (!strcmp(axis, "axis.z") && p->frames[fid - 1].dimension != 3))
      return false;
  } else if (!null(g(v, "component_axis")))
    return false;
  for (unsigned i = 2; i < 7; i++) {
    const char *id = s(g(v, cf[i]));
    uint32_t ref = physical_reference_point_find(p, id);
    if (id) {
      if (!ref || !fid ||
          p->reference_points[ref - 1].context != p->frames[fid - 1].context)
        return false;
    } else if (!null(g(v, cf[i])))
      return false;
  }
  if (null(g(v, "displacement_from")) != null(g(v, "displacement_to")))
    return false;
  if (fid) {
    if (!kind)
      return false;
    const char *k = p->kinds[kind - 1].id;
    bool cosmic_axial=physical_cosmic_vector_kind(p,kind,true);
    bool cosmic=cosmic_axial || physical_cosmic_vector_kind(p,kind,false);
    if (!cosmic && strcmp(k, "quantity.force") && strcmp(k, "quantity.displacement") &&
        strcmp(k, "quantity.torque") && strcmp(k, "quantity.position"))
      return false;
    if (rep && cosmic && p->representations[rep-1].axial!=cosmic_axial)return false;
    if (rep &&
        (p->representations[rep - 1].rank != 1 ||
         p->representations[rep - 1].extent != p->frames[fid - 1].dimension))
      return false;
    if (!rep && !axis)
      return false;
    if (!null(g(v, "applied_at")) && strcmp(k, "quantity.force"))
      return false;
    if (!null(g(v, "moment_reference")) && strcmp(k, "quantity.torque"))
      return false;
    if (!null(g(v, "point_subject")) && strcmp(k, "quantity.position"))
      return false;
    if (!null(g(v, "displacement_from")) && strcmp(k, "quantity.displacement"))
      return false;
    if (!strcmp(k, "quantity.position") &&
        oid != p->frames[fid - 1].affine_origin)
      return false;
    if (!eq(g(a, "frame_identity"), "declared") &&
        !eq(g(a, "frame_identity"), "contradictory"))
      return false;
    if (!eq(g(a, "geometry"), "declared") ||
        !eq(g(a, "observer_state"), "declared"))
      return false;
  } else if (eq(g(a, "frame_identity"), "declared"))
    return false;
  return true;
}
bool physical_frame_signature_match(const J *d, const FisicsPhysicalPolicy *p) {
  const J *tj = g(d, "frame_transform");
  const char *id = s(tj);
  if (!id)
    return null(tj);
  uint32_t h = physical_frame_transform_find(p, id);
  const J *slots = g(d, "slots");
  if (!h || !slots || slots->type != FISICS_JSON_ARRAY ||
      slots->as.array.count != 2 || !eq(g(d, "entity"), "function") ||
      yes(g(d, "variadic")) || !yes(g(d, "prototype")))
    return false;
  const PhysicalFrameTransform *t = &p->frame_transforms[h - 1];
  const J *ret = g(slots->as.array.items[0], "fact"),
          *arg = g(slots->as.array.items[1], "fact");
  for (unsigned i = 0; i < 2; i++) {
    const J *f = i ? ret : arg, *v = g(f, "physical_context");
    if (!eq(g(f, "kind"),
            p->kinds[(i ? t->output_kind : t->input_kind) - 1].id) ||
        !eq(g(f, "representation"),
            p->representations
                [(i ? t->output_representation : t->input_representation) - 1]
                    .id) ||
        !eq(g(v, "frame"), p->frames[(i ? t->target : t->source) - 1].id) ||
        !null(g(v, "component_axis")) || !yes(g(f, "verified")))
      return false;
  }
  const char *refs[] = {"applied_at", "displacement_from", "displacement_to",
                        "moment_reference", "point_subject"};
  for (unsigned i = 0; i < 5; i++) {
    const J *x = g(g(ret, "physical_context"), refs[i]),
            *y = g(g(arg, "physical_context"), refs[i]);
    if (!(null(x) && null(y)) && !eq(x, s(y)))
      return false;
  }
  return true;
}
bool physical_frame_document_match(const J *root,
                                   const FisicsPhysicalPolicy *p) {
  const char *arrays[] = {"expressions", "obligations", "declaration_bindings"};
  for (unsigned i = 0; i < 3; i++) {
    const J *a = g(root, arrays[i]);
    if (!a || a->type != FISICS_JSON_ARRAY || a->as.array.count > 131072)
      return false;
    for (size_t k = 0; k < a->as.array.count; k++)
      if (!physical_frame_fact_match(g(a->as.array.items[k], "fact"), p))
        return false;
  }
  const J *sel = g(root, "selection"), *rows = g(root, "obligations");
  size_t total, required, controls, checked, permitted, blocked;
  if (!n(g(sel, "total"), &total) || !n(g(sel, "required"), &required) ||
      !n(g(sel, "ordinary_controls"), &controls) ||
      !n(g(sel, "checked"), &checked) ||
      !n(g(sel, "permitted_trust"), &permitted) ||
      !n(g(sel, "blocked"), &blocked) || total != rows->as.array.count ||
      total > 131072 || required + controls != total ||
      checked + permitted + blocked != required)
    return false;
  if (yes(g(sel, "accepted")) &&
      (!checked || blocked || !yes(g(sel, "enumeration_complete")) ||
       !yes(g(sel, "ordinary_controls_valid"))))
    return false;
  size_t rc = 0, cc = 0;
  for (size_t i = 0; i < rows->as.array.count; i++) {
    const J *row = rows->as.array.items[i];
    if (!flag(g(row, "strict_required")) || !flag(g(row, "checked")))
      return false;
    if (yes(g(row, "strict_required")))
      rc++;
    if (yes(g(row, "checked"))) {
      cc++;
      if (!eq(g(row, "state"), "checked"))
        return false;
    }
  }
  return rc == required && cc == checked;
}
