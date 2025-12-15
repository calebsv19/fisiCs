#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "fisics_frontend.h"

typedef struct {
    const char* name;
    const char* source;
    bool (*check)(const FisicsAnalysisResult* res, char* msg, size_t msgCap);
} StringTest;

typedef struct {
    const char* name;
    const char* entry_path;              // .c file to analyze
    const char* const* extra_includes;   // optional include dirs to prepend
    size_t extra_include_count;
    bool (*check)(const FisicsAnalysisResult* res, char* msg, size_t msgCap);
} DirTest;

typedef struct {
    const char** items;
    size_t count;
    size_t cap;
} StrList;

static void list_init(StrList* list) {
    list->items = NULL;
    list->count = 0;
    list->cap = 0;
}

static bool list_push(StrList* list, const char* value) {
    if (list->count == list->cap) {
        size_t newCap = list->cap ? list->cap * 2 : 4;
        const char** grown = (const char**)realloc(list->items, newCap * sizeof(char*));
        if (!grown) return false;
        list->items = grown;
        list->cap = newCap;
    }
    list->items[list->count++] = value;
    return true;
}

static char* read_file(const char* path, size_t* outLen) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long len = ftell(f);
    if (len < 0) { fclose(f); return NULL; }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return NULL; }
    char* buf = (char*)malloc((size_t)len + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t nread = fread(buf, 1, (size_t)len, f);
    fclose(f);
    buf[nread] = '\0';
    if (outLen) *outLen = nread;
    return buf;
}

static bool ends_with(const char* s, const char* suffix) {
    if (!s || !suffix) return false;
    size_t slen = strlen(s);
    size_t tlen = strlen(suffix);
    if (tlen > slen) return false;
    return memcmp(s + slen - tlen, suffix, tlen) == 0;
}

static void print_diag(const FisicsDiagnostic* d) {
    const char* kind = d->kind == DIAG_WARNING ? "WARNING"
                        : d->kind == DIAG_NOTE ? "NOTE"
                        : "ERROR";
    const char* file = d->file_path ? d->file_path : "<unknown>";
    fprintf(stderr, "%s:%d:%d: [%s] %s\n",
            file,
            d->line,
            d->column,
            kind,
            d->message ? d->message : "");
    if (d->hint) {
        fprintf(stderr, "    Hint: %s\n", d->hint);
    }
}

// ---- Expectations ----

static bool expect_no_diags(const FisicsAnalysisResult* res, char* msg, size_t msgCap) {
    if (res->diag_count == 0) return true;
    snprintf(msg, msgCap, "expected 0 diagnostics, saw %zu", res->diag_count);
    return false;
}

static bool expect_missing_include(const FisicsAnalysisResult* res, char* msg, size_t msgCap) {
    for (size_t i = 0; i < res->include_count; ++i) {
        const FisicsInclude* inc = &res->includes[i];
        if (inc->name && strcmp(inc->name, "missing.h") == 0) {
            if (!inc->resolved) return true;
            snprintf(msg, msgCap, "missing.h was marked resolved unexpectedly");
            return false;
        }
    }
    snprintf(msg, msgCap, "missing.h include not recorded");
    return false;
}

static bool expect_header_diag(const FisicsAnalysisResult* res, char* msg, size_t msgCap) {
    for (size_t i = 0; i < res->diag_count; ++i) {
        const FisicsDiagnostic* d = &res->diagnostics[i];
        if (d->file_path && ends_with(d->file_path, "bad.h")) {
            return true;
        }
    }
    snprintf(msg, msgCap, "no diagnostic attributed to bad.h");
    return false;
}

static bool expect_some_diag(const FisicsAnalysisResult* res, char* msg, size_t msgCap) {
    if (res->diag_count > 0) return true;
    snprintf(msg, msgCap, "expected at least one diagnostic, saw 0");
    return false;
}

static bool expect_include_resolved(const FisicsAnalysisResult* res,
                                    const char* name,
                                    char* msg,
                                    size_t msgCap) {
    for (size_t i = 0; i < res->include_count; ++i) {
        const FisicsInclude* inc = &res->includes[i];
        if (inc->name && strcmp(inc->name, name) == 0) {
            if (inc->resolved) return true;
            snprintf(msg, msgCap, "%s present but not resolved", name);
            return false;
        }
    }
    snprintf(msg, msgCap, "include %s not recorded", name);
    return false;
}

