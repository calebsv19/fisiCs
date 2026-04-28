// SPDX-License-Identifier: Apache-2.0

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <limits.h>
#include <execinfo.h>

#include <llvm-c/ErrorHandling.h>

#include "main_internal.h"
#include "Compiler/pipeline.h"
#include "Syntax/target_layout.h"
#include "Utils/profiler.h"
#include "Utils/utils.h"

static char g_proc_guard_path[PATH_MAX] = {0};
static pid_t g_proc_guard_group_pid = 0;
static int g_proc_guard_timeout_sec = 0;

static void fisics_proc_guard_cleanup(void) {
    if (g_proc_guard_path[0] != '\0') {
        unlink(g_proc_guard_path);
        g_proc_guard_path[0] = '\0';
    }
}

static int parse_nonnegative_int_env_with_default(const char* key, int defaultValue) {
    const char* raw = getenv(key);
    if (!raw || !raw[0]) return defaultValue;
    char* end = NULL;
    long v = strtol(raw, &end, 10);
    if (!end || *end != '\0' || v < 0 || v > 100000) {
        return defaultValue;
    }
    return (int)v;
}

static void cleanup_stale_guard_entries(const char* dirPath) {
    DIR* dir = opendir(dirPath);
    if (!dir) return;
    struct dirent* ent = NULL;
    while ((ent = readdir(dir)) != NULL) {
        if (strncmp(ent->d_name, "pid-", 4) != 0) continue;
        const char* pidPart = ent->d_name + 4;
        char* end = NULL;
        long pid = strtol(pidPart, &end, 10);
        if (!end || *end != '\0' || pid <= 0) continue;
        if (kill((pid_t)pid, 0) == -1 && errno == ESRCH) {
            char filePath[PATH_MAX];
            if (snprintf(filePath, sizeof(filePath), "%s/%s", dirPath, ent->d_name) < (int)sizeof(filePath)) {
                unlink(filePath);
            }
        }
    }
    closedir(dir);
}

static int count_guard_entries(const char* dirPath) {
    int count = 0;
    DIR* dir = opendir(dirPath);
    if (!dir) return 0;
    struct dirent* ent = NULL;
    while ((ent = readdir(dir)) != NULL) {
        if (strncmp(ent->d_name, "pid-", 4) == 0) {
            count++;
        }
    }
    closedir(dir);
    return count;
}

static void fisics_timeout_handler(int signo) {
    (void)signo;
    fisics_proc_guard_cleanup();
    const char msg[] =
        "Error: fisics compile watchdog timed out; aborting hung compiler process.\n";
    (void)write(STDERR_FILENO, msg, sizeof(msg) - 1u);
    if (g_proc_guard_group_pid > 0) {
        (void)kill(-g_proc_guard_group_pid, SIGKILL);
    }
    _exit(124);
}

static void fisics_proc_guard_disarm_timeout(void) {
    if (g_proc_guard_timeout_sec > 0) {
        alarm(0);
    }
}

static void fisics_proc_guard_arm_timeout(void) {
    g_proc_guard_timeout_sec =
        parse_nonnegative_int_env_with_default("FISICS_TIMEOUT_SEC", 180);
    if (g_proc_guard_timeout_sec <= 0) {
        return;
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = fisics_timeout_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGALRM, &sa, NULL) != 0) {
        fprintf(stderr,
                "Warning: failed to install fisics watchdog handler; continuing without timeout.\n");
        g_proc_guard_timeout_sec = 0;
        return;
    }

    if (setpgid(0, 0) == 0 || (errno == EACCES || errno == EPERM)) {
        pid_t groupPid = getpgrp();
        if (groupPid == getpid()) {
            g_proc_guard_group_pid = groupPid;
        }
    }

    alarm((unsigned int)g_proc_guard_timeout_sec);
}

