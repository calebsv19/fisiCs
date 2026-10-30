// SPDX-License-Identifier: Apache-2.0
#include "Extensions/PhysicalSemantics/physical_frames_internal.h"
#include <stdlib.h>
/* Sort typed arrays without aliasing unlike descriptor layouts. */
#define TABLE(name, member, count, Type)                                       \
  static int name##_order(const void *a, const void *b) {                      \
    return strcmp(((const Type *)a)->id, ((const Type *)b)->id);               \
  }                                                                            \
  uint32_t name(const FisicsPhysicalPolicy *p, const char *id) {               \
    if (!p || !id)                                                             \
      return 0;                                                                \
    for (size_t i = 0; i < p->count; i++)                                      \
      if (!strcmp(p->member[i].id, id))                                        \
        return (uint32_t)i + 1;                                                \
    return 0;                                                                  \
  }
TABLE(physical_frame_context_find, frame_contexts, frame_context_count,
      PhysicalFrameContext)
TABLE(physical_reference_point_find, reference_points, reference_point_count,
      PhysicalReferencePoint)
TABLE(physical_frame_find, frames, frame_count, PhysicalFrame)
TABLE(physical_frame_transform_find, frame_transforms, frame_transform_count,
      PhysicalFrameTransform)
#undef TABLE
#define SORT(member, count, cmp)                                               \
  do {                                                                         \
    if (p->count > 1)                                                          \
      qsort(p->member, p->count, sizeof(*p->member), cmp);                     \
    for (size_t i = 1; i < p->count; i++)                                      \
      if (!strcmp(p->member[i - 1].id, p->member[i].id))                       \
        goto invalid;                                                          \
  } while (0)