static bool expect_include_unresolved(const FisicsAnalysisResult* res,
                                      const char* name,
                                      char* msg,
                                      size_t msgCap) {
    for (size_t i = 0; i < res->include_count; ++i) {
        const FisicsInclude* inc = &res->includes[i];
        if (inc->name && strcmp(inc->name, name) == 0) {
            if (!inc->resolved) return true;
            snprintf(msg, msgCap, "%s unexpectedly resolved", name);
            return false;
        }
    }
    snprintf(msg, msgCap, "include %s not recorded", name);
    return false;
}

// ---- Test tables ----

static bool run_string_test(const StringTest* test,
                            const StrList* includes,
                            const StrList* macros,
                            int* failures) {
    FisicsFrontendOptions opts = {
        .include_paths = includes->items,
        .include_path_count = includes->count,
        .macro_defines = macros->items,
        .macro_define_count = macros->count
    };

    FisicsAnalysisResult res;
    bool ok = fisics_analyze_buffer(test->name, test->source, strlen(test->source), &opts, &res);
    bool pass = ok;
    char msg[256] = {0};
    if (test->check) {
        pass = test->check(&res, msg, sizeof(msg));
    }

    if (pass) {
        printf("PASS %s\n", test->name);
    } else {
        printf("FAIL %s: %s\n", test->name, msg[0] ? msg : "analysis failed");
        for (size_t i = 0; i < res.diag_count; ++i) print_diag(&res.diagnostics[i]);
        (*failures)++;
    }
    fisics_free_analysis_result(&res);
    return pass;
}

static bool run_dir_test(const DirTest* test,
                         const StrList* userIncludes,
                         const StrList* macros,
                         bool lenient,
                         int* failures) {
    size_t includeCount = userIncludes->count + test->extra_include_count;
    const char** includePaths = NULL;
    if (includeCount) {
        includePaths = (const char**)malloc(includeCount * sizeof(char*));
        if (!includePaths) {
            fprintf(stderr, "OOM allocating include paths\n");
            (*failures)++;
            return false;
        }
        size_t idx = 0;
        for (size_t i = 0; i < test->extra_include_count; ++i) {
            includePaths[idx++] = test->extra_includes[i];
        }
        for (size_t i = 0; i < userIncludes->count; ++i) {
            includePaths[idx++] = userIncludes->items[i];
        }
    }

    size_t len = 0;
    char* src = read_file(test->entry_path, &len);
    if (!src) {
        printf("FAIL %s: could not read %s\n", test->name, test->entry_path);
        free(includePaths);
        (*failures)++;
        return false;
    }

    FisicsFrontendOptions opts = {
        .include_paths = includePaths,
        .include_path_count = includeCount,
        .macro_defines = macros->items,
        .macro_define_count = macros->count,
        .lenient_mode = lenient ? 1 : -1
    };

    FisicsAnalysisResult res;
    bool ok = fisics_analyze_buffer(test->entry_path, src, len, &opts, &res);
    free(src);

    bool pass = ok;
    char msg[256] = {0};
    if (test->check) {
        pass = test->check(&res, msg, sizeof(msg));
    }

    if (pass) {
        printf("PASS %s\n", test->name);
    } else {
        printf("FAIL %s: %s\n", test->name, msg[0] ? msg : "analysis failed");
        for (size_t i = 0; i < res.diag_count; ++i) print_diag(&res.diagnostics[i]);
        (*failures)++;
    }

    fisics_free_analysis_result(&res);
    free(includePaths);
    return pass;
}

static bool file_exists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0;
}

