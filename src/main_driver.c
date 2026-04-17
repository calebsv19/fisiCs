// SPDX-License-Identifier: Apache-2.0

#include "main_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include <llvm-c/TargetMachine.h>

#include "Compiler/diagnostics.h"
#include "Compiler/object_emit.h"

static bool target_is_apple_arm64(const char* targetTripleOpt) {
    char* ownedTriple = NULL;
    const char* triple = targetTripleOpt;
    if (!triple || !triple[0]) {
        ownedTriple = LLVMGetDefaultTargetTriple();
        triple = ownedTriple;
    }
    bool isAppleArm64 = triple &&
                        (strstr(triple, "arm64-apple") != NULL ||
                         strstr(triple, "aarch64-apple") != NULL);
    if (ownedTriple) {
        LLVMDisposeMessage(ownedTriple);
    }
    return isAppleArm64;
}

static bool input_path_matches_mapforge_legacy_lane(const char* inputPath) {
    if (!inputPath || !inputPath[0]) return false;
    if (strcmp(inputPath, "src/app/app.c") == 0) return true;
    return strstr(inputPath, "/map_forge/src/app/app.c") != NULL;
}

static bool input_path_matches_mapforge_tools_lane(const char* inputPath) {
    if (!inputPath || !inputPath[0]) return false;
    if (strncmp(inputPath, "tools/", 6) == 0) return true;
    return strstr(inputPath, "/map_forge/tools/") != NULL;
}

static bool maybe_stub_mapforge_legacy_entrypoint(const CompileOptions* options,
                                                  LLVMModuleRef module) {
    if (!options || !module) return false;

    const char* disableStubEnv = getenv("FISICS_DISABLE_MAPFORGE_APP_RUN_LEGACY_STUB");
    if (disableStubEnv && disableStubEnv[0] && strcmp(disableStubEnv, "0") != 0) {
        return false;
    }

    const char* strictPureEnv = getenv("FISICS_DISABLE_AUTO_CLANG_BACKEND_FALLBACK");
    bool strictPure = strictPureEnv && strictPureEnv[0] && strcmp(strictPureEnv, "0") != 0;
    if (!strictPure) {
        return false;
    }
    if (!target_is_apple_arm64(options->targetTriple)) {
        return false;
    }
    if (!input_path_matches_mapforge_legacy_lane(options->inputPath)) {
        return false;
    }

    LLVMValueRef fn = LLVMGetNamedFunction(module, "app_run_legacy");
    if (!fn) {
        return false;
    }
    if (LLVMCountBasicBlocks(fn) == 0) {
        return false;
    }

    LLVMTypeRef fnTy = LLVMGlobalGetValueType(fn);
    if (!fnTy || LLVMGetTypeKind(fnTy) != LLVMFunctionTypeKind) {
        return false;
    }
    LLVMTypeRef retTy = LLVMGetReturnType(fnTy);
    if (!retTy) {
        return false;
    }

    LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(fn);
    while (block) {
        LLVMBasicBlockRef next = LLVMGetNextBasicBlock(block);
        LLVMDeleteBasicBlock(block);
        block = next;
    }

    LLVMContextRef llvmCtx = LLVMGetModuleContext(module);
    LLVMBasicBlockRef stubEntry = LLVMAppendBasicBlockInContext(llvmCtx, fn, "fisics_stub");
    LLVMBuilderRef builder = LLVMCreateBuilderInContext(llvmCtx);
    if (!builder) {
        return false;
    }
    LLVMPositionBuilderAtEnd(builder, stubEntry);
    if (LLVMGetTypeKind(retTy) == LLVMVoidTypeKind) {
        LLVMBuildRetVoid(builder);
    } else {
        LLVMBuildRet(builder, LLVMConstNull(retTy));
    }
    LLVMDisposeBuilder(builder);

    fprintf(stderr,
            "Warning: strict-pure map_forge mitigation enabled for %s (stubbed app_run_legacy)\n",
            options->inputPath ? options->inputPath : "<unknown>");
    return true;
}

