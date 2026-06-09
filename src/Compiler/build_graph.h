// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "Compiler/compiler_context.h"
#include "Compiler/build_manifest.h"
#include "Extensions/extension_profile.h"
#include "Syntax/target_layout.h"

typedef struct FisicsBuildGraphSourceOptions {
    const char* outputPath;
    const char* inputPath;
    const char* outputObject;
    const char* targetTriple;
    const char* dataLayout;
    const char* const* includePaths;
    size_t includePathCount;
    const char* const* macroDefines;
    size_t macroDefineCount;
    const char* const* forcedIncludes;
    size_t forcedIncludeCount;
    CCDialect dialect;
    CCCompatFeatures compatFeatures;
    FisicsOverlayFeatures overlayFeatures;
    bool compileOnly;
    bool enableCodegen;
    bool partial;
    bool fatal;
} FisicsBuildGraphSourceOptions;

typedef struct FisicsBuildGraphManifestOptions {
    const char* outputPath;
    const FisicsBuildManifest* manifest;
    bool dryRun;
    bool partial;
    bool fatal;
} FisicsBuildGraphManifestOptions;

char* fisics_build_graph_derive_object_path(const char* cPath);

bool fisics_build_graph_write_source_json(const FisicsBuildGraphSourceOptions* options,
                                          const CompilerContext* ctx);

bool fisics_build_graph_write_manifest_dry_run_json(
    const FisicsBuildGraphManifestOptions* options);

bool fisics_build_graph_write_compile_commands_json(
    const char* outputPath,
    const FisicsBuildManifest* manifest);
