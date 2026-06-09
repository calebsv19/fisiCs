#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "Compiler/build_manifest.h"

static int write_text(const char* path, const char* text) {
    FILE* fp = fopen(path, "wb");
    if (!fp) return 0;
    size_t len = strlen(text);
    int ok = fwrite(text, 1u, len, fp) == len;
    fclose(fp);
    return ok;
}

static void join_path(char* out, size_t cap, const char* a, const char* b) {
    snprintf(out, cap, "%s/%s", a, b);
}

static int make_tree(char* root, size_t rootCap) {
    snprintf(root, rootCap, "%s", "/tmp/fisics_manifest_contract.XXXXXX");
    if (!mkdtemp(root)) return 0;

    char path[512];
    join_path(path, sizeof(path), root, "src");
    if (mkdir(path, 0700) != 0) return 0;
    join_path(path, sizeof(path), root, "build");
    if (mkdir(path, 0700) != 0) return 0;
    join_path(path, sizeof(path), root, "build/fisics");
    if (mkdir(path, 0700) != 0) return 0;
    join_path(path, sizeof(path), root, "include");
    if (mkdir(path, 0700) != 0) return 0;

    join_path(path, sizeof(path), root, "src/main.c");
    if (!write_text(path, "int main(void) { return 0; }\n")) return 0;
    join_path(path, sizeof(path), root, "src/lib.c");
    if (!write_text(path, "int lib(void) { return 1; }\n")) return 0;
    return 1;
}

static int load_ok(const char* path, FisicsBuildManifest* manifest) {
    FisicsBuildManifestDiagnostic diag = {0};
    if (!fisics_build_manifest_load_file(path, manifest, &diag)) {
        fprintf(stderr, "manifest load failed: %s\n", diag.message);
        return 0;
    }
    return 1;
}

static int expect_fail_contains(const char* path, const char* needle) {
    FisicsBuildManifest manifest = {0};
    FisicsBuildManifestDiagnostic diag = {0};
    if (fisics_build_manifest_load_file(path, &manifest, &diag)) {
        fprintf(stderr, "manifest unexpectedly loaded: %s\n", path);
        fisics_build_manifest_free(&manifest);
        return 0;
    }
    if (!strstr(diag.message, needle)) {
        fprintf(stderr, "expected diagnostic containing '%s', got '%s'\n", needle, diag.message);
        return 0;
    }
    return 1;
}

int main(void) {
    char root[256];
    if (!make_tree(root, sizeof(root))) {
        fprintf(stderr, "failed to create manifest fixture tree\n");
        return 1;
    }

    char manifestPath[512];
    join_path(manifestPath, sizeof(manifestPath), root, "project.json");
    if (!write_text(
            manifestPath,
            "{\n"
            "  \"schema\": \"fisiCs.project\",\n"
            "  \"version\": 0,\n"
            "  \"name\": \"sample\",\n"
            "  \"root\": \".\",\n"
            "  \"build_dir\": \"build/fisics\",\n"
            "  \"defaults\": {\n"
            "    \"standard\": \"c99\",\n"
            "    \"include_dirs\": [\"include\"],\n"
            "    \"defines\": [\"FISICS_MANIFEST=1\"],\n"
            "    \"overlays\": [\"physics-units\"]\n"
            "  },\n"
            "  \"translation_units\": [\n"
            "    {\"source\": \"src/main.c\", \"object\": \"build/fisics/main.o\"},\n"
            "    {\"source\": \"src/lib.c\", \"object\": \"build/fisics/lib.o\"}\n"
            "  ],\n"
            "  \"link\": {\n"
            "    \"output\": \"build/fisics/sample\",\n"
            "    \"libraries\": [\"m\"],\n"
            "    \"library_dirs\": [],\n"
            "    \"args\": [\"-Wl,-dead_strip\"]\n"
            "  }\n"
            "}\n")) {
        fprintf(stderr, "failed to write manifest fixture\n");
        return 1;
    }

    FisicsBuildManifest manifest = {0};
    if (!load_ok(manifestPath, &manifest)) return 1;
    if (manifest.translationUnitCount != 2u ||
        strcmp(manifest.defaults.standard, "c99") != 0 ||
        manifest.defaults.includeDirs.count != 1u ||
        manifest.defaults.defines.count != 1u ||
        manifest.defaults.overlays.count != 1u ||
        strcmp(manifest.translationUnits[0].source, "src/main.c") != 0 ||
        !strstr(manifest.translationUnits[0].resolvedSource, "/src/main.c") ||
        !strstr(manifest.translationUnits[0].resolvedObject, "/build/fisics/main.o") ||
        strcmp(manifest.link.output, "build/fisics/sample") != 0 ||
        !strstr(manifest.link.resolvedOutput, "/build/fisics/sample") ||
        manifest.link.libraries.count != 1u ||
        manifest.link.args.count != 1u) {
        fprintf(stderr, "manifest fields did not round-trip as expected\n");
        fisics_build_manifest_free(&manifest);
        return 1;
    }
    fisics_build_manifest_free(&manifest);

    char badPath[512];
    join_path(badPath, sizeof(badPath), root, "missing_source_field.json");
    if (!write_text(
            badPath,
            "{\"schema\":\"fisiCs.project\",\"version\":0,\"name\":\"bad\","
            "\"translation_units\":[{\"object\":\"build/fisics/missing.o\"}]}\n") ||
        !expect_fail_contains(badPath, "missing source")) {
        return 1;
    }

    join_path(badPath, sizeof(badPath), root, "unknown_field.json");
    if (!write_text(
            badPath,
            "{\"schema\":\"fisiCs.project\",\"version\":0,\"name\":\"bad\","
            "\"unknown\":true,\"translation_units\":[{\"source\":\"src/main.c\"}]}\n") ||
        !expect_fail_contains(badPath, "unknown manifest field")) {
        return 1;
    }

    join_path(badPath, sizeof(badPath), root, "bad_source_path.json");
    if (!write_text(
            badPath,
            "{\"schema\":\"fisiCs.project\",\"version\":0,\"name\":\"bad\","
            "\"translation_units\":[{\"source\":\"src/absent.c\"}]}\n") ||
        !expect_fail_contains(badPath, "source does not exist")) {
        return 1;
    }

    return 0;
}