void physical_frames_free(FisicsPhysicalPolicy *p) {
  for (size_t i = 0; p->frame_contexts && i < p->frame_context_count; i++) {
    free(p->frame_contexts[i].id);
    free(p->frame_contexts[i].observer_id);
    free(p->frame_contexts[i].state_id);
  }
  free(p->frame_contexts);
  for (size_t i = 0; p->reference_points && i < p->reference_point_count; i++)
    free(p->reference_points[i].id);
  free(p->reference_points);
  for (size_t i = 0; p->frames && i < p->frame_count; i++)
    free(p->frames[i].id);
  free(p->frames);
  for (size_t i = 0; p->frame_transforms && i < p->frame_transform_count; i++)
    free(p->frame_transforms[i].id);
  free(p->frame_transforms);
}
static bool table(const FisicsJsonValue *j, size_t ceiling,
                  FisicsPhysicalLoadContext *l) {
  if (!j || j->type != FISICS_JSON_ARRAY)
    return false;
  if (j->as.array.count > ceiling) {
    l->failure = PHYSICAL_LOAD_LIMIT;
    return false;
  }
  return true;
}
bool physical_frames_load(const FisicsJsonValue *root, FisicsPhysicalPolicy *p,
                          FisicsPhysicalLoadContext *l,
                          FisicsSemanticProjectDiagnostic *d) {
  const FisicsJsonValue *cs = fg(root, "frame_contexts"),
                        *ps = fg(root, "reference_points"),
                        *fr = fg(root, "frames"),
                        *ts = fg(root, "frame_transforms");
  if (!table(cs, l->options.max_frame_contexts, l) ||
      !table(ps, l->options.max_reference_points, l) ||
      !table(fr, l->options.max_frames, l) ||
      !table(ts, l->options.max_frame_transforms, l))
    goto invalid;
  p->frame_context_count = cs->as.array.count;
  p->reference_point_count = ps->as.array.count;
  p->frame_count = fr->as.array.count;
  p->frame_transform_count = ts->as.array.count;
#define ALLOC(member, count)                                                   \
  if (p->count &&                                                              \
      !(p->member = physical_calloc(l, p->count, sizeof(*p->member))))         \
  return false
  ALLOC(frame_contexts, frame_context_count);
  const char *const cf[] = {"id", "observer_id", "state_id"};
  for (size_t i = 0; i < p->frame_context_count; i++) {
    const FisicsJsonValue *j = cs->as.array.items[i];
    PhysicalFrameContext *c = &p->frame_contexts[i];
    const char *id = fs(fg(j, "id")), *o = fs(fg(j, "observer_id")),
               *s = fs(fg(j, "state_id"));
    if (!physical_exact_fields(j, cf, 3) || !fn(id) || !fn(o) || !fn(s))
      goto invalid;
    if (!(c->id = physical_dup_text(l, id)) ||
        !(c->observer_id = physical_dup_text(l, o)) ||
        !(c->state_id = physical_dup_text(l, s)))
      return false;
    c->fingerprint =
        ftext(ftext(ftext(fstart("fisiCs.frame_context.v1"), id), o), s);
  }
  SORT(frame_contexts, frame_context_count, physical_frame_context_find_order);
  ALLOC(reference_points, reference_point_count);
  const char *const pf[] = {"id", "context_id"};
  for (size_t i = 0; i < p->reference_point_count; i++) {
    const FisicsJsonValue *j = ps->as.array.items[i];
    PhysicalReferencePoint *q = &p->reference_points[i];
    const char *id = fs(fg(j, "id"));
    q->context = physical_frame_context_find(p, fs(fg(j, "context_id")));
    if (!physical_exact_fields(j, pf, 2) || !fn(id) || !q->context)
      goto invalid;
    if (!(q->id = physical_dup_text(l, id)))
      return false;
    q->fingerprint = fnum(ftext(fstart("fisiCs.reference_point.v1"), id),
                          p->frame_contexts[q->context - 1].fingerprint);
  }
  SORT(reference_points, reference_point_count,
       physical_reference_point_find_order);
  ALLOC(frames, frame_count);
  const char *const ff[] = {"id",         "context_id",   "dimension",
                            "convention", "origin_point", "affine_origin"};
  for (size_t i = 0; i < p->frame_count; i++) {
    const FisicsJsonValue *j = fr->as.array.items[i],
                          *a = fg(j, "affine_origin");
    PhysicalFrame *f = &p->frames[i];
    const char *id = fs(fg(j, "id"));
    int dim = 0;
    f->context = physical_frame_context_find(p, fs(fg(j, "context_id")));
    f->origin_point =
        physical_reference_point_find(p, fs(fg(j, "origin_point")));
    if (!physical_exact_fields(j, ff, 6) || !fn(id) || !f->context ||
        !f->origin_point ||
        p->reference_points[f->origin_point - 1].context != f->context ||
        !fisics_json_integer_int(fg(j, "dimension"), &dim) ||
        (dim != 2 && dim != 3) ||
        !fe(fg(j, "convention"), "cartesian_orthonormal_right_handed"))
      goto invalid;
    if (a->type != FISICS_JSON_NULL) {
      f->affine_origin = fisics_physical_policy_find_origin(p, fs(a));
      if (!f->affine_origin ||
          strcmp(
              p->kinds[p->origins[f->affine_origin - 1].absolute_kind - 1].id,
              "quantity.position"))
        goto invalid;
    }
    f->dimension = (uint32_t)dim;
    if (!(f->id = physical_dup_text(l, id)))
      return false;
    uint64_t h = ftext(fstart("fisiCs.frame.v1"), id);
    h = fnum(h, p->frame_contexts[f->context - 1].fingerprint);
    h = fnum(h, f->dimension);
    h = ftext(h, "cartesian_orthonormal_right_handed");
    h = fnum(h, p->reference_points[f->origin_point - 1].fingerprint);
    f->fingerprint =
        ftext(h, f->affine_origin ? p->origins[f->affine_origin - 1].id : "");
  }
  SORT(frames, frame_count, physical_frame_find_order);
  if (!physical_frame_transforms_load(ts, p, l, d))
    return false;
  SORT(frame_transforms, frame_transform_count,
       physical_frame_transform_find_order);
  uint64_t h = fstart("fisiCs.frame_model.v1");
#define HASH(member, count)                                                    \
  h = fnum(h, p->count);                                                       \
  for (size_t i = 0; i < p->count; i++)                                        \
  h = fnum(h, p->member[i].fingerprint)
  HASH(frame_contexts, frame_context_count);
  HASH(reference_points, reference_point_count);
  HASH(frames, frame_count);
  HASH(frame_transforms, frame_transform_count);
  p->frame_fingerprint = h;
  return true;
invalid:
  physical_load_diag(d, "invalid, duplicate or incompatible "
                        "frame/context/point/transform declaration");
  return false;
}
