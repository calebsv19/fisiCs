// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef uint64_t FisicsOverlayFeatures;

enum {
    FISICS_OVERLAY_NONE = 0,
    FISICS_OVERLAY_IDE_METADATA = (1ULL << 0),
    FISICS_OVERLAY_PHYSICS_UNITS = (1ULL << 1)
};

FisicsOverlayFeatures fisics_all_overlay_features(void);
bool fisics_overlay_has_feature(FisicsOverlayFeatures features, FisicsOverlayFeatures feature);
bool fisics_parse_overlay_mode(const char* mode, FisicsOverlayFeatures* overlayFeatures);