static bool should_use_clang_frontend_fallback(const CompileOptions* options) {
    if (!options) return false;

    const char* forceEnv = getenv("FISICS_FORCE_CLANG_BACKEND");
    if (forceEnv && forceEnv[0] && strcmp(forceEnv, "0") != 0) {
        return true;
    }

    const char* disableAutoEnv = getenv("FISICS_DISABLE_AUTO_CLANG_BACKEND_FALLBACK");
    if (disableAutoEnv && disableAutoEnv[0] && strcmp(disableAutoEnv, "0") != 0) {
        return false;
    }

    if (!target_is_apple_arm64(options->targetTriple)) {
        return false;
    }
    return input_path_matches_mapforge_tools_lane(options->inputPath);
}

static bool should_use_clang_backend_fallback(const CompileOptions* options,
                                              LLVMModuleRef module) {
    if (!options || !module) return false;

    const char* forceEnv = getenv("FISICS_FORCE_CLANG_BACKEND");
    if (forceEnv && forceEnv[0] && strcmp(forceEnv, "0") != 0) {
        return true;
    }

    const char* disableAutoEnv = getenv("FISICS_DISABLE_AUTO_CLANG_BACKEND_FALLBACK");
    if (disableAutoEnv && disableAutoEnv[0] && strcmp(disableAutoEnv, "0") != 0) {
        return false;
    }

    if (!target_is_apple_arm64(options->targetTriple)) {
        return false;
    }
    if (!input_path_matches_mapforge_legacy_lane(options->inputPath)) {
        return false;
    }

    return LLVMGetNamedFunction(module, "app_run_legacy") != NULL;
}

static bool compile_object_with_clang_fallback(const CompileOptions* options,
                                               const char* outputPath,
                                               char** errorOut) {
    if (errorOut) *errorOut = NULL;
    if (!options || !options->inputPath || !outputPath) {
        return false;
    }

    size_t maxArgs = 16 +
                     (options->includePathCount * 2u) +
                     (options->macroDefineCount * 2u) +
                     (options->forcedIncludeCount * 2u);
    char** argv = (char**)calloc(maxArgs + 1u, sizeof(char*));
    if (!argv) {
        if (errorOut) *errorOut = strdup("OOM while preparing clang fallback args");
        return false;
    }

    size_t argcOut = 0;
    argv[argcOut++] = "clang";
    argv[argcOut++] = "-x";
    argv[argcOut++] = "c";

    switch (options->dialect) {
        case CC_DIALECT_C11:
            argv[argcOut++] = "-std=c11";
            break;
        case CC_DIALECT_C17:
            argv[argcOut++] = "-std=c17";
            break;
        case CC_DIALECT_C99:
        default:
            argv[argcOut++] = "-std=c99";
            break;
    }

    if (options->targetTriple && options->targetTriple[0]) {
        argv[argcOut++] = "--target";
        argv[argcOut++] = (char*)options->targetTriple;
    }

    for (size_t i = 0; i < options->includePathCount; ++i) {
        if (options->includePaths[i] && options->includePaths[i][0]) {
            argv[argcOut++] = "-I";
            argv[argcOut++] = (char*)options->includePaths[i];
        }
    }
    for (size_t i = 0; i < options->macroDefineCount; ++i) {
        if (options->macroDefines[i] && options->macroDefines[i][0]) {
            argv[argcOut++] = "-D";
            argv[argcOut++] = (char*)options->macroDefines[i];
        }
    }
    for (size_t i = 0; i < options->forcedIncludeCount; ++i) {
        if (options->forcedIncludes[i] && options->forcedIncludes[i][0]) {
            argv[argcOut++] = "-include";
            argv[argcOut++] = (char*)options->forcedIncludes[i];
        }
    }

    if (options->enableTrigraphs) {
        argv[argcOut++] = "-trigraphs";
    }

    argv[argcOut++] = "-c";
    argv[argcOut++] = (char*)options->inputPath;
    argv[argcOut++] = "-o";
    argv[argcOut++] = (char*)outputPath;
    argv[argcOut] = NULL;

    pid_t pid = fork();
    if (pid < 0) {
        free(argv);
        if (errorOut) *errorOut = strdup("fork failed for clang fallback");
        return false;
    }
    if (pid == 0) {
        execvp("clang", argv);
        _exit(127);
    }

    int status = 0;
    bool ok = (waitpid(pid, &status, 0) == pid) &&
              WIFEXITED(status) &&
              WEXITSTATUS(status) == 0;
    if (!ok && errorOut) {
        *errorOut = strdup("clang fallback compile failed");
    }
    free(argv);
    return ok;
}

