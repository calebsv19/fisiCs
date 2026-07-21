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
#include "Compiler/build_graph.h"
#include "Compiler/build_manifest.h"
#include "Compiler/diagnostic_metadata.h"
#include "Compiler/object_emit.h"
#include "Compiler/pipeline.h"
#include "Extensions/extension_profile.h"
#include "Syntax/target_layout.h"
#include "Utils/profiler.h"
#include "Utils/utils.h"

#ifndef FISICS_VERSION
#define FISICS_VERSION "unknown"
#endif

static char g_proc_guard_path[PATH_MAX] = {0};
static pid_t g_proc_guard_group_pid = 0;
static int g_proc_guard_timeout_sec = 0;

static void print_cli_version(FILE* out) {
    fprintf(out, "fisiCs %s\n", FISICS_VERSION);
}

static void print_cli_usage(FILE* out, const char* argv0) {
    const char* program = (argv0 && argv0[0]) ? argv0 : "fisics";
    fprintf(out,
            "Usage: %s [options] <input.c> [more-inputs...] [-o output]\n"
            "\n"
            "Common commands:\n"
            "  %s --help                 Show this help text.\n"
            "  %s --version              Print compiler version.\n"
            "  %s -c examples/hello_world.c -o /tmp/fisics_hello_world.o\n"
            "                             Compile one C source to an object file.\n"
            "  %s compilation/multi_main.c compilation/multi_helper.c -o /tmp/fisics_multi_bin\n"
            "                             Compile and link C sources into an executable.\n"
            "\n"
            "Common options:\n"
            "  -I<dir>, -I <dir>          Add include search path.\n"
            "  -D<name>[=value]           Define a preprocessor macro.\n"
            "  -L<dir>, -l<name>          Forward library search/name to the linker.\n"
            "  -c                         Compile only; do not link.\n"
            "  -o <path>                  Write output to path.\n"
            "  --target <triple>          Select an LLVM object target.\n"
            "                             x86_64-unknown-none selects freestanding ELF64\n"
            "                             x86-64 with red-zone use forbidden.\n"
            "  --dump-ast                 Print parsed AST.\n"
            "  --dump-sema                Print semantic analysis output.\n"
            "  --dump-ir                  Print LLVM IR.\n"
            "  --emit-build-graph-json <path>\n"
            "                             Write source or manifest build graph JSON.\n"
            "  --build-manifest <path>    Load local build manifest.\n"
            "  --overlay=<name>           Enable opt-in overlay, e.g. physics-units.\n"
            "  --list-diagnostics --json  Print diagnostic explanation catalog.\n"
            "  --explain <code-or-name>   Explain one diagnostic.\n"
            "\n"
            "Examples:\n"
            "  %s examples/hello_world.c -o /tmp/fisics_hello_world\n"
            "  /tmp/fisics_hello_world\n"
            "  %s --overlay=physics-units --dump-sema -c examples/physics_units/ballistics_valid.c -o /tmp/fisics_ballistics_valid.o\n",
            program,
            program,
            program,
            program,
            program,
            program,
            program);
}

static bool is_historical_dev_fixture(const char* path) {
    return path &&
           (strcmp(path, "include/test.txt") == 0 ||
            strcmp(path, "./include/test.txt") == 0);
}

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

static void main_write_json_string(FILE* out, const char* text) {
    fputc('"', out);
    if (text) {
        for (const unsigned char* p = (const unsigned char*)text; *p; ++p) {
            switch (*p) {
                case '"': fputs("\\\"", out); break;
                case '\\': fputs("\\\\", out); break;
                case '\b': fputs("\\b", out); break;
                case '\f': fputs("\\f", out); break;
                case '\n': fputs("\\n", out); break;
                case '\r': fputs("\\r", out); break;
                case '\t': fputs("\\t", out); break;
                default:
                    if (*p < 0x20) {
                        fprintf(out, "\\u%04x", *p);
                    } else {
                        fputc((int)*p, out);
                    }
                    break;
            }
        }
    }
    fputc('"', out);
}

