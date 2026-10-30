// SPDX-License-Identifier: Apache-2.0
#include "main_internal.h"
#include <stdio.h>
bool main_physical_options_valid(FisicsOverlayFeatures features,
                                 const char *policy, size_t sources,
                                 size_t objects, bool manifest,
                                 PreprocessMode mode) {
  bool physical = (features & FISICS_OVERLAY_PHYSICS_SEMANTICS) != 0;
  if (!physical && !policy)
    return true;
  if (!physical || !policy || !policy[0]) {
    fprintf(stderr, "Error: physics-semantics and --physical-semantics-policy "
                    "require each other\n");
    return false;
  }
  if (sources != 1 || objects || manifest || mode != PREPROCESS_INTERNAL) {
    fprintf(stderr, "Error: P5.3 physical semantics requires one direct C "
                    "translation unit and internal preprocessing; "
                    "multi-TU/objects/build manifests are unsupported\n");
    return false;
  }
  return true;
}
