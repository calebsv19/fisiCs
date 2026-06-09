#include <stdio.h>

#include "Compiler/compiler_context.h"
#include "Extensions/extension_profile.h"

int main(void) {
    FisicsOverlayFeatures parsed = FISICS_OVERLAY_NONE;

    if (!fisics_parse_overlay_mode("memory-check", &parsed)) {
        fprintf(stderr, "memory-check overlay did not parse\n");
        return 1;
    }
    if (!fisics_overlay_has_feature(parsed, FISICS_OVERLAY_MEMORY_CHECK)) {
        fprintf(stderr, "memory-check overlay bit not set\n");
        return 1;
    }
    if (fisics_overlay_has_feature(parsed, FISICS_OVERLAY_PHYSICS_UNITS)) {
        fprintf(stderr, "memory-check unexpectedly enabled physics-units\n");
        return 1;
    }

    if (!fisics_parse_overlay_mode("physics-units,memory_check", &parsed)) {
        fprintf(stderr, "combined overlay did not parse\n");
        return 1;
    }
    if (!fisics_overlay_has_feature(parsed, FISICS_OVERLAY_PHYSICS_UNITS) ||
        !fisics_overlay_has_feature(parsed, FISICS_OVERLAY_MEMORY_CHECK)) {
        fprintf(stderr, "combined overlay bits missing\n");
        return 1;
    }

    FisicsOverlayFeatures all = fisics_all_overlay_features();
    if (fisics_overlay_has_feature(all, FISICS_OVERLAY_MEMORY_CHECK)) {
        fprintf(stderr, "memory-check must remain outside overlay=all for Slice 0\n");
        return 1;
    }
    if (!fisics_overlay_has_feature(all, FISICS_OVERLAY_PHYSICS_UNITS)) {
        fprintf(stderr, "overlay=all lost physics-units\n");
        return 1;
    }

    CompilerContext* ctx = cc_create();
    if (!ctx) {
        fprintf(stderr, "failed to create compiler context\n");
        return 1;
    }
    if (cc_overlay_memory_check_enabled(ctx)) {
        fprintf(stderr, "memory-check should default off\n");
        cc_destroy(ctx);
        return 1;
    }
    cc_set_overlay_features(ctx, FISICS_OVERLAY_MEMORY_CHECK);
    if (!cc_overlay_memory_check_enabled(ctx)) {
        fprintf(stderr, "memory-check helper did not observe enabled bit\n");
        cc_destroy(ctx);
        return 1;
    }
    if (cc_overlay_physics_units_enabled(ctx)) {
        fprintf(stderr, "memory-check helper polluted physics-units helper\n");
        cc_destroy(ctx);
        return 1;
    }
    cc_destroy(ctx);

    return 0;
}
