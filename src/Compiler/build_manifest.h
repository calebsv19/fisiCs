// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef struct FisicsBuildManifestStringList {
    char** items;
    size_t count;
} FisicsBuildManifestStringList;

typedef struct FisicsBuildManifestDefaults {
    char* standard;
    FisicsBuildManifestStringList includeDirs;
    FisicsBuildManifestStringList defines;
    FisicsBuildManifestStringList overlays;
} FisicsBuildManifestDefaults;

typedef struct FisicsBuildManifestTranslationUnit {
    char* source;
    char* object;
    char* resolvedSource;
    char* resolvedObject;
} FisicsBuildManifestTranslationUnit;

typedef struct FisicsBuildManifestLink {
    char* output;
    char* resolvedOutput;
    FisicsBuildManifestStringList libraries;
    FisicsBuildManifestStringList libraryDirs;
    FisicsBuildManifestStringList args;
} FisicsBuildManifestLink;

typedef struct FisicsBuildManifest {
    char* schema;
    int version;
    char* name;
    char* root;
    char* resolvedRoot;
    char* buildDir;
    FisicsBuildManifestDefaults defaults;
    FisicsBuildManifestTranslationUnit* translationUnits;
    size_t translationUnitCount;
    FisicsBuildManifestLink link;
} FisicsBuildManifest;

typedef struct FisicsBuildManifestDiagnostic {
    char message[256];
} FisicsBuildManifestDiagnostic;

bool fisics_build_manifest_load_file(const char* path,
                                     FisicsBuildManifest* out,
                                     FisicsBuildManifestDiagnostic* diag);

void fisics_build_manifest_free(FisicsBuildManifest* manifest);