static StringTest STRING_TESTS[] = {
    {
        .name = "missing_include.c",
        .source = "#include <missing.h>\nint main(void){return 0;}\n",
        .check = expect_missing_include,
    },
    {
        .name = "zero_arg_macro.c",
        .source = "#define Z() 123\nint x = Z();\n",
        .check = expect_no_diags,
    },
    {
        .name = "macro_spacing.c",
        .source = "#define F (1<<0)\n#define H ()\nint f(void){return 0;}\n",
        .check = expect_no_diags,
    },
    {
        .name = "zero_arg_space.c",
        .source = "#define EMPTY( ) 7\nint g(void){return EMPTY();}\n",
        .check = expect_no_diags,
    },
    {
        .name = "malformed_macro.c",
        .source = "#define BAD( int x\nint main(void){return 0;}\n",
        .check = expect_some_diag, // should warn/error but not crash
    },
    {
        .name = "conditional_ok.c",
        .source = "#if 1\nint a = 1;\n#endif\n",
        .check = expect_no_diags,
    },
    {
        .name = "conditional_mismatch.c",
        .source = "#endif\nint x;\n",
        .check = expect_some_diag, // should emit #endif mismatch
    },
};

static const char* DIAG_HEADER_INCLUDES[] = { "tests/preprocessor/diag_header_path" };
static const char* INC_SEARCH_INCLUDES[] = { "tests/preprocessor/harness_include_search/include" };
static const char* SMALL_PROJECT_INCLUDES[] = { "tests/fixtures/small_project/include" };
static const char* SYSTEM_LIKE_INCLUDES[] = { "tests/fixtures/system_like/sysroot/usr/include" };
static DirTest DIR_TESTS[] = {
    {
        .name = "diag_header_path",
        .entry_path = "tests/preprocessor/diag_header_path/main.c",
        .extra_includes = DIAG_HEADER_INCLUDES,
        .extra_include_count = 1,
        .check = expect_header_diag,
    },
    {
        .name = "include_search",
        .entry_path = "tests/preprocessor/harness_include_search/main.c",
        .extra_includes = INC_SEARCH_INCLUDES,
        .extra_include_count = 1,
        .check = NULL, // custom include check below
    },
    {
        .name = "small_project_main",
        .entry_path = "tests/fixtures/small_project/src/main.c",
        .extra_includes = SMALL_PROJECT_INCLUDES,
        .extra_include_count = 1,
        .check = expect_no_diags,
    },
    {
        .name = "small_project_b",
        .entry_path = "tests/fixtures/small_project/src/b.c",
        .extra_includes = SMALL_PROJECT_INCLUDES,
        .extra_include_count = 1,
        .check = expect_no_diags,
    },
    {
        .name = "system_like",
        .entry_path = "tests/fixtures/system_like/main.c",
        .extra_includes = SYSTEM_LIKE_INCLUDES,
        .extra_include_count = 1,
        .check = NULL, // custom include resolution check
    },
    {
        .name = "lenient_macro_file",
        .entry_path = "tests/preprocessor/harness_lenient_macro/main.c",
        .extra_includes = NULL,
        .extra_include_count = 0,
        .check = expect_some_diag,
    },
};

static void usage(const char* prog) {
    fprintf(stderr,
            "Usage: %s [--string NAME] [--dir PATH] [-I dir] [-DNAME[=VAL]] [--strict|--lenient]\n"
            "Default: run all built-in string and dir fixtures.\n",
            prog);
}

