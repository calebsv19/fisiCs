#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "Compiler/build_manifest.h"

static int load_manifest(const char* path, FisicsBuildManifest* out,
                         FisicsBuildManifestDiagnostic* diag) {
    memset(out, 0, sizeof(*out));
    memset(diag, 0, sizeof(*diag));
    return fisics_build_manifest_load_file(path, out, diag) ? 1 : 0;
}

static int same_logical_manifest(const FisicsBuildManifest* a,
                                 const FisicsBuildManifest* b) {
    if (!a || !b || a->translationUnitCount != b->translationUnitCount) return 0;
    if (strcmp(a->schema, b->schema) != 0 || a->version != b->version) return 0;
    if (strcmp(a->name, b->name) != 0) return 0;
    if (strcmp(a->defaults.standard, b->defaults.standard) != 0) return 0;
    if (a->defaults.includeDirs.count != b->defaults.includeDirs.count ||
        a->defaults.defines.count != b->defaults.defines.count ||
        a->defaults.overlays.count != b->defaults.overlays.count) return 0;
    for (size_t i = 0; i < a->translationUnitCount; ++i) {
        if (strcmp(a->translationUnits[i].source, b->translationUnits[i].source) != 0 ||
            strcmp(a->translationUnits[i].object, b->translationUnits[i].object) != 0) return 0;
    }
    return strcmp(a->link.output, b->link.output) == 0;
}

static int write_summary(const FisicsBuildManifest* manifest) {
    if (mkdir("artifacts", 0700) != 0 && errno != EEXIST) return 0;
    FILE* fp = fopen("artifacts/manifest_summary.txt", "wb");
    if (!fp) return 0;
    int ok = fprintf(fp,
                     "schema=%s\nversion=%d\nname=%s\nstandard=%s\n"
                     "translation_units=%llu\ninclude_dirs=%llu\ndefines=%llu\n"
                     "overlays=%llu\nlink_output=%s\n",
                     manifest->schema,
                     manifest->version,
                     manifest->name,
                     manifest->defaults.standard,
                     (unsigned long long)manifest->translationUnitCount,
                     (unsigned long long)manifest->defaults.includeDirs.count,
                     (unsigned long long)manifest->defaults.defines.count,
                     (unsigned long long)manifest->defaults.overlays.count,
                     manifest->link.output) > 0;
    return fclose(fp) == 0 && ok;
}

int main(void) {
    const char* scenario = getenv("FISICS_STAGE_G_SCENARIO");
    const char* seed = getenv("FISICS_STAGE_G_SEED");
    if (!scenario || !seed || strcmp(scenario, "fisics-self") != 0 ||
        strcmp(seed, "424242") != 0) return 2;
    printf("TRACE|1|boot|scenario=fisics-self|seed=424242|status=ready\n");

    FisicsBuildManifest first;
    FisicsBuildManifest second;
    FisicsBuildManifestDiagnostic diag;
    if (!load_manifest("project/project.json", &first, &diag)) return 3;
    printf("TRACE|1|loaded|name=%s|units=%llu|standard=%s\n",
           first.name,
           (unsigned long long)first.translationUnitCount,
           first.defaults.standard);

    if (!write_summary(&first)) {
        fisics_build_manifest_free(&first);
        return 4;
    }
    printf("TRACE|1|persisted|artifact=manifest_summary.txt|units=%llu|status=written\n",
           (unsigned long long)first.translationUnitCount);

    if (!load_manifest("project/project.json", &second, &diag)) {
        fisics_build_manifest_free(&first);
        return 5;
    }
    if (!same_logical_manifest(&first, &second)) {
        fisics_build_manifest_free(&first);
        fisics_build_manifest_free(&second);
        return 6;
    }
    printf("TRACE|1|reloaded|equal=1|units=%llu|link=%s\n",
           (unsigned long long)second.translationUnitCount,
           second.link.output);
    fisics_build_manifest_free(&first);
    fisics_build_manifest_free(&second);

    FisicsBuildManifest invalid;
    if (load_manifest("project/invalid.json", &invalid, &diag)) {
        fisics_build_manifest_free(&invalid);
        return 7;
    }
    if (!strstr(diag.message, "missing source")) return 8;
    printf("TRACE|1|invalid|accepted=0|kind=missing_source|status=rejected\n");
    printf("TRACE|1|shutdown|loads=3|artifact_count=1|status=clean\n");
    return 0;
}
