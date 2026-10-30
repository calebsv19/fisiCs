// SPDX-License-Identifier: Apache-2.0
#include "Extensions/PhysicalSemantics/physical_frames_internal.h"
static bool vector(const FisicsPhysicalRepresentation *r, bool axial) {
  return r->record && !r->single_precision && r->rank == 1 && r->extent == 3 &&
         r->axial == axial && r->field_count == 3 &&
         !strcmp(r->fields[0], "x") && !strcmp(r->fields[1], "y") &&
         !strcmp(r->fields[2], "z");
}
bool physical_frame_transforms_load(const FisicsJsonValue *ts,
                                    FisicsPhysicalPolicy *p,
                                    FisicsPhysicalLoadContext *l,
                                    FisicsSemanticProjectDiagnostic *d) {
  if (p->frame_transform_count &&
      !(p->frame_transforms = physical_calloc(l, p->frame_transform_count,
                                              sizeof(*p->frame_transforms))))
    return false;
  const char *const fields[] = {"id",           "source_frame",
                                "target_frame", "operation",
                                "input_kind",   "input_representation",
                                "output_kind",  "output_representation",
                                "provenance"};
  for (size_t i = 0; i < p->frame_transform_count; i++) {
    const FisicsJsonValue *j = ts->as.array.items[i];
    PhysicalFrameTransform *t = &p->frame_transforms[i];
    const char *id = fs(fg(j, "id"));
    if (!physical_exact_fields(j, fields, 9) || !fn(id) ||
        !fe(fg(j, "provenance"), "declared_implementation_and_basis"))
      goto invalid;
    t->source = physical_frame_find(p, fs(fg(j, "source_frame")));
    t->target = physical_frame_find(p, fs(fg(j, "target_frame")));
    t->input_kind =
        fisics_physical_policy_find_kind(p, fs(fg(j, "input_kind")));
    t->output_kind =
        fisics_physical_policy_find_kind(p, fs(fg(j, "output_kind")));
    t->input_representation =
        physical_representation_find(p, fs(fg(j, "input_representation")));
    t->output_representation =
        physical_representation_find(p, fs(fg(j, "output_representation")));
    if (!t->source || !t->target || !t->input_kind ||
        t->input_kind != t->output_kind || !t->input_representation ||
        t->input_representation != t->output_representation)
      goto invalid;
    const PhysicalFrame *s = &p->frames[t->source - 1],
                        *o = &p->frames[t->target - 1];
    if (s->dimension != 3 || o->dimension != 3 || s->context != o->context)
      goto invalid;
    const char *kind = p->kinds[t->input_kind - 1].id;
    bool torque = !strcmp(kind, "quantity.torque");
    bool cosmic=physical_cosmic_vector_kind(p,t->input_kind,false) || physical_cosmic_vector_kind(p,t->input_kind,true);
    cosmic=cosmic && strcmp(kind,"quantity.momentum") && strcmp(kind,"quantity.ratio") && strcmp(kind,"quantity.eccentricity_numerator");
    static const int8_t force_dim[8] = {1, 1, -2}, length_dim[8] = {1},
                        torque_dim[8] = {2, 1, -2};
    const int8_t *dim = torque                            ? torque_dim
                        : !strcmp(kind, "quantity.force") ? force_dim
                                                          : length_dim;
    if (!cosmic && memcmp(p->kinds[t->input_kind - 1].dimension, dim, 8))
      goto invalid;
    if (!vector(&p->representations[t->input_representation - 1], torque || physical_cosmic_vector_kind(p,t->input_kind,true)))
      goto invalid;
    if (fe(fg(j, "operation"), "rotate_vector")) {
      t->operation = PHYSICAL_ROTATE_VECTOR;
      if (!cosmic && !torque && strcmp(kind, "quantity.force") &&
          strcmp(kind, "quantity.displacement"))
        goto invalid;
      FisicsPhysicalReferenceRole role = p->models[t->input_kind - 1].role;
      if (role != FISICS_PHYSICAL_LINEAR && role != FISICS_PHYSICAL_DIFFERENCE)
        goto invalid;
    } else if (fe(fg(j, "operation"), "map_point")) {
      t->operation = PHYSICAL_MAP_POINT;
      uint32_t paired = p->models[t->input_kind - 1].paired_kind;
      if (strcmp(kind, "quantity.position") ||
          p->models[t->input_kind - 1].role != FISICS_PHYSICAL_ABSOLUTE ||
          !paired || strcmp(p->kinds[paired - 1].id, "quantity.displacement") ||
          !s->affine_origin || !o->affine_origin)
        goto invalid;
    } else
      goto invalid;
    if (!(t->id = physical_dup_text(l, id)))
      return false;
    uint64_t h = ftext(fstart("fisiCs.frame_transform.v1"), id);
    h = fnum(h, s->fingerprint);
    h = fnum(h, o->fingerprint);
    h = fnum(h, t->operation);
    h = ftext(h, kind);
    h = fnum(h, p->models[t->input_kind - 1].role);
    h = ftext(h, p->models[t->input_kind - 1].paired_kind
                     ? p->kinds[p->models[t->input_kind - 1].paired_kind - 1].id
                     : "");
    for (unsigned n = 0; n < 8; n++)
      h = fnum(h, (unsigned char)p->kinds[t->input_kind - 1].dimension[n]);
    h = fnum(h, p->representations[t->input_representation - 1].fingerprint);
    t->fingerprint = ftext(h, "declared_implementation_and_basis");
  }
  return true;
invalid:
  physical_load_diag(
      d, "invalid or incompatible directed frame transform contract");
  return false;
}