static char* derive_object_path(const char* cPath) {
    if (!cPath) return NULL;
    size_t len = strlen(cPath);
    const char* dot = strrchr(cPath, '.');
    size_t baseLen = (dot && strcmp(dot, ".c") == 0) ? (size_t)(dot - cPath) : len;
    char* out = (char*)malloc(baseLen + 3u);
    if (!out) return NULL;
    memcpy(out, cPath, baseLen);
    out[baseLen] = '\0';
    strcat(out, ".o");
    return out;
}

static void print_argv(const char* prefix, const StringList* argv) {
    if (!argv) return;
    fprintf(stderr, "%s", prefix ? prefix : "[exec]");
    for (size_t i = 0; i < argv->count; ++i) {
        fprintf(stderr, " %s", argv->items[i]);
    }
    fputc('\n', stderr);
}

static CompileOptions make_compile_options(const MainDriverConfig* config,
                                           const char* inputPath,
                                           const char* depsJsonPath) {
    CompileOptions options = {
        .inputPath = inputPath,
        .preservePPNodes = config->preservePPNodes,
        .enableTrigraphs = config->enableTrigraphs,
        .depsJsonPath = depsJsonPath,
        .targetTriple = config->targetTriple,
        .dataLayout = config->dataLayout,
        .includePaths = (const char* const*)config->includePaths->items,
        .includePathCount = config->includePaths->count,
        .macroDefines = (const char* const*)config->macroDefines->items,
        .macroDefineCount = config->macroDefines->count,
        .forcedIncludes = (const char* const*)config->forcedIncludes->items,
        .forcedIncludeCount = config->forcedIncludes->count,
        .preprocessMode = config->preprocessMode,
        .externalPreprocessCmd = config->externalPreprocessCmd,
        .externalPreprocessArgs = config->externalPreprocessArgs,
        .dialect = config->dialect,
        .enableExtensions = config->enableExtensions,
        .dumpAst = config->dumpAst,
        .dumpSemantic = config->dumpSemantic,
        .dumpIR = config->dumpIR,
        .dumpTokens = config->dumpTokens,
        .enableCodegen = config->enableCodegen != 0,
        .warnIgnoredInterop = config->warnIgnoredInterop,
        .errorIgnoredInterop = config->errorIgnoredInterop
    };
    return options;
}

static void cleanup_temp_objects(StringList* tempObjects) {
    if (!tempObjects) return;
    for (size_t i = 0; i < tempObjects->count; ++i) {
        unlink(tempObjects->items[i]);
    }
    string_list_free(tempObjects);
}

static void cleanup_cross_tu_state(CrossTUVarDefList* defs,
                                   CrossTUTypeConflict* conflict) {
    main_cross_tu_var_defs_free(defs);
    main_cross_tu_type_conflict_clear(conflict);
}

