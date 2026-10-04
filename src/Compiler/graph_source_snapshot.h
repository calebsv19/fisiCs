// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Offline source-receipt reads only. Identity in metadata is never rewritten.
 * An explicit snapshot is fail-closed: never fall back to the original path. */
static bool fisics_graph_snapshot_source(const char* identity, char out[PATH_MAX]) {
    const char* root = getenv("FISICS_GRAPH_SOURCE_SNAPSHOT");
    if (!root) {
        int count = snprintf(out, PATH_MAX, "%s", identity);
        return count >= 0 && count < PATH_MAX;
    }
    size_t length = strlen(identity);
    if (!root[0] || root[0] != '/' || identity[0] != '/' || length < 2 ||
        strstr(identity, "//") || strstr(identity, "/../") || strstr(identity, "/./") ||
        identity[length - 1] == '/' || strcmp(identity + length - 2, "/.") == 0 ||
        (length >= 3 && strcmp(identity + length - 3, "/..") == 0)) return false;
    char* canonical = realpath(root, NULL);
    bool valid = canonical && strcmp(root, canonical) == 0 && strcmp(root, "/") != 0;
    free(canonical);
    if (!valid) return false;
    int count = snprintf(out, PATH_MAX, "%s%s", root, identity);
    if (count < 0 || count >= PATH_MAX) return false;
    canonical = realpath(out, NULL);
    valid = canonical && strcmp(out, canonical) == 0;
    free(canonical);
    return valid;
}