static void main_print_diagnostic_explanation_text(const FisicsDiagnosticExplanation* explanation) {
    int codeId = explanation->code_id;
    int categoryId = fisics_diag_category_id_from_code(codeId);
    printf("Diagnostic %d (%s)\n", codeId, fisics_diag_code_name(codeId));
    printf("category: %s\n", fisics_diag_category_name(categoryId));
    printf("stage: %s\n", fisics_diag_stage_name_from_code(codeId));
    printf("description: %s\n", explanation->description);
    printf("common causes: %s\n", explanation->common_causes);
    printf("next action: %s\n", explanation->next_action);
}

static void main_print_diagnostic_explanation_json_object(FILE* out,
                                                          const FisicsDiagnosticExplanation* explanation) {
    int codeId = explanation->code_id;
    int categoryId = fisics_diag_category_id_from_code(codeId);
    fputs("{\"code_id\":", out);
    fprintf(out, "%d", codeId);
    fputs(",\"code_name\":", out);
    main_write_json_string(out, fisics_diag_code_name(codeId));
    fputs(",\"category_id\":", out);
    fprintf(out, "%d", categoryId);
    fputs(",\"category_name\":", out);
    main_write_json_string(out, fisics_diag_category_name(categoryId));
    fputs(",\"stage\":", out);
    main_write_json_string(out, fisics_diag_stage_name_from_code(codeId));
    fputs(",\"description\":", out);
    main_write_json_string(out, explanation->description);
    fputs(",\"common_causes\":", out);
    main_write_json_string(out, explanation->common_causes);
    fputs(",\"next_action\":", out);
    main_write_json_string(out, explanation->next_action);
    fputc('}', out);
}