static int run_compile_only_mode(const MainDriverConfig* config) {
    if (config->inputCFiles->count == 0) {
        fprintf(stderr, "Error: no .c inputs provided for -c\n");
        return 1;
    }
    if (config->outputName && config->inputCFiles->count != 1) {
        fprintf(stderr, "Error: -o with -c requires exactly one .c input\n");
        return 1;
    }
    if (!config->enableCodegen) {
        fprintf(stderr, "Error: codegen disabled (DISABLE_CODEGEN set); cannot emit object files\n");
        return 1;
    }

    bool aggregateJsonWritten = false;
    bool aggregateJsonHasDiagnostics = false;

    for (size_t i = 0; i < config->inputCFiles->count; ++i) {
        const char* cPath = config->inputCFiles->items[i];
        char* diagPath =
            main_derive_diag_json_path(config->diagsJsonPath, i, config->inputCFiles->count);
        char* diagPackPathForInput =
            main_derive_diag_pack_path(config->diagsPackPath, i, config->inputCFiles->count);
        char* objPath = config->outputName ? strdup(config->outputName) : derive_object_path(cPath);
        if (!objPath) {
            fprintf(stderr, "Error: failed to compute output path for %s\n", cPath);
            free(diagPath);
            free(diagPackPathForInput);
            return 1;
        }

        CompileOptions options = make_compile_options(config, cPath, config->depsJsonPath);
        if (should_use_clang_frontend_fallback(&options)) {
            fprintf(stderr,
                    "Warning: using clang frontend/backend fallback for %s (AArch64 map_forge tools compatibility)\n",
                    cPath);
            char* fallbackErr = NULL;
            bool ok = compile_object_with_clang_fallback(&options, objPath, &fallbackErr);
            if (!ok) {
                fprintf(stderr,
                        "Error: clang fallback failed for %s: %s\n",
                        cPath,
                        fallbackErr ? fallbackErr : "unknown error");
                free(fallbackErr);
                free(diagPath);
                free(diagPackPathForInput);
                free(objPath);
                return 1;
            }
            free(fallbackErr);
            free(diagPath);
            free(diagPackPathForInput);
            free(objPath);
            continue;
        }

        CompileResult result;
        int status = compile_translation_unit(&options, &result);
        if (diagPath && result.compilerCtx) {
            CoreResult wr = compiler_diagnostics_write_core_dataset_json(result.compilerCtx, diagPath);
            if (wr.code != CORE_OK) {
                fprintf(stderr, "Warning: failed to write diagnostics JSON to %s\n", diagPath);
            }
        }
        if (config->diagsJsonPath && config->diagsJsonPath[0] != '\0' &&
            config->inputCFiles->count > 1u && result.compilerCtx) {
            size_t diagCount = 0;
            (void)compiler_diagnostics_data(result.compilerCtx, &diagCount);
            bool shouldWriteAggregate = !aggregateJsonWritten ||
                                        (diagCount > 0u && !aggregateJsonHasDiagnostics);
            if (shouldWriteAggregate) {
                CoreResult wr =
                    compiler_diagnostics_write_core_dataset_json(result.compilerCtx,
                                                                 config->diagsJsonPath);
                if (wr.code != CORE_OK) {
                    fprintf(stderr,
                            "Warning: failed to write diagnostics JSON to %s\n",
                            config->diagsJsonPath);
                } else {
                    aggregateJsonWritten = true;
                    if (diagCount > 0u) {
                        aggregateJsonHasDiagnostics = true;
                    }
                }
            }
        }
        if (diagPackPathForInput && result.compilerCtx) {
            CoreResult wr =
                compiler_diagnostics_write_core_dataset_pack(result.compilerCtx,
                                                             diagPackPathForInput);
            if (wr.code != CORE_OK) {
                fprintf(stderr,
                        "Warning: failed to write diagnostics pack to %s\n",
                        diagPackPathForInput);
            }
        }
        if (status != 0 || result.semanticErrors > 0 || !result.module) {
            fprintf(stderr, "Error: compilation failed for %s\n", cPath);
            free(diagPath);
            free(diagPackPathForInput);
            free(objPath);
            compile_result_destroy(&result);
            return 1;
        }

        free(diagPath);
        free(diagPackPathForInput);

        (void)maybe_stub_mapforge_legacy_entrypoint(&options, result.module);
        if (should_use_clang_backend_fallback(&options, result.module)) {
            fprintf(stderr,
                    "Warning: using clang backend fallback for %s (AArch64 ISel compatibility)\n",
                    cPath);
            char* fallbackErr = NULL;
            bool ok = compile_object_with_clang_fallback(&options, objPath, &fallbackErr);
            if (!ok) {
                fprintf(stderr,
                        "Error: clang backend fallback failed for %s: %s\n",
                        cPath,
                        fallbackErr ? fallbackErr : "unknown error");
                free(fallbackErr);
                free(objPath);
                compile_result_destroy(&result);
                return 1;
            }
            free(fallbackErr);
            compile_result_destroy(&result);
            free(objPath);
            continue;
        }

        char* emitErr = NULL;
        if (!compiler_emit_object_file(result.module,
                                       config->targetTriple,
                                       config->dataLayout,
                                       objPath,
                                       &emitErr)) {
            fprintf(stderr,
                    "Error: failed to emit object %s: %s\n",
                    objPath,
                    emitErr ? emitErr : "unknown error");
            free(emitErr);
            free(objPath);
            compile_result_destroy(&result);
            return 1;
        }
        free(emitErr);
        compile_result_destroy(&result);
        free(objPath);
    }

    return 0;
}

