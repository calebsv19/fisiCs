// SPDX-License-Identifier: Apache-2.0
#include "Extensions/PhysicalSemantics/physical_frames_internal.h"
#include <inttypes.h>
/* Canonical submodel serialization, shared by v6 source coverage receipts. */
void physical_frames_write(FILE *out, const FisicsPhysicalPolicy *p) {
  fprintf(out, "{\"version\":1,\"fingerprint\":\"%016" PRIx64 "\"",
          p->frame_fingerprint);
#define WRITE(member, nitems)                                                  \
  fputs(",\"" #member "\":[", out);                                            \
  for (size_t i = 0; i < p->nitems; i++)                                       \
    fprintf(out, "%s{\"id\":\"%s\",\"fingerprint\":\"%016" PRIx64 "\"}",       \
            i ? "," : "", p->member[i].id, p->member[i].fingerprint);          \
  fputc(']', out)
  WRITE(frame_contexts, frame_context_count);
  WRITE(reference_points, reference_point_count);
  WRITE(frames, frame_count);
  WRITE(frame_transforms, frame_transform_count);
  fputc('}', out);
}
bool physical_frames_match(const FisicsJsonValue *j,
                           const FisicsPhysicalPolicy *p) {
  const char *const fields[] = {"version",        "fingerprint",
                                "frame_contexts", "reference_points",
                                "frames",         "frame_transforms"},
                    *const ef[] = {"id", "fingerprint"};
  int version = 0;
  char fp[17];
  snprintf(fp, sizeof(fp), "%016" PRIx64, p->frame_fingerprint);
  if (!physical_exact_fields(j, fields, 6) ||
      !fisics_json_integer_int(fg(j, "version"), &version) || version != 1 ||
      !fe(fg(j, "fingerprint"), fp))
    return false;
#define MATCH(member, nitems)                                                  \
  do {                                                                         \
    const FisicsJsonValue *a = fg(j, #member);                                 \
    if (!a || a->type != FISICS_JSON_ARRAY || a->as.array.count != p->nitems)  \
      return false;                                                            \
    for (size_t i = 0; i < p->nitems; i++) {                                   \
      const FisicsJsonValue *e = a->as.array.items[i];                         \
      snprintf(fp, sizeof(fp), "%016" PRIx64, p->member[i].fingerprint);       \
      if (!physical_exact_fields(e, ef, 2) ||                                  \
          !fe(fg(e, "id"), p->member[i].id) || !fe(fg(e, "fingerprint"), fp))  \
        return false;                                                          \
    }                                                                          \
  } while (0)
  MATCH(frame_contexts, frame_context_count);
  MATCH(reference_points, reference_point_count);
  MATCH(frames, frame_count);
  MATCH(frame_transforms, frame_transform_count);
  return true;
}
