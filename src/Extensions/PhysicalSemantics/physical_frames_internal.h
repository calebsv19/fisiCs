// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "Extensions/PhysicalSemantics/physical_frames.h"
#include "Extensions/PhysicalSemantics/physical_representations.h"
#include "Extensions/SemanticMetadata/semantic_metadata_parser.h"
#include <string.h>
static inline const FisicsJsonValue *fg(const FisicsJsonValue *j,
                                        const char *k) {
  return fisics_json_object_get(j, k);
}
static inline const char *fs(const FisicsJsonValue *j) {
  return j && j->type == FISICS_JSON_STRING ? j->as.text : NULL;
}
static inline bool fe(const FisicsJsonValue *j, const char *s) {
  const char *v = fs(j);
  return v && !strcmp(v, s);
}
static inline bool fn(const char *s) {
  return s && fisics_semantic_metadata_is_canonical_name(s, NULL);
}
static inline uint64_t fnum(uint64_t h, uint64_t n) {
  for (unsigned i = 0; i < 8; i++, n >>= 8)
    h = (h ^ (unsigned char)n) * UINT64_C(1099511628211);
  return h;
}
static inline uint64_t ftext(uint64_t h, const char *s) {
  h = fnum(h, strlen(s));
  for (; *s; s++)
    h = (h ^ (unsigned char)*s) * UINT64_C(1099511628211);
  return h;
}
static inline uint64_t fstart(const char *s) {
  return ftext(UINT64_C(1469598103934665603), s);
}
bool physical_frame_transforms_load(const FisicsJsonValue *,
                                    FisicsPhysicalPolicy *,
                                    FisicsPhysicalLoadContext *,
                                    FisicsSemanticProjectDiagnostic *);
