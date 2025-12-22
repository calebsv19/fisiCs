#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <limits.h>
#include <execinfo.h>

#include <llvm-c/ErrorHandling.h>

#include "Compiler/pipeline.h"
#include "Compiler/object_emit.h"

typedef struct {
    char** items;
    size_t count;
    size_t capacity;
} StringList;

static void string_list_free(StringList* list) {
    if (!list) return;
    for (size_t i = 0; i < list->count; ++i) {
        free(list->items[i]);
    }
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

static bool string_list_push(StringList* list, const char* value) {
    if (!list || !value) return false;
    if (list->count == list->capacity) {
        size_t newCap = list->capacity ? list->capacity * 2 : 4;
        char** grown = realloc(list->items, newCap * sizeof(char*));
        if (!grown) return false;
        list->items = grown;
        list->capacity = newCap;
    }
    list->items[list->count] = strdup(value);
    if (!list->items[list->count]) return false;
    list->count++;
    return true;
}

static bool has_extension(const char* path, const char* ext) {
    if (!path || !ext) return false;
    size_t pathLen = strlen(path);
    size_t extLen = strlen(ext);
    if (pathLen < extLen) return false;
    return strcmp(path + pathLen - extLen, ext) == 0;
}

static char* derive_object_path(const char* cPath) {
    if (!cPath) return NULL;
    size_t len = strlen(cPath);
    const char* dot = strrchr(cPath, '.');
    size_t baseLen = (dot && strcmp(dot, ".c") == 0) ? (size_t)(dot - cPath) : len;
    char* out = (char*)malloc(baseLen + 3); // ".o" + null
    if (!out) return NULL;
    memcpy(out, cPath, baseLen);
    out[baseLen] = '\0';
    strcat(out, ".o");
    return out;
}

static void llvm_fatal_handler(const char* reason) {
    fprintf(stderr, "LLVM fatal error: %s\n", reason ? reason : "<null>");
    void* frames[64];
    int count = backtrace(frames, 64);
    backtrace_symbols_fd(frames, count, fileno(stderr));
    _exit(1);
}

static char* create_temp_object_path(const char* baseName) {
    (void)baseName;
    char tmpl[] = "/tmp/mycc-XXXXXX";
    int fd = mkstemp(tmpl);
    if (fd == -1) {
        perror("mkstemp");
        return NULL;
    }
    close(fd);
    size_t len = strlen(tmpl);
    char* withExt = malloc(len + 3);
    if (!withExt) {
        unlink(tmpl);
        return NULL;
    }
    snprintf(withExt, len + 3, "%s.o", tmpl);
    // Rename temp file to have .o extension.
    if (rename(tmpl, withExt) != 0) {
        perror("rename");
        unlink(tmpl);
        free(withExt);
        return NULL;
    }
    return withExt;
}

static bool dir_exists(const char* path) {
    if (!path || !*path) return false;
    struct stat st;
    if (stat(path, &st) != 0) return false;
    return S_ISDIR(st.st_mode);
}

static void append_include_dir_if_exists(StringList* list, const char* path) {
    if (!list || !path) return;
    if (dir_exists(path)) {
        string_list_push(list, path);
    }
}

static void print_argv(const char* prefix, StringList* argv) {
    if (!argv) return;
    fprintf(stderr, "%s", prefix ? prefix : "[exec]");
    for (size_t i = 0; i < argv->count; ++i) {
        fprintf(stderr, " %s", argv->items[i]);
    }
    fputc('\n', stderr);
}

static char* detect_sdk_include_from_xcrun(void) {
    FILE* fp = popen("xcrun --show-sdk-path 2>/dev/null", "r");
    if (!fp) return NULL;
    char buffer[PATH_MAX];
    if (!fgets(buffer, sizeof(buffer), fp)) {
        pclose(fp);
        return NULL;
    }
    pclose(fp);
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    }
    if (buffer[0] == '\0') return NULL;

    size_t baseLen = strlen(buffer);
    const char* suffix = "/usr/include";
    size_t suffixLen = strlen(suffix);
    char* path = malloc(baseLen + suffixLen + 1);
    if (!path) return NULL;
    memcpy(path, buffer, baseLen);
    memcpy(path + baseLen, suffix, suffixLen + 1);
    return path;
}