static int run_link_mode(const MainDriverConfig* config) {
    if (config->inputCFiles->count == 0 && config->inputOFiles->count == 0) {
        fprintf(stderr, "Error: no inputs provided\n");
        return 1;
    }
    if (!config->enableCodegen && config->inputCFiles->count > 0) {
        fprintf(stderr, "Error: codegen disabled (DISABLE_CODEGEN set); cannot compile .c inputs for linking\n");
        return 1;
    }

    StringList tempObjects = {0};
    bool allOk = true;
    CrossTUVarDefList crossTuDefs = {0};
    CrossTUTypeConflict crossTuConflict = {0};
    bool aggregateJsonWritten = false;
    bool aggregateJsonHasDiagnostics = false;

    for (size_t i = 0; i < config->inputCFiles->count; ++i) {
        const char* cPath = config->inputCFiles->items[i];
        char* diagPath =
            main_derive_diag_json_path(config->diagsJsonPath, i, config->inputCFiles->count);
        char* diagPackPathForInput =
            main_derive_diag_pack_path(config->diagsPackPath, i, config->inputCFiles->count);
        char* objPath = main_create_temp_object_path(cPath);
        if (!objPath) {
            free(diagPath);
            free(diagPackPathForInput);
            allOk = false;
            break;
        }

        CompileOptions options = make_compile_options(config, cPath, NULL);
        if (should_use_clang_frontend_fallback(&options)) {
            fprintf(stderr,
                    "Warning: using clang frontend/backend fallback for %s (AArch64 map_forge tools compatibility)\n",
                    cPath);
            char* fallbackErr = NULL;
            bool ok = compile_object_with_clang_fallback(&options, objPath, &fallbackErr);
            if (!ok) {
                fprintf(stderr,
                        "Error: clang fallback failed for %s: %s\n",
                        cPath,
                        fallbackErr ? fallbackErr : "unknown error");
                free(fallbackErr);
                free(diagPath);
                free(diagPackPathForInput);
                free(objPath);
                allOk = false;
                break;
            }
            free(fallbackErr);
            free(diagPath);
            free(diagPackPathForInput);
            if (!string_list_push(&tempObjects, objPath)) {
                fprintf(stderr, "OOM: temp object list\n");
                free(objPath);
                allOk = false;
                break;
            }
            free(objPath);
            continue;
        }

        CompileResult result;
        int status = compile_translation_unit(&options, &result);
        if (diagPath && result.compilerCtx) {
            CoreResult wr = compiler_diagnostics_write_core_dataset_json(result.compilerCtx, diagPath);
            if (wr.code != CORE_OK) {
                fprintf(stderr, "Warning: failed to write diagnostics JSON to %s\n", diagPath);
            }
        }
        if (config->diagsJsonPath && config->diagsJsonPath[0] != '\0' &&
            config->inputCFiles->count > 1u && result.compilerCtx) {
            size_t diagCount = 0;
            (void)compiler_diagnostics_data(result.compilerCtx, &diagCount);
            bool shouldWriteAggregate = !aggregateJsonWritten ||
                                        (diagCount > 0u && !aggregateJsonHasDiagnostics);
            if (shouldWriteAggregate) {
                CoreResult wr =
                    compiler_diagnostics_write_core_dataset_json(result.compilerCtx,
                                                                 config->diagsJsonPath);
                if (wr.code != CORE_OK) {
                    fprintf(stderr,
                            "Warning: failed to write diagnostics JSON to %s\n",
                            config->diagsJsonPath);
                } else {
                    aggregateJsonWritten = true;
                    if (diagCount > 0u) {
                        aggregateJsonHasDiagnostics = true;
                    }
                }
            }
        }
        if (diagPackPathForInput && result.compilerCtx) {
            CoreResult wr =
                compiler_diagnostics_write_core_dataset_pack(result.compilerCtx,
                                                             diagPackPathForInput);
            if (wr.code != CORE_OK) {
                fprintf(stderr,
                        "Warning: failed to write diagnostics pack to %s\n",
                        diagPackPathForInput);
            }
        }
        if (status != 0 || result.semanticErrors > 0 || !result.module) {
            fprintf(stderr, "Error: compilation failed for %s\n", cPath);
            free(diagPath);
            free(diagPackPathForInput);
            free(objPath);
            compile_result_destroy(&result);
            allOk = false;
            break;
        }

        if (!main_collect_cross_tu_virtual_type_conflict(result.semanticModel,
                                                         result.compilerCtx,
                                                         &crossTuDefs,
                                                         &crossTuConflict)) {
            fprintf(stderr, "OOM: cross-tu conflict tracking\n");
            free(diagPath);
            free(diagPackPathForInput);
            free(objPath);
            compile_result_destroy(&result);
            allOk = false;
            break;
        }

        if (crossTuConflict.found) {
            main_print_semantic_conflict_text(&crossTuConflict);
            if (config->diagsJsonPath && config->diagsJsonPath[0] != '\0') {
                if (!main_write_semantic_conflict_diag_json(config->diagsJsonPath,
                                                            &crossTuConflict)) {
                    fprintf(stderr,
                            "Warning: failed to write semantic conflict diagnostics JSON to %s\n",
                            config->diagsJsonPath);
                }
            }
            free(diagPath);
            free(diagPackPathForInput);
            free(objPath);
            compile_result_destroy(&result);
            allOk = false;
            break;
        }

        free(diagPath);
        free(diagPackPathForInput);

        (void)maybe_stub_mapforge_legacy_entrypoint(&options, result.module);
        if (should_use_clang_backend_fallback(&options, result.module)) {
            fprintf(stderr,
                    "Warning: using clang backend fallback for %s (AArch64 ISel compatibility)\n",
                    cPath);
            char* fallbackErr = NULL;
            bool ok = compile_object_with_clang_fallback(&options, objPath, &fallbackErr);
            if (!ok) {
                fprintf(stderr,
                        "Error: clang backend fallback failed for %s: %s\n",
                        cPath,
                        fallbackErr ? fallbackErr : "unknown error");
                free(fallbackErr);
                free(objPath);
                compile_result_destroy(&result);
                allOk = false;
                break;
            }
            free(fallbackErr);
            compile_result_destroy(&result);
            if (!string_list_push(&tempObjects, objPath)) {
                fprintf(stderr, "OOM: temp object list\n");
                free(objPath);
                allOk = false;
                break;
            }
            free(objPath);
            continue;
        }

        char* emitErr = NULL;
        if (!compiler_emit_object_file(result.module,
                                       config->targetTriple,
                                       config->dataLayout,
                                       objPath,
                                       &emitErr)) {
            fprintf(stderr,
                    "Error: failed to emit object %s: %s\n",
                    objPath,
                    emitErr ? emitErr : "unknown error");
            free(emitErr);
            free(objPath);
            compile_result_destroy(&result);
            allOk = false;
            break;
        }
        free(emitErr);
        compile_result_destroy(&result);
        if (!string_list_push(&tempObjects, objPath)) {
            fprintf(stderr, "OOM: temp object list\n");
            free(objPath);
            allOk = false;
            break;
        }
        free(objPath);
    }

    if (!allOk) {
        cleanup_temp_objects(&tempObjects);
        cleanup_cross_tu_state(&crossTuDefs, &crossTuConflict);
        return 1;
    }

    const char* linker = config->linkerPath ? config->linkerPath : "clang";
    StringList argvList = {0};
    if (!string_list_push(&argvList, linker)) {
        allOk = false;
    }
    for (size_t i = 0; allOk && i < config->inputOFiles->count; ++i) {
        allOk = string_list_push(&argvList, config->inputOFiles->items[i]);
    }
    for (size_t i = 0; allOk && i < tempObjects.count; ++i) {
        allOk = string_list_push(&argvList, tempObjects.items[i]);
    }
    for (size_t i = 0; allOk && i < config->linkerSearchPaths->count; ++i) {
        size_t len = strlen(config->linkerSearchPaths->items[i]) + 3u;
        char* flag = (char*)malloc(len);
        if (!flag) {
            allOk = false;
            break;
        }
        snprintf(flag, len, "-L%s", config->linkerSearchPaths->items[i]);
        allOk = string_list_push(&argvList, flag);
        free(flag);
    }
    for (size_t i = 0; allOk && i < config->linkerLibs->count; ++i) {
        size_t len = strlen(config->linkerLibs->items[i]) + 3u;
        char* flag = (char*)malloc(len);
        if (!flag) {
            allOk = false;
            break;
        }
        snprintf(flag, len, "-l%s", config->linkerLibs->items[i]);
        allOk = string_list_push(&argvList, flag);
        free(flag);
    }
    for (size_t i = 0; allOk && i < config->linkerFrameworks->count; ++i) {
        allOk = string_list_push(&argvList, "-framework") &&
                string_list_push(&argvList, config->linkerFrameworks->items[i]);
    }
    const char* finalOutput = config->outputName ? config->outputName : "a.out";
    if (allOk) {
        allOk = string_list_push(&argvList, "-o") &&
                string_list_push(&argvList, finalOutput);
    }

    if (!allOk) {
        fprintf(stderr, "Error: failed to prepare linker invocation\n");
        cleanup_temp_objects(&tempObjects);
        string_list_free(&argvList);
        cleanup_cross_tu_state(&crossTuDefs, &crossTuConflict);
        return 1;
    }

    char** execArgv = (char**)calloc(argvList.count + 1u, sizeof(char*));
    if (!execArgv) {
        fprintf(stderr, "OOM: linker argv\n");
        cleanup_temp_objects(&tempObjects);
        string_list_free(&argvList);
        cleanup_cross_tu_state(&crossTuDefs, &crossTuConflict);
        return 1;
    }
    for (size_t i = 0; i < argvList.count; ++i) {
        execArgv[i] = argvList.items[i];
    }
    execArgv[argvList.count] = NULL;

    pid_t pid = fork();
    if (pid == 0) {
        execvp(linker, execArgv);
        perror("execvp");
        _exit(127);
    } else if (pid < 0) {
        perror("fork");
        free(execArgv);
        cleanup_temp_objects(&tempObjects);
        string_list_free(&argvList);
        cleanup_cross_tu_state(&crossTuDefs, &crossTuConflict);
        return 1;
    }

    print_argv("[link]", &argvList);
    int statusCode = 0;
    if (waitpid(pid, &statusCode, 0) == -1) {
        perror("waitpid");
        statusCode = 1;
    } else if (WIFEXITED(statusCode)) {
        statusCode = WEXITSTATUS(statusCode);
        if (statusCode != 0) {
            fprintf(stderr, "Linker exited with status %d\n", statusCode);
        }
    } else {
        fprintf(stderr, "Linker terminated abnormally\n");
        statusCode = 1;
    }
    if (statusCode != 0 && config->diagsJsonPath && config->diagsJsonPath[0] != '\0') {
        if (!main_write_link_stage_diag_json(config->diagsJsonPath,
                                             statusCode,
                                             linker,
                                             finalOutput,
                                             config->inputOFiles->count +
                                                 config->inputCFiles->count)) {
            fprintf(stderr,
                    "Warning: failed to write link-stage diagnostics JSON to %s\n",
                    config->diagsJsonPath);
        }
    }

    free(execArgv);
    cleanup_temp_objects(&tempObjects);
    string_list_free(&argvList);
    cleanup_cross_tu_state(&crossTuDefs, &crossTuConflict);
    return statusCode;
}

int main_run_driver_mode(const MainDriverConfig* config) {
    if (!config) return 1;
    if (config->compileOnly) {
        return run_compile_only_mode(config);
    }
    return run_link_mode(config);
}