static void main_print_diagnostic_explanations_json(FILE* out) {
    size_t count = 0;
    const FisicsDiagnosticExplanation* explanations = fisics_diag_explanations(&count);
    fputs("{\"profile\":\"fisics_diagnostic_explanations\",\"schema_version\":1,\"diagnostics\":[", out);
    for (size_t i = 0; i < count; ++i) {
        if (i > 0) {
            fputc(',', out);
        }
        main_print_diagnostic_explanation_json_object(out, &explanations[i]);
    }
    fputs("]}\n", out);
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

static bool parse_compat_mode(const char* mode, CCCompatFeatures* compatFeatures) {
    if (!mode || !compatFeatures) return false;

    if (strcmp(mode, "0") == 0 ||
        strcmp(mode, "off") == 0 ||
        strcmp(mode, "none") == 0) {
        *compatFeatures = CC_COMPAT_NONE;
        return true;
    }
    if (strcmp(mode, "1") == 0 ||
        strcmp(mode, "on") == 0 ||
        strcmp(mode, "gnu") == 0 ||
        strcmp(mode, "all") == 0) {
        *compatFeatures = cc_gnu_compat_features();
        return true;
    }

    char* copy = strdup(mode);
    if (!copy) {
        return false;
    }

    bool ok = true;
    CCCompatFeatures parsed = CC_COMPAT_NONE;
    char* save = NULL;
    for (char* tok = strtok_r(copy, ",", &save);
         tok;
         tok = strtok_r(NULL, ",", &save)) {
        char* part = trim_in_place(tok);
        if (!part || part[0] == '\0') {
            ok = false;
            break;
        }
        if (strcmp(part, "gnu") == 0 ||
            strcmp(part, "profile-gnu") == 0 ||
            strcmp(part, "profile_gnu") == 0 ||
            strcmp(part, "all") == 0) {
            parsed |= cc_gnu_compat_features();
        } else if (strcmp(part, "block-pointers") == 0 ||
                   strcmp(part, "block_pointers") == 0 ||
                   strcmp(part, "blocks") == 0) {
            parsed |= CC_COMPAT_BLOCK_POINTERS;
        } else if (strcmp(part, "relaxed-atomic") == 0 ||
                   strcmp(part, "relaxed_atomic") == 0 ||
                   strcmp(part, "atomic") == 0) {
            parsed |= CC_COMPAT_RELAXED_ATOMIC;
        } else {
            ok = false;
            break;
        }
    }

    free(copy);
    if (!ok) {
        return false;
    }

    *compatFeatures = parsed;
    return true;
}

static bool parse_std_mode(const char* mode, CCDialect* dialect, CCCompatFeatures* compatFeatures) {
    if (!mode || !dialect || !compatFeatures) return false;

    if (strcmp(mode, "c99") == 0 || strcmp(mode, "iso9899:1999") == 0 ||
        strcmp(mode, "gnu99") == 0) {
        *dialect = CC_DIALECT_C99;
        *compatFeatures = (strcmp(mode, "gnu99") == 0)
            ? cc_gnu_compat_features()
            : CC_COMPAT_NONE;
        return true;
    }
    if (strcmp(mode, "c11") == 0 || strcmp(mode, "iso9899:2011") == 0 ||
        strcmp(mode, "gnu11") == 0) {
        *dialect = CC_DIALECT_C11;
        *compatFeatures = (strcmp(mode, "gnu11") == 0)
            ? cc_gnu_compat_features()
            : CC_COMPAT_NONE;
        return true;
    }
    if (strcmp(mode, "c17") == 0 || strcmp(mode, "c18") == 0 ||
        strcmp(mode, "iso9899:2017") == 0 || strcmp(mode, "iso9899:2018") == 0 ||
        strcmp(mode, "gnu17") == 0 || strcmp(mode, "gnu18") == 0) {
        *dialect = CC_DIALECT_C17;
        *compatFeatures = (strcmp(mode, "gnu17") == 0 || strcmp(mode, "gnu18") == 0)
            ? cc_gnu_compat_features()
            : CC_COMPAT_NONE;
        return true;
    }
    return false;
}

static bool main_path_is_absolute(const char* path) {
    return path && path[0] == '/';
}

static char* main_join_path(const char* root, const char* rel) {
    if (!rel) return NULL;
    if (main_path_is_absolute(rel)) return strdup(rel);
    if (strcmp(rel, ".") == 0) return strdup((root && root[0]) ? root : ".");
    if (strncmp(rel, "./", 2u) == 0) rel += 2;
    if (!root || !root[0] || strcmp(root, ".") == 0) return strdup(rel);
    size_t rlen = strlen(root);
    size_t llen = strlen(rel);
    bool slash = root[rlen - 1u] != '/';
    char* out = (char*)malloc(rlen + (slash ? 1u : 0u) + llen + 1u);
    if (!out) return NULL;
    memcpy(out, root, rlen);
    size_t pos = rlen;
    if (slash) out[pos++] = '/';
    memcpy(out + pos, rel, llen + 1u);
    return out;
}

static bool main_mkdir_parent_recursive(const char* path) {
    if (!path || !path[0]) return false;
    char* copy = strdup(path);
    if (!copy) return false;
    char* slash = strrchr(copy, '/');
    if (!slash) {
        free(copy);
        return true;
    }
    if (slash == copy) {
        free(copy);
        return true;
    }
    *slash = '\0';
    for (char* p = copy + 1; *p; ++p) {
        if (*p != '/') continue;
        *p = '\0';
        if (copy[0] && mkdir(copy, 0755) != 0 && errno != EEXIST) {
            free(copy);
            return false;
        }
        *p = '/';
    }
    if (mkdir(copy, 0755) != 0 && errno != EEXIST) {
        free(copy);
        return false;
    }
    free(copy);
    return true;
}

static bool main_string_list_push_joined(StringList* list, const char* root, const char* rel) {
    char* joined = main_join_path(root, rel);
    if (!joined) return false;
    bool ok = string_list_push(list, joined);
    free(joined);
    return ok;
}

static const char* main_memory_check_runtime_archive_path(void) {
    const char* override = getenv("FISICS_MEMCHECK_RUNTIME_LIB");
    if (override && override[0]) return override;
    return "build/unsanitized/libfisics_memcheck_runtime.a";
}

static int main_run_link_argv(StringList* argvList) {
    if (!argvList || argvList->count == 0) return 1;
    char** execArgv = (char**)calloc(argvList->count + 1u, sizeof(char*));
    if (!execArgv) {
        fprintf(stderr, "OOM: manifest linker argv\n");
        return 1;
    }
    for (size_t i = 0; i < argvList->count; ++i) {
        execArgv[i] = argvList->items[i];
    }
    execArgv[argvList->count] = NULL;

    pid_t pid = fork();
    if (pid == 0) {
        execvp(execArgv[0], execArgv);
        perror("execvp");
        _exit(127);
    }
    if (pid < 0) {
        perror("fork");
        free(execArgv);
        return 1;
    }

    fprintf(stderr, "[manifest-link]");
    for (size_t i = 0; i < argvList->count; ++i) {
        fprintf(stderr, " %s", argvList->items[i]);
    }
    fprintf(stderr, "\n");

    int statusCode = 0;
    if (waitpid(pid, &statusCode, 0) == -1) {
        perror("waitpid");
        statusCode = 1;
    } else if (WIFEXITED(statusCode)) {
        statusCode = WEXITSTATUS(statusCode);
        if (statusCode != 0) {
            fprintf(stderr, "Manifest linker exited with status %d\n", statusCode);
        }
    } else {
        fprintf(stderr, "Manifest linker terminated abnormally\n");
        statusCode = 1;
    }
    free(execArgv);
    return statusCode;
}

static int main_execute_build_manifest(const FisicsBuildManifest* manifest,
                                       const StringList* baseIncludePaths,
                                       const StringList* cliMacroDefines,
                                       const StringList* forcedIncludes,
                                       const char* targetTriple,
                                       const char* dataLayout,
                                       const char* linkerPath,
                                       PreprocessMode preprocessMode,
                                       const char* externalPreprocessCmd,
                                       const char* externalPreprocessArgs,
                                       CCDialect baseDialect,
                                       CCCompatFeatures baseCompatFeatures,
                                       FisicsOverlayFeatures baseOverlayFeatures,
                                       bool preservePPNodes,
                                       bool enableTrigraphs,
                                       bool warnIgnoredInterop,
                                       bool errorIgnoredInterop,
                                       int enableCodegen) {
    if (!manifest) return 1;
    if (!enableCodegen) {
        fprintf(stderr, "Error: codegen disabled (DISABLE_CODEGEN set); cannot execute build manifest\n");
        return 1;
    }

    CCDialect dialect = baseDialect;
    CCCompatFeatures compatFeatures = baseCompatFeatures;
    if (manifest->defaults.standard && manifest->defaults.standard[0]) {
        if (!parse_std_mode(manifest->defaults.standard, &dialect, &compatFeatures)) {
            fprintf(stderr,
                    "Error: unsupported manifest default standard '%s'\n",
                    manifest->defaults.standard);
            return 1;
        }
    }

    FisicsOverlayFeatures overlayFeatures = baseOverlayFeatures;
    if (manifest->defaults.overlays.count > 0) {
        StringList overlayParts = {0};
        for (size_t i = 0; i < manifest->defaults.overlays.count; ++i) {
            if (!string_list_push(&overlayParts, manifest->defaults.overlays.items[i])) {
                string_list_free(&overlayParts);
                fprintf(stderr, "OOM: manifest overlay list\n");
                return 1;
            }
        }
        size_t joinedLen = 0;
        for (size_t i = 0; i < overlayParts.count; ++i) {
            joinedLen += strlen(overlayParts.items[i]) + 1u;
        }
        char* joined = (char*)malloc(joinedLen + 1u);
        if (!joined) {
            string_list_free(&overlayParts);
            fprintf(stderr, "OOM: manifest overlay mode\n");
            return 1;
        }
        joined[0] = '\0';
        for (size_t i = 0; i < overlayParts.count; ++i) {
            if (i) strcat(joined, ",");
            strcat(joined, overlayParts.items[i]);
        }
        FisicsOverlayFeatures manifestOverlays = FISICS_OVERLAY_NONE;
        bool ok = fisics_parse_overlay_mode(joined, &manifestOverlays);
        free(joined);
        string_list_free(&overlayParts);
        if (!ok) {
            fprintf(stderr, "Error: unsupported manifest overlay list\n");
            return 1;
        }
        overlayFeatures |= manifestOverlays;
    }

    StringList includePaths = {0};
    StringList macroDefines = {0};
    int statusCode = 0;
    for (size_t i = 0; i < baseIncludePaths->count; ++i) {
        if (!string_list_push(&includePaths, baseIncludePaths->items[i])) {
            statusCode = 1;
            goto done;
        }
    }
    for (size_t i = 0; i < manifest->defaults.includeDirs.count; ++i) {
        if (!main_string_list_push_joined(&includePaths,
                                          manifest->resolvedRoot,
                                          manifest->defaults.includeDirs.items[i])) {
            statusCode = 1;
            goto done;
        }
    }
    for (size_t i = 0; cliMacroDefines && i < cliMacroDefines->count; ++i) {
        if (!string_list_push(&macroDefines, cliMacroDefines->items[i])) {
            statusCode = 1;
            goto done;
        }
    }
    for (size_t i = 0; i < manifest->defaults.defines.count; ++i) {
        if (!string_list_push(&macroDefines, manifest->defaults.defines.items[i])) {
            statusCode = 1;
            goto done;
        }
    }

    for (size_t i = 0; i < manifest->translationUnitCount; ++i) {
        const FisicsBuildManifestTranslationUnit* tu = &manifest->translationUnits[i];
        if (!main_mkdir_parent_recursive(tu->resolvedObject)) {
            fprintf(stderr, "Error: failed to create object output directory for %s\n", tu->resolvedObject);
            statusCode = 1;
            goto done;
        }
        CompileOptions options = {
            .inputPath = tu->resolvedSource,
            .preservePPNodes = preservePPNodes,
            .depsJsonPath = NULL,
            .targetTriple = targetTriple,
            .dataLayout = dataLayout,
            .includePaths = (const char* const*)includePaths.items,
            .includePathCount = includePaths.count,
            .macroDefines = (const char* const*)macroDefines.items,
            .macroDefineCount = macroDefines.count,
            .forcedIncludes = forcedIncludes ? (const char* const*)forcedIncludes->items : NULL,
            .forcedIncludeCount = forcedIncludes ? forcedIncludes->count : 0u,
            .preprocessMode = preprocessMode,
            .externalPreprocessCmd = externalPreprocessCmd,
            .externalPreprocessArgs = externalPreprocessArgs,
            .dialect = dialect,
            .compatFeatures = compatFeatures,
            .overlayFeatures = overlayFeatures,
            .dumpAst = false,
            .dumpSemantic = false,
            .dumpIR = false,
            .dumpTokens = false,
            .enableCodegen = true,
            .enableTrigraphs = enableTrigraphs,
            .warnIgnoredInterop = warnIgnoredInterop,
            .errorIgnoredInterop = errorIgnoredInterop
        };
        CompileResult result;
        int compileStatus = compile_translation_unit(&options, &result);
        if (compileStatus != 0 || result.semanticErrors > 0 || !result.module) {
            fprintf(stderr, "Error: manifest compilation failed for %s\n", tu->resolvedSource);
            compile_result_destroy(&result);
            statusCode = 1;
            goto done;
        }
        char* emitErr = NULL;
        if (!compiler_emit_object_file(result.module,
                                       targetTriple,
                                       dataLayout,
                                       tu->resolvedObject,
                                       &emitErr)) {
            fprintf(stderr,
                    "Error: failed to emit manifest object %s: %s\n",
                    tu->resolvedObject,
                    emitErr ? emitErr : "unknown error");
            free(emitErr);
            compile_result_destroy(&result);
            statusCode = 1;
            goto done;
        }
        free(emitErr);
        compile_result_destroy(&result);
    }

    if (manifest->link.output && manifest->link.output[0]) {
        if (!main_mkdir_parent_recursive(manifest->link.resolvedOutput)) {
            fprintf(stderr,
                    "Error: failed to create link output directory for %s\n",
                    manifest->link.resolvedOutput);
            statusCode = 1;
            goto done;
        }
        StringList argvList = {0};
        const char* linker = linkerPath ? linkerPath : "clang";
        bool ok = string_list_push(&argvList, linker);
        for (size_t i = 0; ok && i < manifest->translationUnitCount; ++i) {
            ok = string_list_push(&argvList, manifest->translationUnits[i].resolvedObject);
        }
        if (ok && (overlayFeatures & FISICS_OVERLAY_MEMORY_CHECK) != 0) {
            ok = string_list_push(&argvList, main_memory_check_runtime_archive_path());
        }
        for (size_t i = 0; ok && i < manifest->link.libraryDirs.count; ++i) {
            char* dirPath = main_join_path(manifest->resolvedRoot, manifest->link.libraryDirs.items[i]);
            if (!dirPath) {
                ok = false;
                break;
            }
            size_t len = strlen(dirPath) + 3u;
            char* flag = (char*)malloc(len);
            if (!flag) {
                free(dirPath);
                ok = false;
                break;
            }
            snprintf(flag, len, "-L%s", dirPath);
            ok = string_list_push(&argvList, flag);
            free(flag);
            free(dirPath);
        }
        for (size_t i = 0; ok && i < manifest->link.libraries.count; ++i) {
            size_t len = strlen(manifest->link.libraries.items[i]) + 3u;
            char* flag = (char*)malloc(len);
            if (!flag) {
                ok = false;
                break;
            }
            snprintf(flag, len, "-l%s", manifest->link.libraries.items[i]);
            ok = string_list_push(&argvList, flag);
            free(flag);
        }
        for (size_t i = 0; ok && i < manifest->link.args.count; ++i) {
            ok = string_list_push(&argvList, manifest->link.args.items[i]);
        }
        if (ok) {
            ok = string_list_push(&argvList, "-o") &&
                 string_list_push(&argvList, manifest->link.resolvedOutput);
        }
        if (!ok) {
            fprintf(stderr, "Error: failed to prepare manifest linker invocation\n");
            string_list_free(&argvList);
            statusCode = 1;
            goto done;
        }
        statusCode = main_run_link_argv(&argvList);
        string_list_free(&argvList);
        if (statusCode != 0) goto done;
    }

done:
    string_list_free(&includePaths);
    string_list_free(&macroDefines);
    return statusCode;
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

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_cli_usage(stdout, argv[0]);
            return 0;
        }
        if (strcmp(argv[i], "--version") == 0) {
            print_cli_version(stdout);
            return 0;
        }
    }

    if (!fisics_proc_guard_enter()) {
        return 1;
    }
    fisics_proc_guard_arm_timeout();

    LLVMInstallFatalErrorHandler(llvm_fatal_handler);

    const char *filename = NULL;
    bool preservePPNodes = false;
    const char* depsJsonPath = NULL;
    const char* buildGraphJsonPath = NULL;
    const char* buildManifestPath = NULL;
    const char* compileDbPath = NULL;
    const char* diagsJsonPath = NULL;
    const char* diagsPackPath = NULL;
    const char* explainDiagnosticQuery = NULL;
    const char* targetTriple = NULL;
    const char* dataLayout = NULL;
    bool compileOnly = false;
    bool buildManifestDryRun = false;
    bool buildManifestJson = false;
    bool listDiagnostics = false;
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
    CCCompatFeatures compatFeatures = CC_COMPAT_NONE;
    FisicsOverlayFeatures overlayFeatures = FISICS_OVERLAY_NONE;
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
        } else if (strcmp(argv[i], "--emit-build-graph-json") == 0 && i + 1 < argc) {
            buildGraphJsonPath = argv[++i];
        } else if (strcmp(argv[i], "--build-manifest") == 0 && i + 1 < argc) {
            buildManifestPath = argv[++i];
        } else if (strcmp(argv[i], "--emit-compile-db") == 0 && i + 1 < argc) {
            compileDbPath = argv[++i];
        } else if (strcmp(argv[i], "--dry-run") == 0) {
            buildManifestDryRun = true;
        } else if (strcmp(argv[i], "--json") == 0) {
            buildManifestJson = true;
        } else if (strcmp(argv[i], "--explain") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: --explain requires a diagnostic code or name\n");
                goto fail;
            }
            explainDiagnosticQuery = argv[++i];
        } else if (strcmp(argv[i], "--list-diagnostics") == 0) {
            listDiagnostics = true;
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
            if (!parse_std_mode(mode, &dialect, &compatFeatures)) {
                fprintf(stderr, "Warning: unsupported -std mode '%s' (keeping current dialect)\n", mode);
            }
        } else if (strncmp(argv[i], "--compat=", 9) == 0) {
            const char* mode = argv[i] + 9;
            if (!parse_compat_mode(mode, &compatFeatures)) {
                fprintf(stderr, "Error: unknown compatibility mode '%s'\n", mode);
                goto fail;
            }
        } else if (strncmp(argv[i], "--extensions=", 13) == 0) {
            const char* mode = argv[i] + 13;
            if (!parse_compat_mode(mode, &compatFeatures)) {
                fprintf(stderr, "Error: unknown extensions mode '%s'\n", mode);
                goto fail;
            }
        } else if (strncmp(argv[i], "--overlay=", 10) == 0) {
            const char* mode = argv[i] + 10;
            if (!fisics_parse_overlay_mode(mode, &overlayFeatures)) {
                fprintf(stderr, "Error: unknown overlay mode '%s'\n", mode);
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
            if (has_extension(filename, ".c") || is_historical_dev_fixture(filename)) {
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
            if (has_extension(argv[i], ".c") || is_historical_dev_fixture(argv[i])) {
                string_list_push(&inputCFiles, argv[i]);
            } else if (has_extension(argv[i], ".o") ||
                       has_extension(argv[i], ".a") ||
                       has_extension(argv[i], ".so") ||
                       has_extension(argv[i], ".dylib")) {
                string_list_push(&inputOFiles, argv[i]);
            } else {
                fprintf(stderr, "Warning: unrecognized input extension for %s\n", argv[i]);
            }
        } else {
            fprintf(stderr, "Error: unknown option '%s'\n", argv[i]);
            fprintf(stderr, "Run '%s --help' for usage.\n", argv[0] && argv[0][0] ? argv[0] : "fisics");
            goto fail;
        }
    }
    if (explainDiagnosticQuery) {
        const FisicsDiagnosticExplanation* explanation =
            fisics_diag_explanation_by_query(explainDiagnosticQuery);
        if (!explanation) {
            fprintf(stderr, "Error: unknown diagnostic code or name: %s\n", explainDiagnosticQuery);
            goto fail;
        }
        if (buildManifestJson) {
            main_print_diagnostic_explanation_json_object(stdout, explanation);
            fputc('\n', stdout);
        } else {
            main_print_diagnostic_explanation_text(explanation);
        }
        string_list_free(&includePaths);
        string_list_free(&macroDefines);
        string_list_free(&forcedIncludes);
        string_list_free(&inputCFiles);
        string_list_free(&inputOFiles);
        string_list_free(&linkerSearchPaths);
        string_list_free(&linkerLibs);
        string_list_free(&linkerFrameworks);
        return llvm_shutdown_and_return(0);
    }
    if (listDiagnostics) {
        if (!buildManifestJson) {
            fprintf(stderr, "Error: --list-diagnostics currently requires --json\n");
            goto fail;
        }
        main_print_diagnostic_explanations_json(stdout);
        string_list_free(&includePaths);
        string_list_free(&macroDefines);
        string_list_free(&forcedIncludes);
        string_list_free(&inputCFiles);
        string_list_free(&inputOFiles);
        string_list_free(&linkerSearchPaths);
        string_list_free(&linkerLibs);
        string_list_free(&linkerFrameworks);
        return llvm_shutdown_and_return(0);
    }
    if (!buildManifestPath && !filename && inputCFiles.count == 0 && inputOFiles.count == 0) {
        fprintf(stderr, "Error: no input files.\n");
        print_cli_usage(stderr, argv[0]);
        goto fail;
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
    const char* compatEnv = getenv("FISICS_COMPAT");
    if (compatEnv && compatEnv[0]) {
        if (!parse_compat_mode(compatEnv, &compatFeatures)) {
            fprintf(stderr,
                    "Warning: unsupported FISICS_COMPAT mode '%s' (keeping current compatibility profile)\n",
                    compatEnv);
        }
    }
    const char* extEnv = getenv("FISICS_EXTENSIONS");
    if (extEnv && extEnv[0]) {
        if (!parse_compat_mode(extEnv, &compatFeatures)) {
            fprintf(stderr,
                    "Warning: unsupported FISICS_EXTENSIONS mode '%s' (keeping current compatibility profile)\n",
                    extEnv);
        }
    }
    const char* overlayEnv = getenv("FISICS_OVERLAY");
    if (overlayEnv && overlayEnv[0]) {
        if (!fisics_parse_overlay_mode(overlayEnv, &overlayFeatures)) {
            fprintf(stderr,
                    "Warning: unsupported FISICS_OVERLAY mode '%s' (keeping current overlay profile)\n",
                    overlayEnv);
        }
    }

    const char* depsEnv = getenv("EMIT_DEPS_JSON");
    if (!depsJsonPath && depsEnv && depsEnv[0] != '\0') {
        depsJsonPath = depsEnv;
    }
    const char* graphEnv = getenv("EMIT_BUILD_GRAPH_JSON");
    if (!buildGraphJsonPath && graphEnv && graphEnv[0] != '\0') {
        buildGraphJsonPath = graphEnv;
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

    if (buildManifestPath) {
        if (buildManifestDryRun && !buildManifestJson) {
            fprintf(stderr, "Error: --build-manifest --dry-run currently requires --json\n");
            goto fail;
        }
        FisicsBuildManifest manifest = {0};
        FisicsBuildManifestDiagnostic manifestDiag = {0};
        if (!fisics_build_manifest_load_file(buildManifestPath, &manifest, &manifestDiag)) {
            fprintf(stderr,
                    "Error: failed to load build manifest %s: %s\n",
                    buildManifestPath,
                    manifestDiag.message[0] ? manifestDiag.message : "unknown error");
            goto fail;
        }
        bool artifactOk = true;
        if (compileDbPath && compileDbPath[0]) {
            artifactOk = fisics_build_graph_write_compile_commands_json(compileDbPath, &manifest);
            if (!artifactOk) {
                fprintf(stderr, "Error: failed to write compile database to %s\n", compileDbPath);
            }
        }
        if (artifactOk && buildManifestDryRun) {
            const char* outPath = (buildGraphJsonPath && buildGraphJsonPath[0]) ? buildGraphJsonPath : "-";
            FisicsBuildGraphManifestOptions graphOptions = {
                .outputPath = outPath,
                .manifest = &manifest,
                .dryRun = true,
                .partial = false,
                .fatal = false
            };
            artifactOk = fisics_build_graph_write_manifest_dry_run_json(&graphOptions);
            if (!artifactOk) {
                fprintf(stderr, "Error: failed to write manifest dry-run build graph JSON\n");
            }
        }
        if (artifactOk && !buildManifestDryRun && (!compileDbPath || !compileDbPath[0])) {
            int execStatus = main_execute_build_manifest(&manifest,
                                                         &includePaths,
                                                         &macroDefines,
                                                         &forcedIncludes,
                                                         targetTriple,
                                                         dataLayout,
                                                         linkerPath,
                                                         preprocessMode,
                                                         externalPreprocessCmd,
                                                         externalPreprocessArgs,
                                                         dialect,
                                                         compatFeatures,
                                                         overlayFeatures,
                                                         preservePPNodes,
                                                         enableTrigraphs,
                                                         warnIgnoredInterop,
                                                         errorIgnoredInterop,
                                                         enableCodegen);
            if (execStatus != 0) {
                artifactOk = false;
            }
        }
        fisics_build_manifest_free(&manifest);
        if (!artifactOk) {
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
        return llvm_shutdown_and_return(0);
    }

    bool driverMode = compileOnly || outputName || inputOFiles.count > 0 ||
                      linkerSearchPaths.count > 0 || linkerLibs.count > 0 ||
                      linkerFrameworks.count > 0 || linkerPath;
    if (driverMode) {
        MainDriverConfig driverConfig = {
            .compileOnly = compileOnly,
            .preservePPNodes = preservePPNodes,
            .depsJsonPath = depsJsonPath,
            .buildGraphJsonPath = buildGraphJsonPath,
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
            .compatFeatures = compatFeatures,
            .overlayFeatures = overlayFeatures,
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
        .compatFeatures = compatFeatures,
        .overlayFeatures = overlayFeatures,
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
    if (status == 0 && result.semanticErrors > 0) {
        status = 1;
    }
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
    if (result.compilerCtx && buildGraphJsonPath && buildGraphJsonPath[0] != '\0') {
        FisicsBuildGraphSourceOptions graphOptions = {
            .outputPath = buildGraphJsonPath,
            .inputPath = inputPath,
            .outputObject = fisics_build_graph_derive_object_path(inputPath),
            .targetTriple = targetTriple,
            .dataLayout = dataLayout,
            .includePaths = (const char* const*)includePaths.items,
            .includePathCount = includePaths.count,
            .macroDefines = (const char* const*)macroDefines.items,
            .macroDefineCount = macroDefines.count,
            .forcedIncludes = (const char* const*)forcedIncludes.items,
            .forcedIncludeCount = forcedIncludes.count,
            .dialect = dialect,
            .compatFeatures = compatFeatures,
            .overlayFeatures = overlayFeatures,
            .compileOnly = false,
            .enableCodegen = enableCodegen != 0,
            .partial = status != 0,
            .fatal = false
        };
        if (!fisics_build_graph_write_source_json(&graphOptions, result.compilerCtx)) {
            fprintf(stderr, "Warning: failed to write build graph JSON to %s\n", buildGraphJsonPath);
        }
        free((char*)graphOptions.outputObject);
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
