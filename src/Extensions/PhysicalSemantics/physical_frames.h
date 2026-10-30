// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "Extensions/PhysicalSemantics/physical_policy.h"
#include <stdio.h>
#define PHYSICAL_MAX_FRAME_CONTEXTS 64u
#define PHYSICAL_MAX_REFERENCE_POINTS 256u
#define PHYSICAL_MAX_FRAMES 256u
#define PHYSICAL_MAX_FRAME_TRANSFORMS 512u
/* All handles are local to the owning policy. Zero denotes absent/unknown.
 * Coordinate origins, physical points, and affine origins remain distinct. */
typedef struct PhysicalFrameContext {
  char *id;
  uint64_t fingerprint;
  char *observer_id, *state_id;
} PhysicalFrameContext;
typedef struct PhysicalReferencePoint {
  char *id;
  uint64_t fingerprint;
  uint32_t context;
} PhysicalReferencePoint;
typedef struct PhysicalFrame {
  char *id;
  uint64_t fingerprint;
  uint32_t context, dimension, origin_point, affine_origin;
} PhysicalFrame;
typedef enum {
  PHYSICAL_ROTATE_VECTOR = 1,
  PHYSICAL_MAP_POINT
} PhysicalTransformOperation;
typedef struct PhysicalFrameTransform {
  char *id;
  uint64_t fingerprint;
  uint32_t source, target, input_kind, output_kind;
  uint32_t input_representation, output_representation;
  PhysicalTransformOperation operation;
} PhysicalFrameTransform;
bool physical_frames_load(const FisicsJsonValue *, FisicsPhysicalPolicy *,
                          FisicsPhysicalLoadContext *,
                          FisicsSemanticProjectDiagnostic *);
void physical_frames_free(FisicsPhysicalPolicy *);
uint32_t physical_frame_context_find(const FisicsPhysicalPolicy *,
                                     const char *);
uint32_t physical_reference_point_find(const FisicsPhysicalPolicy *,
                                       const char *);
uint32_t physical_frame_find(const FisicsPhysicalPolicy *, const char *);
uint32_t physical_frame_transform_find(const FisicsPhysicalPolicy *,
                                       const char *);
void physical_frames_write(FILE *, const FisicsPhysicalPolicy *);
bool physical_frames_match(const FisicsJsonValue *,
                           const FisicsPhysicalPolicy *);