int main(int argc, char** argv) {
    const char* stringFilter = NULL;
    const char* dirPath = NULL;
    bool lenient = true;
    StrList userIncludes;
    StrList macroDefines;
    list_init(&userIncludes);
    list_init(&macroDefines);

    for (int i = 1; i < argc; ++i) {
        const char* arg = argv[i];
        if (strcmp(arg, "--string") == 0 && i + 1 < argc) {
            stringFilter = argv[++i];
        } else if (strcmp(arg, "--dir") == 0 && i + 1 < argc) {
            dirPath = argv[++i];
        } else if (strncmp(arg, "-I", 2) == 0) {
            const char* path = arg[2] ? arg + 2 : ((i + 1 < argc) ? argv[++i] : NULL);
            if (!path || !list_push(&userIncludes, path)) { usage(argv[0]); return 1; }
        } else if (strncmp(arg, "-D", 2) == 0) {
            const char* def = arg[2] ? arg + 2 : ((i + 1 < argc) ? argv[++i] : NULL);
            if (!def || !list_push(&macroDefines, def)) { usage(argv[0]); return 1; }
        } else if (strcmp(arg, "--strict") == 0) {
            lenient = false;
        } else if (strcmp(arg, "--lenient") == 0) {
            lenient = true;
        } else if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0) {
            usage(argv[0]);
            return 0;
        } else {
            usage(argv[0]);
            return 1;
        }
    }

    int failures = 0;

    // Run string tests
    for (size_t i = 0; i < sizeof(STRING_TESTS) / sizeof(STRING_TESTS[0]); ++i) {
        const StringTest* t = &STRING_TESTS[i];
        if (stringFilter && strcmp(stringFilter, t->name) != 0) continue;
        run_string_test(t, &userIncludes, &macroDefines, &failures);
    }
    if (stringFilter) {
        bool found = false;
        for (size_t i = 0; i < sizeof(STRING_TESTS) / sizeof(STRING_TESTS[0]); ++i) {
            if (strcmp(STRING_TESTS[i].name, stringFilter) == 0) { found = true; break; }
        }
        if (!found) {
            fprintf(stderr, "No string test named '%s'\n", stringFilter);
            failures++;
        }
    }

    // Directory tests: either specific path or built-ins.
    if (dirPath) {
        const char* entry = dirPath;
        char tmp[1024];
        if (file_exists(dirPath)) {
            // If a directory was provided, assume main.c inside it.
            struct stat st;
            if (stat(dirPath, &st) == 0 && S_ISDIR(st.st_mode)) {
                snprintf(tmp, sizeof(tmp), "%s/main.c", dirPath);
                entry = tmp;
            }
        }
        DirTest custom = {
            .name = dirPath,
            .entry_path = entry,
            .extra_includes = NULL,
            .extra_include_count = 0,
            .check = NULL
        };
        run_dir_test(&custom, &userIncludes, &macroDefines, lenient, &failures);
    } else {
        for (size_t i = 0; i < sizeof(DIR_TESTS) / sizeof(DIR_TESTS[0]); ++i) {
            const DirTest* t = &DIR_TESTS[i];
            if (strcmp(t->name, "include_search") == 0 || strcmp(t->name, "system_like") == 0) {
                // Custom include validation.
                size_t len = 0;
                char* src = read_file(t->entry_path, &len);
                if (!src) {
                    printf("FAIL %s: could not read source\n", t->name);
                    failures++;
                    continue;
                }
                size_t includeCount = userIncludes.count + t->extra_include_count;
                const char** includePaths = NULL;
                if (includeCount) {
                    includePaths = (const char**)malloc(includeCount * sizeof(char*));
                    size_t idx = 0;
                    for (size_t j = 0; j < t->extra_include_count; ++j) includePaths[idx++] = t->extra_includes[j];
                    for (size_t j = 0; j < userIncludes.count; ++j) includePaths[idx++] = userIncludes.items[j];
                }
                FisicsFrontendOptions opts = {
                    .include_paths = includePaths,
                    .include_path_count = includeCount,
                    .macro_defines = macroDefines.items,
                    .macro_define_count = macroDefines.count,
                    .lenient_mode = lenient ? 1 : -1
                };
                FisicsAnalysisResult res;
                bool ok = fisics_analyze_buffer(t->entry_path, src, len, &opts, &res);
                free(src);
                free(includePaths);
                if (!ok) {
                    printf("FAIL %s: analysis failed\n", t->name);
                    failures++;
                    fisics_free_analysis_result(&res);
                    continue;
                }
                char msg[128] = {0};
                bool pass = true;
                if (strcmp(t->name, "include_search") == 0) {
                    pass = expect_include_resolved(&res, "lib/foo.h", msg, sizeof(msg));
                } else if (strcmp(t->name, "system_like") == 0) {
                    pass = expect_include_resolved(&res, "sys/sys_stub.h", msg, sizeof(msg))
                        && expect_include_unresolved(&res, "missing_sys.h", msg, sizeof(msg));
                }
                if (pass) {
                    printf("PASS %s (includes)\n", t->name);
                } else {
                    printf("FAIL %s (includes): %s\n", t->name, msg);
                    for (size_t d = 0; d < res.diag_count; ++d) print_diag(&res.diagnostics[d]);
                    failures++;
                }
                fisics_free_analysis_result(&res);
            } else {
                run_dir_test(t, &userIncludes, &macroDefines, lenient, &failures);
            }
        }
    }

    free(userIncludes.items);
    free(macroDefines.items);
    return failures == 0 ? 0 : 1;
}