static bool fisics_proc_guard_enter(void) {
    int maxProcs = parse_nonnegative_int_env_with_default("FISICS_MAX_PROCS", 1);
    if (maxProcs <= 0) return true;

    const char* dirPath = "/tmp/fisics_proc_guard";
    if (mkdir(dirPath, 0700) != 0 && errno != EEXIST) {
        fprintf(stderr, "Warning: failed to create process-guard dir %s\n", dirPath);
        return true;
    }

    cleanup_stale_guard_entries(dirPath);
    int running = count_guard_entries(dirPath);
    if (running >= maxProcs) {
        fprintf(stderr,
                "Error: refusing to start fisics (FISICS_MAX_PROCS=%d, currently active=%d)\n",
                maxProcs,
                running);
        fprintf(stderr,
                "Hint: set FISICS_MAX_PROCS higher, or set it to 0 to disable this guard.\n");
        return false;
    }

    pid_t pid = getpid();
    if (snprintf(g_proc_guard_path, sizeof(g_proc_guard_path), "%s/pid-%ld", dirPath, (long)pid) >= (int)sizeof(g_proc_guard_path)) {
        g_proc_guard_path[0] = '\0';
        return true;
    }

    int fd = open(g_proc_guard_path, O_CREAT | O_EXCL | O_WRONLY, 0600);
    if (fd < 0) {
        // Best effort: don't hard-fail if guard file cannot be created.
        g_proc_guard_path[0] = '\0';
        return true;
    }
    char msg[64];
    int len = snprintf(msg, sizeof(msg), "%ld\n", (long)pid);
    if (len > 0) (void)write(fd, msg, (size_t)len);
    close(fd);
    atexit(fisics_proc_guard_cleanup);
    return true;
}

