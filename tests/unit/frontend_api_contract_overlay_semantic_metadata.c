#include <stdio.h>

#include "Compiler/compiler_context.h"
#include "Extensions/SemanticMetadata/semantic_metadata_parser.h"
#include "Extensions/extension_profile.h"

int main(void) {
    FisicsOverlayFeatures parsed = FISICS_OVERLAY_NONE;
    if (!fisics_parse_overlay_mode("semantic-metadata", &parsed) ||
        !fisics_overlay_has_feature(parsed, FISICS_OVERLAY_SEMANTIC_METADATA)) {
        fprintf(stderr, "semantic-metadata overlay did not parse\n");
        return 1;
    }
    if (fisics_overlay_has_feature(parsed, FISICS_OVERLAY_IDE_METADATA) ||
        fisics_overlay_has_feature(parsed, FISICS_OVERLAY_PHYSICS_UNITS) ||
        fisics_overlay_has_feature(parsed, FISICS_OVERLAY_MEMORY_CHECK)) {
        fprintf(stderr, "semantic-metadata polluted another overlay bit\n");
        return 1;
    }
    if (!fisics_parse_overlay_mode("physics-units,semantic_metadata", &parsed) ||
        !fisics_overlay_has_feature(parsed, FISICS_OVERLAY_PHYSICS_UNITS) ||
        !fisics_overlay_has_feature(parsed, FISICS_OVERLAY_SEMANTIC_METADATA)) {
        fprintf(stderr, "combined semantic-metadata overlay did not parse\n");
        return 1;
    }
    if (fisics_overlay_has_feature(fisics_all_overlay_features(),
                                   FISICS_OVERLAY_SEMANTIC_METADATA)) {
        fprintf(stderr, "experimental semantic-metadata must remain outside overlay=all\n");
        return 1;
    }

    size_t descriptorCount = 0;
    const FisicsSemanticMetadataDescriptor* descriptors =
        fisics_semantic_metadata_descriptors(&descriptorCount);
    if (!descriptors || descriptorCount != 5) {
        fprintf(stderr, "semantic-metadata descriptor table is incomplete\n");
        return 1;
    }
    const FisicsSemanticMetadataDescriptor* produces =
        fisics_semantic_metadata_descriptor(FISICS_METADATA_ATTR_PRODUCES);
    const FisicsSemanticMetadataDescriptor* consumes =
        fisics_semantic_metadata_descriptor(FISICS_METADATA_ATTR_CONSUMES);
    if (!produces || !consumes || !produces->relationship ||
        !consumes->relationship || !produces->repeatable ||
        !consumes->repeatable ||
        produces->valuePolicy != FISICS_METADATA_VALUE_CANONICAL_NAME ||
        consumes->valuePolicy != FISICS_METADATA_VALUE_CANONICAL_NAME ||
        produces->allowedTargetMask !=
            (FISICS_METADATA_TARGET_FUNCTION |
             FISICS_METADATA_TARGET_FILE_VARIABLE) ||
        consumes->allowedTargetMask != produces->allowedTargetMask) {
        fprintf(stderr, "SM-4 relationship descriptors do not match the frozen contract\n");
        return 1;
    }

    CompilerContext* ctx = cc_create();
    if (!ctx) return 1;
    if (cc_overlay_semantic_metadata_enabled(ctx)) {
        fprintf(stderr, "semantic-metadata should default off\n");
        cc_destroy(ctx);
        return 1;
    }
    cc_set_overlay_features(ctx, FISICS_OVERLAY_SEMANTIC_METADATA);
    if (!cc_overlay_semantic_metadata_enabled(ctx) ||
        cc_overlay_physics_units_enabled(ctx) ||
        cc_overlay_memory_check_enabled(ctx)) {
        fprintf(stderr, "semantic-metadata context helper is not isolated\n");
        cc_destroy(ctx);
        return 1;
    }
    cc_destroy(ctx);
    return 0;
}
