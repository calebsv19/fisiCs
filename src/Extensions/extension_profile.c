// SPDX-License-Identifier: Apache-2.0

#include "Extensions/extension_profile.h"

#include <stdlib.h>
#include <string.h>

static char* trim_in_place(char* text) {
    if (!text) return NULL;
    while (*text == ' ' || *text == '\t' || *text == '\n' || *text == '\r') {
        ++text;
    }
    size_t len = strlen(text);
    while (len > 0) {
        char c = text[len - 1];
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
            break;
        }
        text[--len] = '\0';
    }
    return text;
}

FisicsOverlayFeatures fisics_all_overlay_features(void) {
    return FISICS_OVERLAY_IDE_METADATA |
           FISICS_OVERLAY_PHYSICS_UNITS;
}

bool fisics_overlay_has_feature(FisicsOverlayFeatures features, FisicsOverlayFeatures feature) {
    return (features & feature) != 0;
}

bool fisics_parse_overlay_mode(const char* mode, FisicsOverlayFeatures* overlayFeatures) {
    if (!mode || !overlayFeatures) return false;

    if (strcmp(mode, "0") == 0 ||
        strcmp(mode, "off") == 0 ||
        strcmp(mode, "none") == 0) {
        *overlayFeatures = FISICS_OVERLAY_NONE;
        return true;
    }
    if (strcmp(mode, "1") == 0 ||
        strcmp(mode, "on") == 0 ||
        strcmp(mode, "all") == 0) {
        *overlayFeatures = fisics_all_overlay_features();
        return true;
    }

    char* copy = strdup(mode);
    if (!copy) {
        return false;
    }

    bool ok = true;
    FisicsOverlayFeatures parsed = FISICS_OVERLAY_NONE;
    char* save = NULL;
    for (char* tok = strtok_r(copy, ",", &save);
         tok;
         tok = strtok_r(NULL, ",", &save)) {
        char* part = trim_in_place(tok);
        if (!part || part[0] == '\0') {
            ok = false;
            break;
        }
        if (strcmp(part, "all") == 0) {
            parsed |= fisics_all_overlay_features();
        } else if (strcmp(part, "ide") == 0 ||
                   strcmp(part, "ide-metadata") == 0 ||
                   strcmp(part, "ide_metadata") == 0) {
            parsed |= FISICS_OVERLAY_IDE_METADATA;
        } else if (strcmp(part, "units") == 0 ||
                   strcmp(part, "physics-units") == 0 ||
                   strcmp(part, "physics_units") == 0) {
            parsed |= FISICS_OVERLAY_PHYSICS_UNITS;
        } else {
            ok = false;
            break;
        }
    }

    free(copy);
    if (!ok) {
        return false;
    }

    *overlayFeatures = parsed;
    return true;
}