void string_list_free(StringList* list) {
    if (!list) return;
    for (size_t i = 0; i < list->count; ++i) {
        free(list->items[i]);
    }
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

bool string_list_push(StringList* list, const char* value) {
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

static void llvm_fatal_handler(const char* reason) {
    fprintf(stderr, "LLVM fatal error: %s\n", reason ? reason : "<null>");
    void* frames[64];
    int count = backtrace(frames, 64);
    backtrace_symbols_fd(frames, count, fileno(stderr));
    _exit(1);
}

static int llvm_shutdown_and_return(int code) {
    /*
     * LLVM teardown has been intermittently crashing in DebugCounterOwner
     * destruction on process exit (SIGTRAP in macOS malloc free path).
     * Keep process termination stable for harness/repeated runs by avoiding
     * process-exit destructor paths that can trip this crash.
     */
    if (profiler_enabled()) {
        profiler_shutdown();
    }
    fisics_proc_guard_disarm_timeout();
    fisics_proc_guard_cleanup();
    fflush(NULL);
    _Exit(code);
    return code;
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

static bool env_is_enabled(const char* key) {
    const char* value = getenv(key);
    return value && value[0] && value[0] != '0';
}

static bool validate_shim_profile_contract(void) {
    if (!env_is_enabled("FISICS_SHIM_PROFILE_ENFORCE")) {
        return true;
    }

    const char* defaultProfileId = "shim_profile_lang_frontend_shadow_v1";
    const char* defaultProfileVersion = "1";
    const char* profileId = getenv("FISICS_SHIM_PROFILE_ID");
    const char* profileVersion = getenv("FISICS_SHIM_PROFILE_VERSION");
    const char* expectedProfileId = getenv("FISICS_SHIM_PROFILE_EXPECT_ID");
    const char* expectedProfileVersion = getenv("FISICS_SHIM_PROFILE_EXPECT_VERSION");
    const char* expectedOverlay = getenv("FISICS_SHIM_PROFILE_EXPECT_OVERLAY");
    const char* expectedInclude = getenv("FISICS_SHIM_PROFILE_EXPECT_INCLUDE");
    const char* sysPaths = getenv("SYSTEM_INCLUDE_PATHS");

    if (!expectedProfileId || !expectedProfileId[0]) expectedProfileId = defaultProfileId;
    if (!expectedProfileVersion || !expectedProfileVersion[0]) expectedProfileVersion = defaultProfileVersion;

    if (!profileId || !profileId[0]) {
        fprintf(stderr, "shim profile contract failed: FISICS_SHIM_PROFILE_ID is required\n");
        return false;
    }
    if (!profileVersion || !profileVersion[0]) {
        fprintf(stderr, "shim profile contract failed: FISICS_SHIM_PROFILE_VERSION is required\n");
        return false;
    }
    if (strcmp(profileId, expectedProfileId) != 0) {
        fprintf(stderr,
                "shim profile contract failed: profile id '%s' does not match expected '%s'\n",
                profileId,
                expectedProfileId);
        return false;
    }
    if (strcmp(profileVersion, expectedProfileVersion) != 0) {
        fprintf(stderr,
                "shim profile contract failed: profile version '%s' does not match expected '%s'\n",
                profileVersion,
                expectedProfileVersion);
        return false;
    }
    if (!expectedOverlay || !expectedOverlay[0]) {
        fprintf(stderr, "shim profile contract failed: FISICS_SHIM_PROFILE_EXPECT_OVERLAY is required\n");
        return false;
    }
    if (!expectedInclude || !expectedInclude[0]) {
        fprintf(stderr, "shim profile contract failed: FISICS_SHIM_PROFILE_EXPECT_INCLUDE is required\n");
        return false;
    }
    if (!sysPaths || !sysPaths[0]) {
        fprintf(stderr, "shim profile contract failed: SYSTEM_INCLUDE_PATHS is required\n");
        return false;
    }

    char** parsed = NULL;
    size_t parsedCount = 0;
    if (!compiler_collect_include_paths(sysPaths, &parsed, &parsedCount)) {
        fprintf(stderr, "shim profile contract failed: unable to parse SYSTEM_INCLUDE_PATHS\n");
        return false;
    }

    bool ok = true;
    if (parsedCount < 2 ||
        strcmp(parsed[0], expectedOverlay) != 0 ||
        strcmp(parsed[1], expectedInclude) != 0) {
        fprintf(stderr,
                "shim profile contract failed: SYSTEM_INCLUDE_PATHS must start with '%s:%s'\n",
                expectedOverlay,
                expectedInclude);
        ok = false;
    }

    size_t overlayIndex = (size_t)-1;
    size_t includeIndex = (size_t)-1;
    for (size_t i = 0; i < parsedCount; ++i) {
        if (overlayIndex == (size_t)-1 && strcmp(parsed[i], expectedOverlay) == 0) {
            overlayIndex = i;
        }
        if (includeIndex == (size_t)-1 && strcmp(parsed[i], expectedInclude) == 0) {
            includeIndex = i;
        }
    }
    if (overlayIndex == (size_t)-1 || includeIndex == (size_t)-1 || overlayIndex >= includeIndex) {
        fprintf(stderr,
                "shim profile contract failed: expected overlay path before include path in SYSTEM_INCLUDE_PATHS\n");
        ok = false;
    }

    compiler_free_include_paths(parsed, parsedCount);
    return ok;
}

// === Feature Toggles ===
#define ENABLE_LEXER_OUTPUT      0
#define ENABLE_AST_PRINT         1
#define ENABLE_SYNTAX_CHECK      1
#define ENABLE_CODEGEN           1

static bool parse_std_mode(const char* mode, CCDialect* dialect, bool* enableExtensions) {
    if (!mode || !dialect || !enableExtensions) return false;

    if (strcmp(mode, "c99") == 0 || strcmp(mode, "iso9899:1999") == 0 ||
        strcmp(mode, "gnu99") == 0) {
        *dialect = CC_DIALECT_C99;
        *enableExtensions = (strcmp(mode, "gnu99") == 0);
        return true;
    }
    if (strcmp(mode, "c11") == 0 || strcmp(mode, "iso9899:2011") == 0 ||
        strcmp(mode, "gnu11") == 0) {
        *dialect = CC_DIALECT_C11;
        *enableExtensions = (strcmp(mode, "gnu11") == 0);
        return true;
    }
    if (strcmp(mode, "c17") == 0 || strcmp(mode, "c18") == 0 ||
        strcmp(mode, "iso9899:2017") == 0 || strcmp(mode, "iso9899:2018") == 0 ||
        strcmp(mode, "gnu17") == 0 || strcmp(mode, "gnu18") == 0) {
        *dialect = CC_DIALECT_C17;
        *enableExtensions = (strcmp(mode, "gnu17") == 0 || strcmp(mode, "gnu18") == 0);
        return true;
    }
    return false;
}

int main(int argc, char **argv) {
    const char* progressEnv = getenv("FISICS_DEBUG_PROGRESS");
    bool debugProgress = progressEnv && progressEnv[0] && progressEnv[0] != '0';
    if (debugProgress) fprintf(stderr, "[main] start argc=%d\n", argc);
    profiler_init();
    if (profiler_enabled()) {
        atexit(profiler_shutdown);
    }
    const char* nanoEnv = getenv("MallocNanoZone");
    if (!nanoEnv) {
        setenv("MallocNanoZone", "0", 0);
    }

    if (!fisics_proc_guard_enter()) {
        return 1;
    }
    fisics_proc_guard_arm_timeout();

    LLVMInstallFatalErrorHandler(llvm_fatal_handler);

    const char *filename = NULL;
    bool preservePPNodes = false;
    const char* depsJsonPath = NULL;
    const char* diagsJsonPath = NULL;
    const char* diagsPackPath = NULL;
    const char* targetTriple = NULL;
    const char* dataLayout = NULL;
    bool compileOnly = false;
    const char* outputName = NULL;
    const char* linkerPath = NULL;
    bool dumpAst = false;
    bool dumpSemantic = false;
    bool dumpIR = false;
    bool dumpTokens = false;
    bool dumpLayout = false;
    bool enableTrigraphs = false;
    bool warnIgnoredInterop = true;
    bool errorIgnoredInterop = false;
    PreprocessMode preprocessMode = PREPROCESS_INTERNAL;
    const char* externalPreprocessCmd = NULL;
    const char* externalPreprocessArgs = NULL;
    CCDialect dialect = CC_DIALECT_C99;
    bool enableExtensions = false;
    StringList includePaths = {0};
    StringList macroDefines = {0};
    StringList forcedIncludes = {0};
    StringList inputCFiles = {0};
    StringList inputOFiles = {0};
    StringList linkerSearchPaths = {0};
    StringList linkerLibs = {0};
    StringList linkerFrameworks = {0};

    if (!validate_shim_profile_contract()) {
        return 1;
    }

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
    append_include_dir_if_exists(&includePaths, "/opt/homebrew/include");
    append_include_dir_if_exists(&includePaths, "/opt/homebrew/include/SDL2");
    append_include_dir_if_exists(&includePaths, "/usr/local/include");
    append_include_dir_if_exists(&includePaths, "/usr/local/include/SDL2");
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
        } else if (strcmp(argv[i], "--emit-diags-json") == 0 && i + 1 < argc) {
            diagsJsonPath = argv[++i];
        } else if (strcmp(argv[i], "--emit-diags-pack") == 0 && i + 1 < argc) {
            diagsPackPath = argv[++i];
        } else if (strcmp(argv[i], "--target") == 0 && i + 1 < argc) {
            targetTriple = argv[++i];
        } else if (strcmp(argv[i], "--data-layout") == 0 && i + 1 < argc) {
            dataLayout = argv[++i];
        } else if (strcmp(argv[i], "--dump-layout") == 0) {
            dumpLayout = true;
        } else if (strcmp(argv[i], "--dump-ast") == 0) {
            dumpAst = true;
        } else if (strcmp(argv[i], "--dump-sema") == 0) {
            dumpSemantic = true;
        } else if (strcmp(argv[i], "--dump-ir") == 0) {
            dumpIR = true;
        } else if (strcmp(argv[i], "--dump-tokens") == 0) {
            dumpTokens = true;
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
        } else if (strncmp(argv[i], "-D", 2) == 0) {
            const char* def = argv[i] + 2;
            if (!*def) {
                if (i + 1 >= argc) { fprintf(stderr, "-D requires a macro definition\n"); goto fail; }
                def = argv[++i];
            }
            if (!string_list_push(&macroDefines, def)) { fprintf(stderr, "OOM: macro define\n"); goto fail; }
        } else if (strcmp(argv[i], "-include") == 0) {
            if (i + 1 >= argc) { fprintf(stderr, "-include requires a path\n"); goto fail; }
            if (!string_list_push(&forcedIncludes, argv[++i])) { fprintf(stderr, "OOM: forced include\n"); goto fail; }
        } else if (strncmp(argv[i], "-include", 8) == 0) {
            const char* path = argv[i] + 8;
            if (!*path) { fprintf(stderr, "-include requires a path\n"); goto fail; }
            if (!string_list_push(&forcedIncludes, path)) { fprintf(stderr, "OOM: forced include\n"); goto fail; }
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
        } else if (strcmp(argv[i], "-framework") == 0) {
            if (i + 1 >= argc) { fprintf(stderr, "-framework requires a framework name\n"); goto fail; }
            if (!string_list_push(&linkerFrameworks, argv[++i])) {
                fprintf(stderr, "OOM: framework name\n");
                goto fail;
            }
        } else if (strncmp(argv[i], "--linker=", 9) == 0) {
            linkerPath = argv[i] + 9;
        } else if (strcmp(argv[i], "--no-warn-ignored-cc") == 0) {
            warnIgnoredInterop = false;
        } else if (strcmp(argv[i], "--error-ignored-cc") == 0) {
            errorIgnoredInterop = true;
        } else if (strncmp(argv[i], "--dialect=", 10) == 0) {
            const char* mode = argv[i] + 10;
            if (strcmp(mode, "c99") == 0) {
                dialect = CC_DIALECT_C99;
            } else if (strcmp(mode, "c11") == 0) {
                dialect = CC_DIALECT_C11;
            } else if (strcmp(mode, "c17") == 0) {
                dialect = CC_DIALECT_C17;
            } else {
                fprintf(stderr, "Error: unknown dialect '%s'\n", mode);
                goto fail;
            }
        } else if (strncmp(argv[i], "-std=", 5) == 0) {
            const char* mode = argv[i] + 5;
            if (!parse_std_mode(mode, &dialect, &enableExtensions)) {
                fprintf(stderr, "Warning: unsupported -std mode '%s' (keeping current dialect)\n", mode);
            }
        } else if (strncmp(argv[i], "--extensions=", 13) == 0) {
            const char* mode = argv[i] + 13;
            if (strcmp(mode, "gnu") == 0 || strcmp(mode, "on") == 0) {
                enableExtensions = true;
            } else if (strcmp(mode, "off") == 0 || strcmp(mode, "none") == 0) {
                enableExtensions = false;
            } else {
                fprintf(stderr, "Error: unknown extensions mode '%s'\n", mode);
                goto fail;
            }
        } else if (strncmp(argv[i], "--preprocess=", 13) == 0) {
            const char* mode = argv[i] + 13;
            if (strcmp(mode, "internal") == 0) {
                preprocessMode = PREPROCESS_INTERNAL;
            } else if (strcmp(mode, "external") == 0) {
                preprocessMode = PREPROCESS_EXTERNAL;
            } else if (strcmp(mode, "hybrid") == 0) {
                preprocessMode = PREPROCESS_HYBRID;
            } else {
                fprintf(stderr, "Error: unknown preprocess mode '%s'\n", mode);
                goto fail;
            }
        } else if (strcmp(argv[i], "--preprocess-cmd") == 0 && i + 1 < argc) {
            externalPreprocessCmd = argv[++i];
        } else if (strncmp(argv[i], "--preprocess-cmd=", 17) == 0) {
            externalPreprocessCmd = argv[i] + 17;
        } else if (strcmp(argv[i], "--preprocess-args") == 0 && i + 1 < argc) {
            externalPreprocessArgs = argv[++i];
        } else if (strncmp(argv[i], "--preprocess-args=", 18) == 0) {
            externalPreprocessArgs = argv[i] + 18;
        } else if (argv[i][0] != '-' && !filename) {
            filename = argv[i];
            if (has_extension(filename, ".c")) {
                string_list_push(&inputCFiles, filename);
            } else if (has_extension(filename, ".o") ||
                       has_extension(filename, ".a") ||
                       has_extension(filename, ".so") ||
                       has_extension(filename, ".dylib")) {
                string_list_push(&inputOFiles, filename);
            } else {
                fprintf(stderr, "Warning: unrecognized input extension for %s\n", filename);
            }
        } else if (argv[i][0] != '-') {
            if (has_extension(argv[i], ".c")) {
                string_list_push(&inputCFiles, argv[i]);
            } else if (has_extension(argv[i], ".o") ||
                       has_extension(argv[i], ".a") ||
                       has_extension(argv[i], ".so") ||
                       has_extension(argv[i], ".dylib")) {
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
    const char* ppEnv = getenv("FISICS_PREPROCESS");
    if (ppEnv && ppEnv[0]) {
        if (strcmp(ppEnv, "external") == 0) {
            preprocessMode = PREPROCESS_EXTERNAL;
        } else if (strcmp(ppEnv, "hybrid") == 0) {
            preprocessMode = PREPROCESS_HYBRID;
        } else if (strcmp(ppEnv, "internal") == 0) {
            preprocessMode = PREPROCESS_INTERNAL;
        }
    }
    const char* externalCmdEnv = getenv("FISICS_EXTERNAL_CPP");
    if (externalCmdEnv && externalCmdEnv[0]) {
        externalPreprocessCmd = externalCmdEnv;
    }
    const char* externalArgsEnv = getenv("FISICS_EXTERNAL_CPP_ARGS");
    if (externalArgsEnv && externalArgsEnv[0]) {
        externalPreprocessArgs = externalArgsEnv;
    }
    const char* dialectEnv = getenv("FISICS_DIALECT");
    if (dialectEnv && dialectEnv[0]) {
        if (strcmp(dialectEnv, "c99") == 0) {
            dialect = CC_DIALECT_C99;
        } else if (strcmp(dialectEnv, "c11") == 0) {
            dialect = CC_DIALECT_C11;
        } else if (strcmp(dialectEnv, "c17") == 0) {
            dialect = CC_DIALECT_C17;
        }
    }
    const char* extEnv = getenv("FISICS_EXTENSIONS");
    if (extEnv && extEnv[0]) {
        enableExtensions = (strcmp(extEnv, "1") == 0 ||
                            strcmp(extEnv, "on") == 0 ||
                            strcmp(extEnv, "gnu") == 0);
    }

    const char* depsEnv = getenv("EMIT_DEPS_JSON");
    if (!depsJsonPath && depsEnv && depsEnv[0] != '\0') {
        depsJsonPath = depsEnv;
    }
    const char* diagsEnv = getenv("EMIT_DIAGS_JSON");
    if (!diagsJsonPath && diagsEnv && diagsEnv[0] != '\0') {
        diagsJsonPath = diagsEnv;
    }
    const char* diagsPackEnv = getenv("EMIT_DIAGS_PACK");
    if (!diagsPackPath && diagsPackEnv && diagsPackEnv[0] != '\0') {
        diagsPackPath = diagsPackEnv;
    }
    const char* warnInteropEnv = getenv("WARN_IGNORED_CC");
    if (warnInteropEnv && warnInteropEnv[0] != '\0') {
        warnIgnoredInterop = warnInteropEnv[0] != '0';
    }
    const char* errInteropEnv = getenv("ERROR_IGNORED_CC");
    if (errInteropEnv && errInteropEnv[0] != '\0' && errInteropEnv[0] != '0') {
        errorIgnoredInterop = true;
    }

    if (dumpLayout) {
        const TargetLayout* tl = tl_from_triple(targetTriple);
        tl_dump(tl, stdout);
        return 0;
    }

    bool driverMode = compileOnly || outputName || inputOFiles.count > 0 ||
                      linkerSearchPaths.count > 0 || linkerLibs.count > 0 ||
                      linkerFrameworks.count > 0 || linkerPath;
    if (driverMode) {
        MainDriverConfig driverConfig = {
            .compileOnly = compileOnly,
            .preservePPNodes = preservePPNodes,
            .depsJsonPath = depsJsonPath,
            .diagsJsonPath = diagsJsonPath,
            .diagsPackPath = diagsPackPath,
            .targetTriple = targetTriple,
            .dataLayout = dataLayout,
            .outputName = outputName,
            .linkerPath = linkerPath,
            .dumpAst = dumpAst,
            .dumpSemantic = dumpSemantic,
            .dumpIR = dumpIR,
            .dumpTokens = dumpTokens,
            .enableTrigraphs = enableTrigraphs,
            .warnIgnoredInterop = warnIgnoredInterop,
            .errorIgnoredInterop = errorIgnoredInterop,
            .preprocessMode = preprocessMode,
            .externalPreprocessCmd = externalPreprocessCmd,
            .externalPreprocessArgs = externalPreprocessArgs,
            .dialect = dialect,
            .enableExtensions = enableExtensions,
            .enableCodegen = enableCodegen,
            .includePaths = &includePaths,
            .macroDefines = &macroDefines,
            .forcedIncludes = &forcedIncludes,
            .inputCFiles = &inputCFiles,
            .inputOFiles = &inputOFiles,
            .linkerSearchPaths = &linkerSearchPaths,
            .linkerLibs = &linkerLibs,
            .linkerFrameworks = &linkerFrameworks
        };
        int driverStatus = main_run_driver_mode(&driverConfig);
        if (driverStatus != 0) {
            goto fail;
        }
        string_list_free(&includePaths);
        string_list_free(&macroDefines);
        string_list_free(&forcedIncludes);
        string_list_free(&inputCFiles);
        string_list_free(&inputOFiles);
        string_list_free(&linkerSearchPaths);
        string_list_free(&linkerLibs);
        string_list_free(&linkerFrameworks);
        if (compileOnly) {
            return 0;
        }
        return llvm_shutdown_and_return(driverStatus);
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
        .macroDefines = (const char* const*)macroDefines.items,
        .macroDefineCount = macroDefines.count,
        .forcedIncludes = (const char* const*)forcedIncludes.items,
        .forcedIncludeCount = forcedIncludes.count,
        .preprocessMode = preprocessMode,
        .externalPreprocessCmd = externalPreprocessCmd,
        .externalPreprocessArgs = externalPreprocessArgs,
        .dialect = dialect,
        .enableExtensions = enableExtensions,
        .dumpAst = dumpAst || ENABLE_AST_PRINT,
        .dumpSemantic = dumpSemantic || ENABLE_SYNTAX_CHECK,
        .dumpIR = dumpIR || (enableCodegen && ENABLE_CODEGEN),
        .dumpTokens = dumpTokens,
        .enableCodegen = enableCodegen,
        .warnIgnoredInterop = warnIgnoredInterop,
        .errorIgnoredInterop = errorIgnoredInterop
    };

    CompileResult result;
    int status = compile_translation_unit(&options, &result);
    if (result.compilerCtx && diagsJsonPath && diagsJsonPath[0] != '\0') {
        CoreResult wr = compiler_diagnostics_write_core_dataset_json(result.compilerCtx, diagsJsonPath);
        if (wr.code != CORE_OK) {
            fprintf(stderr, "Warning: failed to write diagnostics JSON to %s\n", diagsJsonPath);
        }
    }
    if (result.compilerCtx && diagsPackPath && diagsPackPath[0] != '\0') {
        CoreResult wr = compiler_diagnostics_write_core_dataset_pack(result.compilerCtx, diagsPackPath);
        if (wr.code != CORE_OK) {
            fprintf(stderr, "Warning: failed to write diagnostics pack to %s\n", diagsPackPath);
        }
    }

    compile_result_destroy(&result);
    string_list_free(&includePaths);
    string_list_free(&macroDefines);
    string_list_free(&forcedIncludes);
    string_list_free(&inputCFiles);
    string_list_free(&inputOFiles);
    string_list_free(&linkerSearchPaths);
    string_list_free(&linkerLibs);
    string_list_free(&linkerFrameworks);
    return llvm_shutdown_and_return(status);

fail:
    string_list_free(&includePaths);
    string_list_free(&macroDefines);
    string_list_free(&forcedIncludes);
    string_list_free(&inputCFiles);
    string_list_free(&inputOFiles);
    string_list_free(&linkerSearchPaths);
    string_list_free(&linkerLibs);
    string_list_free(&linkerFrameworks);
    return llvm_shutdown_and_return(1);
}