static char* detect_clang_resource_include(void) {
    FILE* fp = popen("clang -print-resource-dir 2>/dev/null", "r");
    if (!fp) return NULL;
    char buffer[PATH_MAX];
    if (!fgets(buffer, sizeof(buffer), fp)) {
        pclose(fp);
        return NULL;
    }
    pclose(fp);
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    }
    if (buffer[0] == '\0') return NULL;

    size_t baseLen = strlen(buffer);
    const char* suffix = "/include";
    size_t suffixLen = strlen(suffix);
    char* path = malloc(baseLen + suffixLen + 1);
    if (!path) return NULL;
    memcpy(path, buffer, baseLen);
    memcpy(path + baseLen, suffix, suffixLen + 1);
    return path;
}

// === Feature Toggles ===
#define ENABLE_LEXER_OUTPUT      0
#define ENABLE_AST_PRINT         1
#define ENABLE_SYNTAX_CHECK      1
#define ENABLE_CODEGEN           1

int main(int argc, char **argv) {
    const char* progressEnv = getenv("FISICS_DEBUG_PROGRESS");
    bool debugProgress = progressEnv && progressEnv[0] && progressEnv[0] != '0';
    if (debugProgress) fprintf(stderr, "[main] start argc=%d\n", argc);
    const char* nanoEnv = getenv("MallocNanoZone");
    if (!nanoEnv) {
        setenv("MallocNanoZone", "0", 0);
    }

    LLVMInstallFatalErrorHandler(llvm_fatal_handler);

    const char *filename = NULL;
    bool preservePPNodes = false;
    const char* depsJsonPath = NULL;
    const char* targetTriple = NULL;
    const char* dataLayout = NULL;
    bool compileOnly = false;
    const char* outputName = NULL;
    const char* linkerPath = NULL;
    bool dumpAst = false;
    bool dumpSemantic = false;
    bool dumpIR = false;
    bool enableTrigraphs = false;
    StringList includePaths = {0};
    StringList inputCFiles = {0};
    StringList inputOFiles = {0};
    StringList linkerSearchPaths = {0};
    StringList linkerLibs = {0};

    // Seed include paths from default list.
    char** defaultIncludePaths = NULL;
    size_t defaultIncludeCount = 0;
    if (!compiler_collect_include_paths(DEFAULT_INCLUDE_PATHS,
                                        &defaultIncludePaths,
                                        &defaultIncludeCount)) {
        fprintf(stderr, "OOM: include paths\n");
        return 1;
    }
    if (debugProgress) fprintf(stderr, "[main] default include count=%zu\n", defaultIncludeCount);
    for (size_t i = 0; i < defaultIncludeCount; ++i) {
        string_list_push(&includePaths, defaultIncludePaths[i]);
    }
    compiler_free_include_paths(defaultIncludePaths, defaultIncludeCount);
    // Optional system include paths (e.g., macOS SDK)
    const char* sysEnv = getenv("SYSTEM_INCLUDE_PATHS");
    if (sysEnv && sysEnv[0]) {
        char** parsed = NULL;
        size_t parsedCount = 0;
        if (compiler_collect_include_paths(sysEnv, &parsed, &parsedCount)) {
            for (size_t i = 0; i < parsedCount; ++i) {
                string_list_push(&includePaths, parsed[i]);
            }
            compiler_free_include_paths(parsed, parsedCount);
        }
    }
    const char* sdkRoot = getenv("SDKROOT");
    const char* enableXcrun = getenv("ENABLE_XCRUN_DETECT");
    bool allowXcrunDetect = enableXcrun && enableXcrun[0] && enableXcrun[0] != '0';
    if (sdkRoot && sdkRoot[0]) {
        char buffer[PATH_MAX];
        snprintf(buffer, sizeof(buffer), "%s/usr/include", sdkRoot);
        append_include_dir_if_exists(&includePaths, buffer);
    } else if (allowXcrunDetect) {
        char* sdkFromXcrun = detect_sdk_include_from_xcrun();
        if (sdkFromXcrun) {
            append_include_dir_if_exists(&includePaths, sdkFromXcrun);
            free(sdkFromXcrun);
        }
    }
    // Always fall back to common macOS SDK/system include locations so we don’t hang on xcrun.
    append_include_dir_if_exists(&includePaths, "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include");
    append_include_dir_if_exists(&includePaths, "/Library/Developer/CommandLineTools/usr/include");
    append_include_dir_if_exists(&includePaths, "/usr/include");
    char* clangResource = detect_clang_resource_include();
    if (clangResource) {
        append_include_dir_if_exists(&includePaths, clangResource);
        free(clangResource);
    }

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--preserve-pp") == 0) {
            preservePPNodes = true;
        } else if (strcmp(argv[i], "--emit-deps-json") == 0 && i + 1 < argc) {
            depsJsonPath = argv[++i];
        } else if (strcmp(argv[i], "--target") == 0 && i + 1 < argc) {
            targetTriple = argv[++i];
        } else if (strcmp(argv[i], "--data-layout") == 0 && i + 1 < argc) {
            dataLayout = argv[++i];
        } else if (strcmp(argv[i], "--dump-ast") == 0) {
            dumpAst = true;
        } else if (strcmp(argv[i], "--dump-sema") == 0) {
            dumpSemantic = true;
        } else if (strcmp(argv[i], "--dump-ir") == 0) {
            dumpIR = true;
        } else if (strcmp(argv[i], "--trigraphs") == 0) {
            enableTrigraphs = true;
        } else if (strcmp(argv[i], "-c") == 0) {
            compileOnly = true;
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            outputName = argv[++i];
        } else if (strncmp(argv[i], "-I", 2) == 0) {
            const char* path = argv[i] + 2;
            if (!*path) {
                if (i + 1 >= argc) { fprintf(stderr, "-I requires a path\n"); goto fail; }
                path = argv[++i];
            }
            if (!string_list_push(&includePaths, path)) { fprintf(stderr, "OOM: include path\n"); goto fail; }
        } else if (strncmp(argv[i], "-L", 2) == 0) {
            const char* path = argv[i] + 2;
            if (!*path) {
                if (i + 1 >= argc) { fprintf(stderr, "-L requires a path\n"); goto fail; }
                path = argv[++i];
            }
            if (!string_list_push(&linkerSearchPaths, path)) { fprintf(stderr, "OOM: -L path\n"); goto fail; }
        } else if (strncmp(argv[i], "-l", 2) == 0) {
            const char* lib = argv[i] + 2;
            if (!*lib) {
                if (i + 1 >= argc) { fprintf(stderr, "-l requires a library name\n"); goto fail; }
                lib = argv[++i];
            }
            if (!string_list_push(&linkerLibs, lib)) { fprintf(stderr, "OOM: -l name\n"); goto fail; }
        } else if (strncmp(argv[i], "--linker=", 9) == 0) {
            linkerPath = argv[i] + 9;
        } else if (argv[i][0] != '-' && !filename) {
            filename = argv[i];
            if (has_extension(filename, ".c")) {
                string_list_push(&inputCFiles, filename);
            } else if (has_extension(filename, ".o")) {
                string_list_push(&inputOFiles, filename);
            } else {
                fprintf(stderr, "Warning: unrecognized input extension for %s\n", filename);
            }
        } else if (argv[i][0] != '-') {
            if (has_extension(argv[i], ".c")) {
                string_list_push(&inputCFiles, argv[i]);
            } else if (has_extension(argv[i], ".o")) {
                string_list_push(&inputOFiles, argv[i]);
            } else {
                fprintf(stderr, "Warning: unrecognized input extension for %s\n", argv[i]);
            }
        }
    }
    if (!filename && inputCFiles.count == 0 && inputOFiles.count == 0) {
        filename = "include/test.txt";
        string_list_push(&inputCFiles, filename);
    }

    int enableCodegen = ENABLE_CODEGEN;
    const char* disableCodegenEnv = getenv("DISABLE_CODEGEN");
    if (disableCodegenEnv && disableCodegenEnv[0] != '\0' && disableCodegenEnv[0] != '0') {
        enableCodegen = 0;
    }

    const char* preserveEnv = getenv("PRESERVE_PP_NODES");
    if (preserveEnv && preserveEnv[0] != '\0' && preserveEnv[0] != '0') {
        preservePPNodes = true;
    }
    const char* triEnv = getenv("ENABLE_TRIGRAPHS");
    if (triEnv && triEnv[0] != '\0' && triEnv[0] != '0') {
        enableTrigraphs = true;
    }

    const char* depsEnv = getenv("EMIT_DEPS_JSON");
    if (!depsJsonPath && depsEnv && depsEnv[0] != '\0') {
        depsJsonPath = depsEnv;
    }

    bool driverMode = compileOnly || outputName || inputOFiles.count > 0 ||
                      linkerSearchPaths.count > 0 || linkerLibs.count > 0 || linkerPath;
    if (driverMode) {
        if (compileOnly) {
            if (inputCFiles.count == 0) {
                fprintf(stderr, "Error: no .c inputs provided for -c\n");
                goto fail;
            }
            if (outputName && inputCFiles.count != 1) {
                fprintf(stderr, "Error: -o with -c requires exactly one .c input\n");
                goto fail;
            }
            if (!enableCodegen) {
                fprintf(stderr, "Error: codegen disabled (DISABLE_CODEGEN set); cannot emit object files\n");
                goto fail;
            }

            for (size_t i = 0; i < inputCFiles.count; ++i) {
                const char* cPath = inputCFiles.items[i];
                char* objPath = NULL;
                if (outputName) {
                    objPath = strdup(outputName);
                } else {
                    objPath = derive_object_path(cPath);
                }
                if (!objPath) {
                    fprintf(stderr, "Error: failed to compute output path for %s\n", cPath);
                    goto fail;
                }

                CompileOptions options = {
                    .inputPath = cPath,
                    .preservePPNodes = preservePPNodes,
                    .enableTrigraphs = enableTrigraphs,
                    .depsJsonPath = depsJsonPath,
                    .targetTriple = targetTriple,
                    .dataLayout = dataLayout,
                    .includePaths = (const char* const*)includePaths.items,
                    .includePathCount = includePaths.count,
                    .dumpAst = dumpAst,
                    .dumpSemantic = dumpSemantic,
                    .dumpIR = dumpIR,
                    .enableCodegen = enableCodegen
                };

                CompileResult result;
                int status = compile_translation_unit(&options, &result);
                if (status != 0 || result.semanticErrors > 0 || !result.module) {
                    fprintf(stderr, "Error: compilation failed for %s\n", cPath);
                    free(objPath);
                    compile_result_destroy(&result);
                    goto fail;
                }

                char* emitErr = NULL;
                if (!compiler_emit_object_file(result.module,
                                               targetTriple,
                                               dataLayout,
                                               objPath,
                                               &emitErr)) {
                    fprintf(stderr, "Error: failed to emit object %s: %s\n",
                            objPath,
                            emitErr ? emitErr : "unknown error");
                    free(emitErr);
                    free(objPath);
                    compile_result_destroy(&result);
                    goto fail;
                }
                free(emitErr);
                compile_result_destroy(&result);
                free(objPath);
            }

            string_list_free(&includePaths);
            string_list_free(&inputCFiles);
            string_list_free(&inputOFiles);
            string_list_free(&linkerSearchPaths);
            string_list_free(&linkerLibs);
            return 0;
        } else {
            if (inputCFiles.count == 0 && inputOFiles.count == 0) {
                fprintf(stderr, "Error: no inputs provided\n");
                goto fail;
            }
            if (!enableCodegen && inputCFiles.count > 0) {
                fprintf(stderr, "Error: codegen disabled (DISABLE_CODEGEN set); cannot compile .c inputs for linking\n");
                goto fail;
            }

            StringList tempObjects = {0};
            bool allOk = true;

            // Step A: compile all .c to temp .o
            for (size_t i = 0; i < inputCFiles.count; ++i) {
                const char* cPath = inputCFiles.items[i];
                char* objPath = create_temp_object_path(cPath);
                if (!objPath) { allOk = false; break; }

                CompileOptions options = {
                    .inputPath = cPath,
                    .preservePPNodes = preservePPNodes,
                    .enableTrigraphs = enableTrigraphs,
                    .depsJsonPath = NULL,
                    .targetTriple = targetTriple,
                    .dataLayout = dataLayout,
                    .includePaths = (const char* const*)includePaths.items,
                    .includePathCount = includePaths.count,
                    .dumpAst = dumpAst,
                    .dumpSemantic = dumpSemantic,
                    .dumpIR = dumpIR,
                    .enableCodegen = enableCodegen
                };

                CompileResult result;
                int status = compile_translation_unit(&options, &result);
                if (status != 0 || result.semanticErrors > 0 || !result.module) {
                    fprintf(stderr, "Error: compilation failed for %s\n", cPath);
                    free(objPath);
                    compile_result_destroy(&result);
                    allOk = false;
                    break;
                }

                char* emitErr = NULL;
                if (!compiler_emit_object_file(result.module,
                                               targetTriple,
                                               dataLayout,
                                               objPath,
                                               &emitErr)) {
                    fprintf(stderr, "Error: failed to emit object %s: %s\n",
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
                for (size_t i = 0; i < tempObjects.count; ++i) {
                    unlink(tempObjects.items[i]);
                }
                string_list_free(&tempObjects);
                goto fail;
            }

            // Step B: build linker argv
            const char* linker = linkerPath ? linkerPath : "clang";
            StringList argvList = {0};
            if (!string_list_push(&argvList, linker)) {
                allOk = false;
            }
            for (size_t i = 0; allOk && i < inputOFiles.count; ++i) {
                allOk = string_list_push(&argvList, inputOFiles.items[i]);
            }
            for (size_t i = 0; allOk && i < tempObjects.count; ++i) {
                allOk = string_list_push(&argvList, tempObjects.items[i]);
            }
            for (size_t i = 0; allOk && i < linkerSearchPaths.count; ++i) {
                size_t len = strlen(linkerSearchPaths.items[i]) + 3;
                char* flag = (char*)malloc(len);
                if (!flag) { allOk = false; break; }
                snprintf(flag, len, "-L%s", linkerSearchPaths.items[i]);
                allOk = string_list_push(&argvList, flag);
                free(flag);
            }
            for (size_t i = 0; allOk && i < linkerLibs.count; ++i) {
                size_t len = strlen(linkerLibs.items[i]) + 3;
                char* flag = (char*)malloc(len);
                if (!flag) { allOk = false; break; }
                snprintf(flag, len, "-l%s", linkerLibs.items[i]);
                allOk = string_list_push(&argvList, flag);
                free(flag);
            }
            const char* finalOutput = outputName ? outputName : "a.out";
            if (allOk) {
                allOk = string_list_push(&argvList, "-o") &&
                        string_list_push(&argvList, finalOutput);
            }

            if (!allOk) {
                fprintf(stderr, "Error: failed to prepare linker invocation\n");
                for (size_t i = 0; i < tempObjects.count; ++i) {
                    unlink(tempObjects.items[i]);
                }
                string_list_free(&tempObjects);
                string_list_free(&argvList);
                goto fail;
            }

            // execvp-style array
            char** execArgv = calloc(argvList.count + 1, sizeof(char*));
            if (!execArgv) {
                fprintf(stderr, "OOM: linker argv\n");
                for (size_t i = 0; i < tempObjects.count; ++i) {
                    unlink(tempObjects.items[i]);
                }
                string_list_free(&tempObjects);
                string_list_free(&argvList);
                goto fail;
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
                for (size_t i = 0; i < tempObjects.count; ++i) {
                    unlink(tempObjects.items[i]);
                }
                string_list_free(&tempObjects);
                string_list_free(&argvList);
                goto fail;
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

            for (size_t i = 0; i < tempObjects.count; ++i) {
                unlink(tempObjects.items[i]);
            }

            free(execArgv);
            string_list_free(&tempObjects);
            string_list_free(&argvList);

            string_list_free(&includePaths);
            string_list_free(&inputCFiles);
            string_list_free(&inputOFiles);
            string_list_free(&linkerSearchPaths);
            string_list_free(&linkerLibs);
            return statusCode;
        }
    }

    const char* inputPath = (inputCFiles.count > 0) ? inputCFiles.items[0] : filename;

    if (debugProgress) fprintf(stderr, "[main] starting compile for %s\n", inputPath ? inputPath : "<null>");
    CompileOptions options = {
        .inputPath = inputPath,
        .preservePPNodes = preservePPNodes,
        .enableTrigraphs = enableTrigraphs,
        .depsJsonPath = depsJsonPath,
        .targetTriple = targetTriple,
        .dataLayout = dataLayout,
        .includePaths = (const char* const*)includePaths.items,
        .includePathCount = includePaths.count,
        .dumpAst = dumpAst || ENABLE_AST_PRINT,
        .dumpSemantic = dumpSemantic || ENABLE_SYNTAX_CHECK,
        .dumpIR = dumpIR || (enableCodegen && ENABLE_CODEGEN),
        .enableCodegen = enableCodegen
    };

    CompileResult result;
    int status = compile_translation_unit(&options, &result);

    compile_result_destroy(&result);
    string_list_free(&includePaths);
    string_list_free(&inputCFiles);
    string_list_free(&inputOFiles);
    string_list_free(&linkerSearchPaths);
    string_list_free(&linkerLibs);
    return status;

fail:
    string_list_free(&includePaths);
    string_list_free(&inputCFiles);
    string_list_free(&inputOFiles);
    string_list_free(&linkerSearchPaths);
    string_list_free(&linkerLibs);
    return 1;
}
